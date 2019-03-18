#include <iostream>
#include <cstdlib>
#include "smtp.h"
#include "Socket.h"
#include "EventLoop.h"
#include "EventLoopThread.h"
#include "EventLoopThreadPool.h"

void * run_smtp_server(void * args)
{
    SMTP::get_instance() -> run();
}

int main(int argc, char **argv)
{
    if(argc != 2)
    {
        std::cout << "Usage: " << argv[0] << " <port>" << std::endl;
        return 0;
    }
    int port = std::atoi(argv[1]);

    
    
    int listenFd = Socket::createSocket();
    Socket::setReuseAddr(listenFd, true);
    struct sockaddr_in servAddr;
    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr.sin_port = htons(port);
    Socket::Bind(listenFd, servAddr);
    Socket::Listen(listenFd);
// run smtp_server
    pthread_t smtp_id;
    pthread_create(&smtp_id,NULL,run_smtp_server,NULL);

    EventLoopThreadPool *threadPool = new EventLoopThreadPool(4);
    while(true)
    {
        struct sockaddr_in clientAddr;
        socklen_t clientAddrLen = sizeof(clientAddr);
        memset(&clientAddr, 0, sizeof(clientAddr));
        int connFd = Socket::Accept(listenFd, &clientAddr);

        EventLoopThread *thread = threadPool->getNextThread();
        EventLoop *loop = thread->getLoop();
        loop->addToLoop(connFd);
    }
    pthread_join(smtp_id,NULL);
    Socket::Close(listenFd);
    delete threadPool;
    return 0;
}
