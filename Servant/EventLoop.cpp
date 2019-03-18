#include "EventLoop.h"

class IgnoreSigPipe
{
public:
    IgnoreSigPipe()
    {
        signal(SIGPIPE, SIG_IGN);
    }
};

IgnoreSigPipe initObj;

EventLoop::EventLoop()
    :isLooping(false),
     threadId(CurrentThread::gettid()),
     isQuit(false),
     e(new Epoll(this))
{ }

EventLoop::~EventLoop()
{
    assert(!isLooping);
}

void EventLoop::loop()
{
    assert(!isLooping);
    isLooping = true;
    isQuit = false;

    while(!isQuit)
    {
//        std::cout << "----------Looping----------" << std::endl;
        addToLoop();
        std::vector<Handler*> activeEvents;
        activeEvents.clear();
        e->epoll(activeEvents);
        for(std::vector<Handler*>::iterator iter = activeEvents.begin();
            iter != activeEvents.end(); ++iter)
        {
            std::cout << "----------Handle request----------" << std::endl;
            (*iter)->handle();
            e->removeFd((*iter)->connFd());
            delete *iter;
        }
    }
    isLooping = false;
}

void EventLoop::quit()
{
    isQuit = true;
}

void EventLoop::addToLoop(const int fd)
{
    // e->addToEpoll(fd);
    std::cout << "----------Add " << fd << " to loop----------" << std::endl;
    fds.push_back(fd);
}

void EventLoop::addToLoop()
{
    if(fds.empty())
    {
 //       std::cout << "----------fds empty----------" << std::endl;
        return;
    }
    for(int i = 0; i < fds.size(); ++i)
        e->addToEpoll(fds[i]);
    fds.clear();
    std::cout << "----------Add all fd to loop----------" << std::endl;
}
