#ifndef MESSAGE_SERVICE_IMPL_H
#define MESSAGE_SERVICE_IMPL_H

#include <memory>
#include <shared_mutex>

#include "ServerStruct.h"

#include "proto/messages.grpc.pb.h"

/**
 * @brief Message service implementation.
 * 
 */
class MessageServiceImpl final : public messages::Messenger::Service {

private:
    std::shared_ptr<ServerStruct> _server;

public:
    MessageServiceImpl(std::shared_ptr<ServerStruct> server) : _server(server) {}

    ~MessageServiceImpl() {
        _server = nullptr;
    }

    /**
     * @brief Receive a message from another server.
     * 
     * @param context 
     * @param request 
     * @param reply 
     * @return grpc::Status
     */
    virtual grpc::Status send(grpc::ServerContext *context, const messages::MessageRequest *request,
            messages::MessageReply *reply) override {
        std::cout << "-> Receiving message from " << request->address()
            << " with message ID " << request->id() << '\n' << std::endl;

        _server->insertLog(request->address(), request->id());

        reply->set_code(messages::ReplyCode::OK);
        return grpc::Status::OK;
    }

    virtual grpc::Status sendSeqNumber(grpc::ServerContext *context, const messages::SeqNumberRequest *request,
            messages::SeqNumberReply *reply) override {
        std::cout << "-> Receiving sequence number" << " (message ID: "
            << request->msgid() << ")\n" << std::endl;

        // TODO

        reply->set_code(messages::ReplyCode::OK);
        return grpc::Status::OK;
    }

};

#endif