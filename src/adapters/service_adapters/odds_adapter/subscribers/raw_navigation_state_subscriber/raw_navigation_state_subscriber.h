#pragma once

#include <atomic>
#include "../../../../../adapters/abstract_adapters/observable/observable.h"
#include "../../../../../adapters/interface_adapters/interface_receivers/interface_raw_navigation_state_receiver.h"
#include "../../../../../adapters/service_adapters/odds_adapter/subscribers/data_listener/data_listener.h"

class RawNavigationStateSubscriber : public Observable, public InterfaceRawNavigationStateReceiver {
    public:
        RawNavigationStateSubscriber(); 

        void start() override; 

        void stop();

        void add_observer(Observer* observer);

        void remove_observer(Observer* observer);

        Sentence get_sentence();

    private:
        std::atomic<bool> is_running_ = false;
        int sockfd_{};
        Sentence sentence_{};
        DataListener<Sentence> listener_;
        void process_message(const char* message);
};
