#ifndef CLIENT_SERVICE_IMPL_H
#define CLIENT_SERVICE_IMPL_H

#include <chrono>
#include <memory>
#include <mutex>
#include <grpcpp/grpcpp.h>

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

    virtual grpc::Status datacenter(grpc::ServerContext *context, const messages::DatacenterRequest *request,
                               messages::DatacenterReply *reply) override {
        std::cout << "-> Received datacenter request\n" << std::endl;

        std::cout << "NO STRING: " << getenv("MY_NODE_NAME") << std::endl;
        std::cout << "STRING: " << std::string(getenv("MY_NODE_NAME")) << std::endl;

        std::string datacenter = "";
        if (getenv("MY_NODE_NAME")) {
            datacenter = std::string(getenv("MY_NODE_NAME"));
        }

        reply->set_datacenter(datacenter);
        return grpc::Status::OK;
    }

    /**
     * @brief Activates the server. Starts the message exchange.
     * 
     * @param context 
     * @param request 
     * @param reply 
     * @return grpc::Status 
     */
    virtual grpc::Status begin(grpc::ServerContext *context, const messages::BeginRequest *request,
            messages::BeginReply *reply) override {
        std::cout << "-> Received begin signal\n" << std::endl;

        std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
        // Sending messages to the other servers
        while (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - start).count() < request->duration()) {
            messages::MessageRequest msgRequest;
            messages::MessageReply msgReply;

            std::shared_lock<std::shared_mutex> lock(*(_server->msgCounterMutex()));
            msgRequest.set_id(_server->id());
            msgRequest.set_msgid(_server->msgCounter());

            // Deliver the message to itself first
            _server->insertLog(_server->id(), _server->msgCounter());
            lock.unlock();

            _server->incrementMsgCounter();

            for (auto const &[id, ip] : _server->servers()) {
                // Dont send a message to itself
                if ((ip + ":" + _server->port()) == (_server->address())) {
                    continue;
                }

                _server->sendMessage(ip + ":" + _server->port(), msgRequest, &msgReply);
            }
        }

        reply->set_code(messages::ReplyCode::OK);
        return grpc::Status::OK;
    }

    virtual grpc::Status log(grpc::ServerContext *context, const messages::LogRequest *request,
            messages::LogReply *reply) override {
        std::cout << "-> Received log request\n" << std::endl;

        // Filling reply with the log information
        std::shared_lock<std::shared_mutex> lock(*(_server->logMutex()));
        for (std::string msg : _server->log()) {
            reply->add_log(msg);
        }
        lock.unlock();

        reply->set_address(_server->address());
        reply->set_code(messages::ReplyCode::OK);
        return grpc::Status::OK;
    }

};

#endif