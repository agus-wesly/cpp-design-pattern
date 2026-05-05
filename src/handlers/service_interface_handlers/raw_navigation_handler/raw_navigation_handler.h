#pragma once

#include "../../observer/observer.h"
#include "../../../adapters/interface_adapters/interface_receivers/interface_raw_navigation_state_receiver.h"
#include "../../../globals/data_types/app_state_data_type.h"
#include "../../../globals/data_types/app_state_data_type.h"
#include "../../../globals/data_types/thread_sync_primitive_data_type.h"

class RawNavigationHandler : public Observer {
    public:
        RawNavigationHandler (
            InterfaceRawNavigationStateReceiver *observable,
            AppState &app_state,
            ThreadSyncPrimitive &thread_sync_primitive
        );

        void update_data();

    private:
        InterfaceRawNavigationStateReceiver* observable_{};
        AppState &app_state;
        ThreadSyncPrimitive &thread_sync_primitive;
};
