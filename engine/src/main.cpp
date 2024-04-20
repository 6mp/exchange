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

    auto orderFillCallback = [](const Order& placedOrder, const Order& restingOrder) {
        std::cout << "order id " << placedOrder.getId() << " filled resting order id " << restingOrder.getId() << "\n";
    };

    auto orderQueudCallback = [](const Order& queuedOrder) {
        std::cout << "queued order with id " << queuedOrder.getId() << "\n";
    };

    auto orderKillCallback = [](const Order& killedOrder) {
        std::cout << "killed order with id " << killedOrder.getId() << "\n";
    };

    auto orderAddToBookCallback = [](const Order& addedOrder) {
        std::cout << "added order to book with id " << addedOrder.getId() << "\n";
    };

    static Orderbook book{orderQueudCallback, orderFillCallback, orderAddToBookCallback, orderKillCallback};

    std::atexit(+[] { book.requestStop(); });



    // add limit to book
    const auto test = Order{1, 10.0, 1, OrderSide::BUY};
    book.addOrder(test);

    const auto test2 = Order{2, 11.0, 1, OrderSide::BUY};
    book.addOrder(test2);

    const auto test3 = Order{3, 12.0, 1, OrderSide::BUY};
    book.addOrder(test3);

    /*
    // add market to book that should fill the limit
    const auto test2 = Order{2, OrderSide::SELL, 1};
    book.addOrder(test2);
    */

    // add another limit on the other side that will fill the resting order
    const auto test4 = Order{4, 9.0, 1, OrderSide::SELL};
    book.addOrder(test4);

    // fill it
    const auto test5 = Order{5, OrderSide::SELL, 1};
    book.addOrder(test5);

    // spin
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}
