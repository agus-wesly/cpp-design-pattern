#include "raw_navigation_handler.h"

RawNavigationHandler::RawNavigationHandler (
    InterfaceRawNavigationStateReceiver *observable,
    AppState &app_state,
    ThreadSyncPrimitive &thread_sync_primitive
) : observable_(observable), app_state_(app_state), 
thread_sync_primitive_(thread_sync_primitive) {}

void RawNavigationHandler::update_data() {
    std::lock_guard(this->thread_sync_primitive_.navigation_state_mtx);
    Sentence new_data = this->observable_->get_sentence();
    this->app_state_.navigation_state.update(new_data);
}