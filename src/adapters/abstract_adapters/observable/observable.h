#pragma once
#include <list>
#include <thread>
#include "../../../handlers/observer/observer.h"

class Observable {
    public:
        void notify_observers();

        void sync_threads();

        void add_observer(Observer* observer);

        virtual void remove_observer(Observer* observer);

    private:
        std::list<Observer*> observers_;
        std::list<std::thread> threads_;
};
