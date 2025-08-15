#pragma once

class IEventHandler
{
public:
    virtual ~IEventHandler() = default;
    
    /**
     * @brief The callback method invoked by the EventLoop when an event occurs.
     * @param fd The file descriptor that triggered the event.
     * @param events A bitmask of the epoll events that occurred.
     */
    virtual void handleEvent(int fd, unsigned int events) = 0;
};
