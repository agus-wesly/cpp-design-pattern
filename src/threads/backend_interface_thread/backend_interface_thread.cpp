#include "backend_interface_thread.h"
#include "../../handlers/service_interface_handlers/raw_navigation_handler/raw_navigation_handler.h"

void BackendInterfaceThreadsContainer::raw_navigation_state_receiver_thread (
            InterfaceRawNavigationStateReceiver *raw_navigation_state_receiver,
            AppState &app_state,
            ThreadSyncPrimitive &thread_sync_primitive
        ) 
        {
            RawNavigationHandler raw_navigation_handler(
                raw_navigation_state_receiver,
                app_state,
                thread_sync_primitive
            );
            raw_navigation_state_receiver->add_observer(&raw_navigation_handler);
            raw_navigation_state_receiver->start();
        }
