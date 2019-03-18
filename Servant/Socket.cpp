#include "Socket.h"

void Socket::setReuseAddr(const int fd, bool on)
{
    int optval = on ? 1 : 0;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR,
                     &optval, sizeof(optval));
}

void Socket::setTcpNoDelay(const int fd, bool on)
{
    int optval = on ? 1 : 0;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(optval));
}

int Socket::createSocket()
{
    int sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(sockfd < 0)
    {
        std::cout << "Socket::createNonblock error: " << strerror(errno) << std::endl;
        exit(1);
    }
    // setNonBlockAndCloseOnExec(sockfd);
    return sockfd;  
}

void Socket::Bind(const int sockfd, const struct sockaddr_in &addr)
{
    int ret = bind(sockfd, (struct sockaddr*)(&addr), sizeof(addr));
    if(ret < 0) 
    {
        std::cout << "Socket::bind error: " << strerror(errno) << std::endl;
        exit(1);
    }
}

void Socket::Listen(const int sockfd)
{
    if(listen(sockfd, 5) < 0)
    {
        std::cout << "Socket::listen error: " << strerror(errno) << std::endl;
        exit(1);
    }
}

int Socket::Accept(const int sockfd, struct sockaddr_in *addr)
{
    socklen_t addrLen = sizeof(*addr);
    // int connfd = accept(sockfd, (struct sockaddr*)&addr, &addrLen);
    // setNonBlockAndCloseOnExec(connfd);
    int connfd = accept4(sockfd, (struct sockaddr*)&addr, 
                         &addrLen, SOCK_NONBLOCK | SOCK_CLOEXEC);

    if(connfd < 0)
    {
        switch(errno)
        {
            case EAGAIN:
            case ECONNABORTED:
            case EINTR:
            case EMFILE:
                break;

            case EFAULT:
            case EINVAL:
            case ENFILE:
            case ENOMEM:
                std::cout << "Socket::accept error: " << strerror(errno) << std::endl;
                exit(1);
                break;

            default:
                std::cout << "Socket::accept error: " << strerror(errno) << std::endl;
                exit(1);
                break;
        }
    }
    return connfd;
}

void Socket::Close(const int sockfd)
{
    if(close(sockfd) < 0)
    {
        std::cout << "Socket::close error: " << strerror(errno) << std::endl;
        exit(1);
    }
}

void Socket::setNonBlockAndCloseOnExec(const int sockfd)
{
}