#include <iostream>
#include <memory>
#include <thread>
#include <chrono>

#include <grpcpp/grpcpp.h>
#include "generated/navigation_service.grpc.pb.h"

class NavigationServiceImpl final
    : public NavigationService::NavigationService::Service {

public:
    grpc::Status StreamNavigationData(
      grpc::ServerContext* context,
      const StreamRequest* request,
      grpc::ServerWriter<NavigationStateForClient>* writer) override {

    std::cout << "Client connected\n";

    while (!context->IsCancelled()) {

      NavigationStateForClient msg;

      auto* lat = msg.mutable_latitude_dd();
      lat->set_value(1.234);
      lat->set_is_valid(true);

      auto* lon = msg.mutable_longitude_dd();
      lon->set_value(5.678);
      lon->set_is_valid(true);

      if (!writer->Write(msg)) {
        break; // client disconnected
      }

      std::this_thread::sleep_for(std::chrono::seconds(3));
    }

    std::cout << "Client disconnected\n";
    return grpc::Status::OK;
  }
};

int main() {
  std::string server_address("0.0.0.0:50051");

  NavigationServiceImpl service;

  grpc::ServerBuilder builder;
  builder.AddListeningPort(
      server_address,
      grpc::InsecureServerCredentials());

  builder.RegisterService(&service);

  std::unique_ptr<grpc::Server> server(builder.BuildAndStart());

  std::cout << "Server listening on "
            << server_address << std::endl;

  server->Wait();
}
