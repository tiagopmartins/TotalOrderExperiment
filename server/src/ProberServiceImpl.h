#ifndef PROBER_SERVICE_IMPL_H
#define PROBER_SERVICE_IMPL_H

#include <ctime>
#include <grpcpp/grpcpp.h>

#include "proto/prober.grpc.pb.h"

/**
 * @brief Prober service implementation.
 * 
 */
class ProberServiceImpl final : public messages::Prober::Service {

public:
    ProberServiceImpl() {}

    ~ProberServiceImpl() {}

    /**
     * @brief Receive a probing request to test the network.
     * 
     * @param context 
     * @param request 
     * @param reply 
     * @return grpc::Status
     */
    virtual grpc::Status probing(grpc::ServerContext *context, const messages::ProbingRequest *request,
            messages::ProbingReply *reply) override {
        std::cout << "-> Received probing request" << std::endl;

        std::time_t currentTime = std::time(nullptr) * 1000;
        reply->set_currenttime(currentTime);

        reply->set_code(messages::ReplyCode::OK);
        return grpc::Status::OK;
    }

};

#endif