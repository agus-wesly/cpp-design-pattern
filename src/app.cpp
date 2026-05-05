#include "globals/data_types/app_state_data_type.h"
#include "adapters/service_adapters/odds_adapter/subscribers/raw_navigation_state_subscriber/raw_navigation_state_subscriber.h"
#include "globals/data_types/thread_sync_primitive_data_type.h"
#include "handlers/service_interface_handlers/raw_navigation_handler/raw_navigation_handler.h"
#include "threads/backend_interface_thread/backend_interface_thread.h"

int main() {
    AppState app_state{};

    ThreadSyncPrimitive thread_sync_primitive{};
    RawNavigationStateSubscriber raw_navigation_state_subscriber{};

    std::thread raw_navigation_state_subscriber_thread
    (
        BackendInterfaceThreadsContainer::raw_navigation_state_receiver_thread,
        &raw_navigation_state_subscriber,
        std::ref(app_state),
        std::ref(thread_sync_primitive)
    );

    raw_navigation_state_subscriber_thread.join(); 

    return EXIT_SUCCESS;
}