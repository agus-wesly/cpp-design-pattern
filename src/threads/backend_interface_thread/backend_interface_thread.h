#pragma once
#include "../../globals/data_types/app_state_data_type.h"
#include "../../adapters/interface_adapters/interface_receivers/interface_raw_navigation_state_receiver.h"
#include "../../globals/data_types/thread_sync_primitive_data_type.h"

class BackendInterfaceThreadsContainer {
    public:
        static void raw_navigation_state_receiver_thread(
            InterfaceRawNavigationStateReceiver *raw_navigation_state_receiver,
            AppState &app_state,
            ThreadSyncPrimitive &thread_sync_primitive
        ); 
};