#ifndef CLIENT_H
#define CLIENT_H

#include <string>
#include <vector>
#include <memory>
#include <grpcpp/grpcpp.h>

#include "proto/client.grpc.pb.h"

/**
 * @brief Client representation.
 * 
 */
class Client {

private:
    // Container for information and state
    struct AsyncClientCall {
        std::string address;
        messages::BeginReply reply;
        grpc::ClientContext context;
        grpc::Status status;

        std::unique_ptr<grpc::ClientAsyncResponseReader<messages::BeginReply>> responseReader;
    };

    const std::vector<std::string> SERVERS = {"localhost:50001", "localhost:50002"};   // Server list for testing purposes
    
    std::unique_ptr<messages::Client::Stub> _stub;
    grpc::CompletionQueue _cq;                          // Producer-consumer queue for asynchronous communication

public:
    Client() {}

    Client(std::shared_ptr<grpc::Channel> channel) : _stub(messages::Client::NewStub(channel)) {}

    ~Client() {}

    std::vector<std::string> servers() {
        return this->SERVERS;
    }

    void printLog(messages::LogReply *reply);

    void createStub(std::string address);

    void sendMessage(std::string address, messages::BeginRequest *request);

    /**
     * @brief Begin the message exchange between servers.
     * 
     * @param msgN Number of messages the servers are going to exchange each.
     */
    void begin(int msgN);

    /**
     * @brief Fetches and prints the log of messages from the servers.
     * 
     */
    void fetchLog();

    void AsyncCompleteRpc();

};

#endif