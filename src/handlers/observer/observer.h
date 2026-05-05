#pragma once

class Observer {
    public:
        virtual ~Observer() = default;
        virtual void update_data() = 0;
};
