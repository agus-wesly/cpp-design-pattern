#include "observable.h"
void Observable::notify_observers()
{
    for (auto &observer : this->observers_)
    {
        this->threads_.emplace_back([observer](){ observer->update_data(); });
    }
}

void Observable::sync_threads()
{
    for (auto &thread : this->threads_)
    {
        thread.join();
    }
    this->threads_.clear();
}

void Observable::add_observer(Observer *observer)
{
    this->observers_.push_back(observer);
}

void Observable::remove_observer(Observer *observer)
{
    this->observers_.remove(observer);
}