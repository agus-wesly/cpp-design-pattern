#include "data_listener.h"

#include "../../../../../utils/message_queue/message_queue.inl"
#include "../../../../../utils/sentence_util/sentence_util.h"

template<typename MessageT>
MessageQueue<MessageT>* DataListener<MessageT>::get_queue_() {
    return &this->queue_;
}

template<typename MessageT>
void DataListener<MessageT>::on_data_available(char* data) {
    Sentence parsed = SentenceUtil::parse_from_char(data);
    this->queue_.push(std::move(parsed));
}