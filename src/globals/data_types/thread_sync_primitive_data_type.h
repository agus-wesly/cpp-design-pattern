#pragma once
#include <shared_mutex>

struct ThreadSyncPrimitive {
    std::shared_mutex navigation_state_mtx{};
};
