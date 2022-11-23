#ifndef CLIENT_SERVICE_IMPL_H
#define CLIENT_SERVICE_IMPL_H

#include <chrono>
#include <memory>
#include <mutex>
#include <grpcpp/grpcpp.h>
#include <cstdlib>

#include "ServerStruct.h"

#include "proto/client.grpc.pb.h"

/**
 * @brief Client service implementation.
 * 
 */
class ClientServiceImpl final : public messages::Client::Service {

private:
    std::shared_ptr<ServerStruct> _server;

public:
    ClientServiceImpl(std::shared_ptr<ServerStruct> server) : _server(server) {}

    ~ClientServiceImpl() {
        _server = nullptr;
    }

    /**
     * @brief Queries for the server datacenter.
     *
     * @param context
     * @param request
     * @param reply
     * @return grpc::Status
     */
    virtual grpc::Status execute(grpc::ServerContext *context, const messages::TransactionRequest *request,
                                    messages::TransactionReply *reply) override {
        std::cout << "-> Received transaction request\n" << std::endl;

        // Deliver the message
        _server->insertLog(request->clientid(), request->messageid());

        reply->set_code(messages::ReplyCode::OK);
        return grpc::Status::OK;
    }

    /**
     * @brief Queries for the server datacenter.
     *
     * @param context
     * @param request
     * @param reply
     * @return grpc::Status
     */
    virtual grpc::Status datacenter(grpc::ServerContext *context, const messages::DatacenterRequest *request,
                               messages::DatacenterReply *reply) override {
        std::cout << "-> Received datacenter request\n" << std::endl;

        std::string datacenter = "";
        if (std::getenv("MY_NODE_NAME")) {
            datacenter = std::string(std::getenv("MY_NODE_NAME"));
        }

        reply->set_datacenter(datacenter);
        return grpc::Status::OK;
    }

    virtual grpc::Status log(grpc::ServerContext *context, const messages::LogRequest *request,
            messages::LogReply *reply) override {
        std::cout << "-> Received log request\n" << std::endl;

        // Filling reply with the log information
        std::shared_lock <std::shared_mutex> lock(*(_server->logMutex()));
        for (std::string msg: _server->log()) {
            reply->add_log(msg);
        }
        lock.unlock();

        // Warn about being the sequencer or not
        if (this->_server->seq() == this->_server->host()) {
            reply->set_sequencer(true);

        } else {
            reply->set_sequencer(false);
        }

        reply->set_address(_server->address());
        reply->set_code(messages::ReplyCode::OK);
        return grpc::Status::OK;
    }

};

#endif