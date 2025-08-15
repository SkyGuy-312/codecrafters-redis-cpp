#include "event_loop.h"
#include <iostream>
#include <stdexcept>
#include <fcntl.h>
#include <unistd.h>
#include <algorithm>

EventLoop::EventLoop():
epoll_fd(epoll_create1(0)),
running(false)
{
    if (this->epoll_fd < 0)
    {
        std::cerr << "Failed to create epoll instance\n";
        switch (errno)
        {
        case EMFILE:
            throw std::runtime_error("epoll fd creation failed: too many open file descriptors! program resource limit reachaed.");
            break;
        case ENFILE:
            throw std::runtime_error("epoll fd creation failed: system-wide resource limit for file descriptor reached.");
            break;
        case EINVAL:
            throw std::runtime_error("Invalid flag argument..");
            break;
        }
    }

    // piping stop_fd so that it can be monitored for its value and be stopped upon need
    if(pipe(this->stop_fd) < 0) throw std::runtime_error ("Pipe failed for stop fd");

    // Set read end non-blocking
    int flags = fcntl(this->stop_fd[0], F_GETFL, 0);
    fcntl(this->stop_fd[0], F_SETFL, flags | O_NONBLOCK);

    // add the stop fd to epoll events
    epoll_event stop_event{};
    stop_event.data.fd = this->stop_fd[0];
    stop_event.events = EPOLLIN;
    if (epoll_ctl(this->epoll_fd, EPOLL_CTL_ADD, this->stop_fd[0], &stop_event) < 0)
    {
        switch (errno)
        {
        case EBADF:
            std::cerr << "stop signal epoll_ctl error: invalid file descriptor" << std::endl;
            close(this->epoll_fd);
            break;
        case EEXIST:
            std::cerr << "stop signal epoll_ctl error: stop signal FD already being monitored" << std::endl;
            break;
        case ENOMEM:
            std::cerr << "stop signal epoll_ctl error: kernel out of memory" << std::endl;
            close(this->epoll_fd);
            break;
        default:
            perror("stop signal epoll_ctl failed with unexpected error");
            close(this->epoll_fd);
            break;
        }
    }
}

EventLoop::~EventLoop()
{
    // close stop_fd
    if (epoll_ctl(this->epoll_fd, EPOLL_CTL_DEL, this->stop_fd[0], nullptr) < 0)
    {
        perror("stop signal epoll_ctl failed with unexpected error");
    }
    close (this->stop_fd[0]);
    close (this->stop_fd[1]);
    
    //close epoll_fd
    close(this->epoll_fd);
}

void EventLoop::run(){
    this->running = true;
    constexpr unsigned char TRIGGERED_EVENTS_BUFFER_SIZE = 20;
    this->events.resize(TRIGGERED_EVENTS_BUFFER_SIZE);
    while (true){
        int triggered_events_count = epoll_wait(this->epoll_fd, events.data(), TRIGGERED_EVENTS_BUFFER_SIZE, -1);
        if (triggered_events_count < 1){
            std::cerr << "error occurred\n";
            break;
        }
        for (int i{0}; i < triggered_events_count; i++){
            const int &fd = events[i].data.fd;
            if (fd == this->stop_fd[0]){
                // drain the pipe
                char buf[100];
                while (read(fd, buf, sizeof(buf) >0));  // keep draining the pipe
                running = false;
                break;
            }
            
            auto it = this->handlers.find(fd);
            if (it != handlers.end()){
                it->second->handleEvent(fd, events[i].events);
            }

        }
    }
}

void EventLoop::stop(){
    char c  = 'S';
    if(write(stop_fd[1], &c, sizeof(c)) < 0) perror("write to stop_fd failed");
}

bool EventLoop::addFd(int fd, uint32_t events, IEventHandler* handler)
{
    epoll_event fd_event{};
    fd_event.data.fd = fd;
    fd_event.events = events;
    this->handlers[fd] = handler;
    if (epoll_ctl(this->epoll_fd, EPOLL_CTL_ADD, fd, &fd_event) < 0)
    {
        switch (errno)
        {
        case EBADF:
            std::cerr << "stop signal epoll_ctl error: invalid file descriptor" << std::endl;
            break;
        case EEXIST:
            std::cerr << "stop signal epoll_ctl error: stop signal FD already being monitored" << std::endl;
            break;
        case ENOMEM:
            std::cerr << "stop signal epoll_ctl error: kernel out of memory" << std::endl;
            break;
        default:
            perror("stop signal epoll_ctl failed with unexpected error");
            break;
        }
        return false;
    }
    return true;
}

bool EventLoop::modFd(int fd, uint32_t events)
{
    epoll_event fd_event{};
    fd_event.data.fd = fd;
    fd_event.events = events;
    if (epoll_ctl(this->epoll_fd, EPOLL_CTL_MOD, fd, &fd_event) < 0)
    {
        switch (errno)
        {
        case EBADF:
            std::cerr << "stop signal epoll_ctl error: invalid file descriptor" << std::endl;
            break;
        case EEXIST:
            std::cerr << "stop signal epoll_ctl error: stop signal FD already being monitored" << std::endl;
            break;
        case ENOMEM:
            std::cerr << "stop signal epoll_ctl error: kernel out of memory" << std::endl;
            break;
        default:
            perror("stop signal epoll_ctl failed with unexpected error");
            break;
        }
        return false;
    }
    return true;
}

bool EventLoop::delFd(int fd)
{
    this->handlers.erase(fd);
    if (epoll_ctl(this->epoll_fd, EPOLL_CTL_DEL, fd, nullptr) < 0)
    {
        perror("stop signal epoll_ctl failed with unexpected error");
    }
    return true;
}
