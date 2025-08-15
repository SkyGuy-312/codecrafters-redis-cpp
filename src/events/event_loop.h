#pragma once
#include "../handler/event_handler.h"
#include <unordered_map>
#include <vector>
#include <sys/epoll.h>
#include <memory>

/**
 * @class EventLoop
 * @brief generic event loop using linux epoll
 * 
 * Encapsulates the epol mechanism to monitor multiple file descriptors for IO events.
 * Uses IEvenetHandler observer mechanism to decouple event detedtion from event handling.
 * This class is generic and has no knowledge of any other components of the redis server
 */
class EventLoop{
    int epoll_fd;   // file descriptor for the epoll instance
    int stop_fd[2];    // special file descriptor to signal the loop to stop
    bool running;   // flag to control the main loop run()

    std::unordered_map<int, IEventHandler*> handlers; // map of pointers to handler objects
    std::vector<epoll_event> events;    //buffer to store the events returned by epoll_wait

    // making the EventLoop object non copyable
    EventLoop(const EventLoop&) = delete;
    EventLoop & operator=(const EventLoop&) = delete;

public:

    /**
     * @brief Constructor for EventLoop and init the epoll instance.
     * @throws std::runtime error on failure
     */
    EventLoop();

    /**
     * @brief Destructor. Cleans up resources by closing file descriptors
     */
    ~EventLoop();

    /**
     * @brief Starts the event loop.
     * This is blocking call that will continuously wait for events and dispatch them 
     * to the registered handlers until stop() is called 
     */
    void run();

    /**
     * @brief Stops the event loop.
     */
    void stop();

    /**
     * @brief Registers a file descriptor and its handler with the event loop.
     * @param fd The file descriptor to monitor
     * @param events The initial set of events to watch for (e.g, EPOLLIN).
     * @param handler The callback function to execute when an event occurs on @param fd.
     * @return True on success, false on failure
     */
    bool addFd(int fd ,uint32_t events, IEventHandler* handler);

    /**
     * @brief Modifies the set of events to watch for an already registered fd.
     * @param fd The file descriptor to modify.
     * @param events the new set of events to watch for.
     * @return True on success, false on failure.
     */
    bool modFd(int fd, uint32_t events);

    /**
     * @brief Unregisters a file descriptor from the event loop.
     * @param fd the file descriptor to remove.
     * @return True on success, false on failure.
     */
    bool delFd(int fd);
};