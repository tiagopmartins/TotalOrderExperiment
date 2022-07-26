#ifndef CLIENT_SERVICE_IMPL_H
#define CLIENT_SERVICE_IMPL_H

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

        int i = 0;
        // Sending messages to the other servers
        while (i < request->msgn()) {
            messages::MessageRequest msgRequest;
            messages::MessageReply msgReply;

            msgRequest.set_address(_server->host() + ":" + _server->port());
            msgRequest.set_id(_server->msgCounter());

            // Deliver the message to itself
            std::mutex logMutex;
            logMutex.lock();
            _server->insertLog(_server->host() + ":" + _server->port(), _server->msgCounter());
            logMutex.unlock();

            _server->incrementMsgCounter();

            for (std::string address : _server->servers()) {
                // Dont send a message to itself
                if (address == (_server->host() + ":" + _server->port())) {
                    continue;
                }

                _server->sendMessage(address, msgRequest, &msgReply);
            }

            i++;
        }

        reply->set_code(messages::ReplyCode::OK);
        return grpc::Status::OK;
    }

    virtual grpc::Status log(grpc::ServerContext *context, const messages::LogRequest *request,
            messages::LogReply *reply) override {
        std::cout << "-> Received log request\n" << std::endl;

        // Filling reply with the log information
        std::mutex logMutex;
        logMutex.lock();
        for (std::string msg : _server->log()) {
            reply->add_log(msg);
        }
        logMutex.unlock();

        reply->set_address(_server->host() + ":" + _server->port());
        reply->set_code(messages::ReplyCode::OK);
        return grpc::Status::OK;
    }

};

#endif