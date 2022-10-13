#ifndef PROBER_SERVICE_IMPL_H
#define PROBER_SERVICE_IMPL_H

#include <chrono>
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

        std::chrono::time_point now = std::chrono::system_clock::now();
        std::chrono::time_point nowMS = std::chrono::time_point_cast<std::chrono::microseconds>(now);
        auto currentTime = std::chrono::duration_cast<std::chrono::microseconds>(nowMS.time_since_epoch());
        reply->set_arrival(currentTime.count());

        reply->set_code(messages::ReplyCode::OK);
        return grpc::Status::OK;
    }

};

#endif