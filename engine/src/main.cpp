// orderbook shit, matching engine shit, then basic interface to put orders in
// grpc service
// python thing or grafana thing to visualize which pulls from grpc

#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include <protos/Orderbook.grpc.pb.h>
#include <protos/Orderbook.pb.h>

#include <Orderbook/Orderbook.hpp>

class GreeterServiceImpl final : public Greeter::Service {
    grpc::Status SayHello(grpc::ServerContext* context, const HelloRequest* request, HelloReply* reply) override {
        std::string prefix("Hello ");
        reply->set_message(prefix + request->name());
        return grpc::Status::OK;
    }
};

int main() {
    /*  std::string server_address = "127.0.0.1:8080";
      GreeterServiceImpl service;

      grpc::EnableDefaultHealthCheckService(true);
      grpc::ServerBuilder builder;
      // Listen on the given address without any authentication mechanism.
      builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
      // Register "service" as the instance through which we'll communicate with
      // clients. In this case it corresponds to an *synchronous* service.
      builder.RegisterService(&service);
      // Finally assemble the server.
      std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
      std::cout << "Server listening on " << server_address << std::endl;

      // Wait for the server to shutdown. Note that some other thread must be
      // responsible for shutting down the server for this call to ever return.
      server->Wait();*/

    auto orderFillCallback = [](Order& placedOrder, Order& restingOrder) {
        std::cout << "filled " << placedOrder.getFilledQuantity() << " \n";
    };

    auto orderAddCallback = [](Order& placedOrder) {
        std::cout << "placed order with id " << placedOrder.getId() << "\n";
    };

    static Orderbook book{orderAddCallback, orderFillCallback};

    std::atexit(+[] { book.requestStop(); });

    while (true) {}
}
