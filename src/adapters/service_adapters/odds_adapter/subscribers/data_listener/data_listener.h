#pragma once

#include "../../../../../utils/message_queue/message_queue.h"
#include "../../../../../utils/sentence_util/sentence_util.h"

template<typename MessageT>
class DataListener {
public:
    DataListener() = default;

    MessageQueue<MessageT>* get_queue_();
    void on_data_available(char* data);

private:
    MessageQueue<MessageT> queue_{};
};