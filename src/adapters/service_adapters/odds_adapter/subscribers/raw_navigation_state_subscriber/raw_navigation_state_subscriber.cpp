#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <unistd.h>
#include "raw_navigation_state_subscriber.h"
#include "../../../../../adapters/service_adapters/odds_adapter/subscribers/data_listener/data_listener.inl"

const char* IP = "127.0.0.1";
constexpr int PORT = 5000;

RawNavigationStateSubscriber::RawNavigationStateSubscriber()
{
    if (this->is_running_)
    {
        std::cerr << "UDP Listener already running" << std::endl;
        return;
    }

    this->sockfd_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (this->sockfd_ < 0)
    {
        std::cerr << "Failed to create socket" << std::endl;
        return;
    }

    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    setsockopt(this->sockfd_, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, IP, &server_addr.sin_addr) <= 0)
    {
        std::cerr << "Invalid address" << std::endl;
        close(this->sockfd_);
        this->sockfd_ = -1;
        return;
    }

    if (bind(this->sockfd_, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        std::cerr << "bind failed" << std::endl;
        close(this->sockfd_);
        this->sockfd_ = -1;
        return;
    }

    this->is_running_ = true;

    std::thread t([this]()
                  {
                struct sockaddr_in sender_addr{};
                socklen_t sender_len = sizeof(sender_addr);
                while (this->is_running_) {
                    char buff[1024];
                    auto received = recvfrom(
                            this->sockfd_, buff, sizeof(buff) - 1,
                            0, (struct sockaddr*)&sender_addr, &sender_len);

                    if (received < 0) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                            continue;
                        }

                        std::cerr << "Error receiving UDP packet: " << strerror(errno) << std::endl;
                        continue;
                    }

                    buff[received] = '\0';

                    this->listener_.on_data_available(buff);
                } });
    t.detach();
    std::cout << "Listening via UDP into : " << IP << ":" << PORT << std::endl;
}

void RawNavigationStateSubscriber::start()
{
    std::cout << "UDP Subscriber [Raw Navigation State ] Ready ..." << std::endl;

    Sentence buff{};
    while (this->listener_.get_queue_()->wait_and_pop(buff))
    {
        this->sync_threads();
        this->sentence_ = buff;
        this->notify_observers();
    }
}

void RawNavigationStateSubscriber::stop()
{
    this->is_running_ = false;
}

void RawNavigationStateSubscriber::add_observer(Observer *observer)
{
    Observable::add_observer(observer);
}

void RawNavigationStateSubscriber::remove_observer(Observer *observer)
{
    Observable::remove_observer(observer);
}

Sentence RawNavigationStateSubscriber::get_sentence()
{
    return this->sentence_;
}
