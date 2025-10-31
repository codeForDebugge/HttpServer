#include <TCPServer.h>

ThreadPool pool(25);
TCPServer::TCPServer()
{
   keep_running = true;
}

bool TCPServer::InitializeServer()
{
    serverSocket = socket(AF_INET,SOCK_STREAM,0);
    if (serverSocket < 0)
    {
        perror("ServerSocket");
        return false;
    }
    int opt = 1;
    if (setsockopt(serverSocket,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt)) < 0)
    {
        perror("ServerSocket setopt");
        return false;
    }
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) < 0) {
        perror("setsockopt(SO_REUSEPORT)");
    }
    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons((uint16_t)PORT);

    if (bind(serverSocket,(sockaddr*)(&address),sizeof(address))<0)
    {
        perror("server socket Bind");
        return false;
    }

    if (listen(serverSocket,5)<0)
    {
        perror("server Listen failed");
        closeSocket(serverSocket);
        return false;
    }
    epoll_fd = epoll_create1(0);

    if (epoll_fd == -1)
    {
        perror("Epoll Creation failed");
        closeSocket(serverSocket);
        return false;        
    }
    std::cout<<"server listening at port "<<PORT<<" serverSocket "<<serverSocket<<std::endl;
    return true;
}


TCPServer::HttpRequest TCPServer::parseHttpRequest(const std::string &raw) {
    HttpRequest req;
    std::istringstream stream(raw);
    std::string line;

    // ---- First line: METHOD PATH VERSION ----
    std::getline(stream, line);
    std::istringstream firstLine(line);
    firstLine >> req.method >> req.path >> req.version;

    // ---- Next lines: headers ----
    while (std::getline(stream, line) && line != "\r") {
        size_t pos = line.find(":");
        if (pos != std::string::npos) {
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);
            // trim
            key.erase(key.find_last_not_of(" \r\n\t") + 1);
            value.erase(0, value.find_first_not_of(" \r\n\t"));
            value.erase(value.find_last_not_of(" \r\n\t") + 1);
            req.headers[key] = value;
        }
    }

    // ---- Body (if Content-Length present) ----
    if (req.headers.find("Content-Length") != req.headers.end()) {
        int len = std::stoi(req.headers["Content-Length"]);
        std::string body(len, '\0');
        stream.read(&body[0], len);
        req.body = body;
    }

    return req;
}
void TCPServer::handleClient(int clientSocket)
{
    std::cout<<"Recieved recv request thread: "<<std::this_thread::get_id()<<std::endl;
    char buffer[256];
    int recvByte = recv(clientSocket,buffer,sizeof(buffer),0);

    if (recvByte < 0)
    {
        if (errno != EAGAIN && errno != EWOULDBLOCK)
        {
            std::cout<<"Error in recv thread "<<std::this_thread::get_id()<<std::endl;
            close(clientSocket);
        }
        return;
    }
    else if (recvByte == 0)
    {
        std::cout<<"client disconnected socket \n";
        close(clientSocket);
    }
    else
    {
        buffer[recvByte] = '\0';
        std::string rawRequest(buffer);          
        HttpRequest req = parseHttpRequest(rawRequest);
        std::cout << "Method: " << req.method << std::endl;
        std::cout << "Path: " << req.path << std::endl;
        std::cout << "Version: " << req.version << std::endl;
        sendhttpResponse(clientSocket);
    }
}
void TCPServer::epollThreadPool()
{
    int MAXEVENT = 10;
    struct epoll_event ev[MAXEVENT];
    std::cout<<"Epoll Thread pool Entry \n";
    while(true)
    {
        int counter = epoll_wait(epoll_fd,ev,MAXEVENT,-1);
        for(int i = 0;i<counter; i++)
        {
            int fd = ev[i].data.fd;

            if (ev[i].events && EPOLLIN)
            {
                std::cout<<"Recived event \n";
                pool.pushTask([this,fd]{this->handleClient(fd);});
            }
        }
    }
    std::cout<<"Epoll Thread pool Exit \n";
}
bool TCPServer::startSever()
{
    epollthread = std::thread(&TCPServer::epollThreadPool,this);
    sockaddr_in cliaddr;
    socklen_t cli_len = sizeof(cliaddr);
    std::cout<<"server listening at port "<<PORT<<" serverSocket "<<serverSocket<<std::endl;
    while(keep_running)
    {
        //std::cout<<"Waiting for a connection \n";
        if ((client_fd = accept(serverSocket,(struct sockaddr*)&cliaddr,&cli_len)) < 0)
        {
            //std::cout<<"Accept failed error "<<strerror(errno)<<" server socket "<<serverSocket<<std::endl;
            continue;
        }
        std::cout<<"New connection found \n";
        if (!setNonBlocking(client_fd))
        {
            std::cout<<"setNonBlcoking failed \n";
            return false;
        }
 
        struct epoll_event ev;
        ev.events = EPOLLIN | EPOLLET;
        ev.data.fd = client_fd;
        if ((epoll_ctl(epoll_fd,EPOLL_CTL_ADD,client_fd, &ev)) < 0)
        {
            std::cout<<"Epoll add failed \n";
            return false;
        }
    }
    closeSocket(client_fd);
    closeSocket(epoll_fd);
    return false;
}
bool TCPServer::setNonBlocking(int clientSocket)
{
    int flag = fcntl(clientSocket,F_GETFL,0);
    if (flag == -1)
    {
        perror("setNonBlocking");
        return false;
    }
    flag |= O_NONBLOCK;
    if (fcntl(clientSocket,F_SETFL,flag) < 0)
    {
        perror("setNonBlocking");
        return false;
    }
    std::cout<<"Socket set to non blocking \n";
    return true;
}
void TCPServer::sendhttpResponse(int fd)
{
    std::string http_response =
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: text/plain\r\n"
    "Content-Length: 13\r\n"
    "Connection: keep-alive\r\n"
    "\r\n"
    "Hello, world!";
    int sentByte = send(fd,http_response.c_str(),http_response.length(),0);

    if (sentByte < 0)
    {
        std::cout<<"Response sent failed "<<strerror(errno)<<std::endl;
        return;
    }
    std::cout<<"Sent data ";
    
}
TCPServer::~TCPServer()
{
   closeSocket(serverSocket);
   closeSocket(client_fd);
   closeSocket(epoll_fd);
   keep_running = false;
   if (epollthread.joinable())
   {
       epollthread.join();  
   }
}
void TCPServer::closeSocket(int socketFD)
{
    std::cout<<"Closing socket "<<socketFD<<std::endl;
    close(socketFD);
}
