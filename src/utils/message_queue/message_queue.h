/*
 * Copyright PT LEN INNOVATION TECHNOLOGY
 *
 * THIS SOFTWARE SOURCE CODE AND ANY EXECUTABLE DERIVED THEREOF ARE PROPRIETARY
 * TO PT LEN INNOVATION TECHNOLOGY, AS APPLICABLE, AND SHALL NOT BE USED IN ANY WAY
 * OTHER THAN BEFOREHAND AGREED ON BY PT LEN INNOVATION TECHNOLOGY, NOR BE REPRODUCED
 * OR DISCLOSED TO THIRD PARTIES WITHOUT PRIOR WRITTEN AUTHORIZATION BY
 * PT LEN INNOVATION TECHNOLOGY, AS APPLICABLE.
 */

/*
 =================================================================================================================
 Name        : message_queue.h
 Author      : Muhammad Hutomo Padmanaba
 Version     : 0.1.0 27/12/2025
 Description : Utility for message queue
 =================================================================================================================
*/

#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>

template<typename T>
class MessageQueue {
public:
    MessageQueue() = default;
    ~MessageQueue() = default;

    // Non-copyable
    MessageQueue(const MessageQueue&) = delete;
    MessageQueue& operator=(const MessageQueue&) = delete;

    /**
     * Method to push message by copying
     * @param value message to be pushed 
     */
    void push(const T& value);
    /**
     * Method to push message by moving ownership
     * @param value message to be pushed 
     */
    void push(T&& value);

    /**
     * Method to check for message
     * @param value message to be popped 
     */
    bool wait_and_pop(T& value);
    /**
     * Method to stop checking queue
     */
    void stop();

private:
    std::queue<T> q_;
    std::mutex mtx_;
    std::condition_variable cv_;
    bool stop_ = false;
};
