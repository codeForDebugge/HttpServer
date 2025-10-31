#include <iostream>
#include <TCPServer.h>
using namespace std;

int main()
{
    TCPServer *tcpserver = new TCPServer();
    if (tcpserver->InitializeServer())
    {
        std::cout<<"Server initialized \n";
    }
    if (tcpserver->startSever())
    {
        std::cout<<"Server started \n";
    }
    return 1;
}
