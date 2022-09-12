#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <vector>

#include <grpcpp/grpcpp.h>

#include "yaml-cpp/yaml.h"
#include "proto/client.grpc.pb.h"

#include "Client.h"
#include "Prober.h"

Client::Client() {
    findProcesses();
    Prober prober = Prober();
    prober.stability();
}

Client::Client(std::shared_ptr<grpc::Channel> channel) : _stub(messages::Client::NewStub(channel)) {
    findProcesses();
}

void Client::findProcesses() {
    YAML::Node addresses = YAML::LoadFile(SERVER_LIST_PATH);

    if (addresses["ips"]) {
        for (std::size_t i = 0; i < addresses["ips"].size(); i++) {
            _servers.push_back(addresses["ips"][i].as<std::string>());
        }
    }
}

void Client::sendMessage(std::string address, messages::BeginRequest *request) {
    createStub(address);
    AsyncClientCall* call = new AsyncClientCall();
    call->address = address;
    call->responseReader = _stub->PrepareAsyncbegin(&call->context, *request, &_cq);
    call->responseReader->StartCall();
    call->responseReader->Finish(&call->reply, &call->status, (void*) call);
}

void Client::createStub(std::string address) {
    _stub = messages::Client::NewStub(grpc::CreateChannel(address, grpc::InsecureChannelCredentials()));
}

void Client::begin(int msgN) {
    messages::BeginRequest request;
    request.set_msgn(msgN);

    // Thread to wait for replies
    std::thread (&Client::AsyncCompleteRpc, this).detach();

    // Broadcast begin to servers
    for (std::string ip : this->_servers) {
        sendMessage(ip + ":" + SERVER_PORT, &request);
    }
}

std::vector<std::string>* Client::fetchLog() {
    std::vector<std::string> *logs = new std::vector<std::string>();
    messages::LogRequest request;
    messages::LogReply reply;
    
    // Broadcast log request
    for (std::string ip : this->_servers) {
        createStub(ip + ":" + SERVER_PORT);
        grpc::ClientContext context;
        grpc::Status status = _stub->log(&context, request, &reply);

        if (status.ok()) {
            logs->push_back("SERVER$" + reply.address());  // mark the start of a server log
            for (const std::string &msg : reply.log()) {
                logs->push_back(msg);
            }

        } else {
            std::cerr << "-> Failed to fetch log from " << ip + ":" + SERVER_PORT << "\n" <<
                "\tError " << status.error_code() << ": " << status.error_message() << '\n' << std::endl;
        }
    }

    return logs;
}

/*
std::map<std::string, std::vector<int>>* Client::probe() {
    Prober prober = Prober();
    std::map<std::string, std::vector<int>> *times = prober.stability();

    // Create vector with the info
}
*/

void Client::AsyncCompleteRpc() {
    void *gotTag;
    bool ok = false;

    // Wait until the next response in the completion queue is ready
    while (_cq.Next(&gotTag, &ok)) {
        // Tag = memory location of the call
        AsyncClientCall *call = static_cast<AsyncClientCall*>(gotTag);
        GPR_ASSERT(ok);

        if (call->status.ok()) {
            std::cout << "-> Successfully sent begin signal to " << call->address << '\n' << std::endl;

        } else {
            std::cerr << "-> Failed to send begin signal to " << call->address << "\n" <<
                "\tError " << call->status.error_code() << ": " << call->status.error_message() << '\n' << std::endl;
        }

        delete call;
    }
}