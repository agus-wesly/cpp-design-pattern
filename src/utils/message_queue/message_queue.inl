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
 Name        : message_queue.cpp
 Author      : Muhammad Hutomo Padmanaba
 Version     : 0.1.0 27/12/2025
 Description : Utility for message queue
 =================================================================================================================
*/

#include "message_queue.h"

template<typename T>
void MessageQueue<T>::push(const T& value) {
    {
        std::lock_guard<std::mutex> lock(mtx_);
        q_.push(value);
    }
    cv_.notify_one();
}

template<typename T>
void MessageQueue<T>::push(T&& value) {
    {
        std::lock_guard<std::mutex> lock(mtx_);
        q_.push(std::move(value));
    }
    cv_.notify_one();
}

template<typename T>
bool MessageQueue<T>::wait_and_pop(T& value) {
    std::unique_lock<std::mutex> lock(mtx_);
    cv_.wait(lock, [&] { return stop_ || !q_.empty(); });

    if (stop_ && q_.empty())
        return false;

    value = std::move(q_.front());
    q_.pop();
    return true;
}

template<typename T>
void MessageQueue<T>::stop() {
    {
        std::lock_guard<std::mutex> lock(mtx_);
        stop_ = true;
    }
    cv_.notify_all();
}
