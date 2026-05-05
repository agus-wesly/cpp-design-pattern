#pragma once

class InterfaceReceiver {
    public:
        virtual ~InterfaceReceiver() = default;

        virtual void start() = 0;
        virtual void stop() = 0;
};
