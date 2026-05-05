#pragma once

#include "../../../globals/data_types/sentence_data_types/sentence_data_type.h"
#include "interface_receiver.h"
#include "../../../handlers/observer/observer.h"

class InterfaceRawNavigationStateReceiver : public InterfaceReceiver {
    public:
        ~InterfaceRawNavigationStateReceiver() override = default;

        virtual Sentence get_sentence() = 0;

        virtual void start() = 0;

        virtual void stop() = 0;

        virtual void add_observer(Observer *observer) = 0;
        
        virtual void remove_observer(Observer *observer) = 0;
};
