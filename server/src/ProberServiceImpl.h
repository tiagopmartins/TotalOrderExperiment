#ifndef PROBER_SERVICE_IMPL_H
#define PROBER_SERVICE_IMPL_H

#include <ctime>
#include <grpcpp/grpcpp.h>
#include <google/protobuf/util/time_util.h>

#include "proto/prober.grpc.pb.h"

using namespace google::protobuf;

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
        std::cout << "-> Received probing request\n" << std::endl;
        reply->set_code(messages::ReplyCode::OK);
        return grpc::Status::OK;
    }

};

#endif