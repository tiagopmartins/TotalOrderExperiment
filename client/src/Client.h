#ifndef CLIENT_H
#define CLIENT_H

#include <string>
#include <vector>
#include <memory>

#include <grpcpp/grpcpp.h>

#include "proto/client.grpc.pb.h"

const std::string SERVER_LIST_PATH = "hydro/cluster/server_ips.yml";

const std::string SERVER_PORT = "50001";    // Port the servers receive connections on

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

    std::vector<std::string> _servers;
    
    std::unique_ptr<messages::Client::Stub> _stub;
    grpc::CompletionQueue _cq;                          // Producer-consumer queue for asynchronous communication

public:
    Client();

    Client(std::shared_ptr<grpc::Channel> channel);

    ~Client() {}

    std::vector<std::string> servers() {
        return this->_servers;
    }

    /**
     * @brief Finds all the processes alive and stores them.
     * 
     */
    void findProcesses();

    void createStub(std::string address);

    void sendMessage(std::string address, messages::BeginRequest *request);

    /**
     * @brief Begin the message exchange between servers.
     * 
     * @param msgN Number of messages the servers are going to exchange each.
     */
    void begin(int msgN);

    /**
     * @brief Fetches the log of messages from the servers.
     * 
     * @return Vector with the logs, separated by the address of the respective server.
     */
    std::vector<std::string>* fetchLog();

    void AsyncCompleteRpc();

};

#endif