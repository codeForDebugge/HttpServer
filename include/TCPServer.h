#ifndef TCPSERVER_H
#define TCPSERVER_H
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <atomic>
#include <string.h>
#include <thread>
#include <fcntl.h>
#include <unordered_map>
#include <sys/epoll.h>
#include <ThreadPool.h>
#include <sstream>
#define PORT 12345
class TCPServer
{
    private:
    struct HttpRequest {
    std::string method;
    std::string path;
    std::string version;
    std::unordered_map<std::string, std::string> headers;
    std::string body;
    };

    int serverSocket;
    std::atomic<bool> keep_running;
    int client_fd;
    int epoll_fd;
    std::thread epollthread;

    public:
    TCPServer();
    ~TCPServer();

    HttpRequest parseHttpRequest(const std::string &raw);
    bool setNonBlocking(int fd);
    bool InitializeServer();
    bool startSever();
    void closeSocket(int socketFD);
    void epollThreadPool();
    void sendhttpResponse(int fd);
    void handleClient(int clientSocket);
};
#endif
