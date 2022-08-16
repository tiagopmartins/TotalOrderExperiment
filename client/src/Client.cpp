#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <grpcpp/grpcpp.h>

#include "Client.h"

#include "proto/client.grpc.pb.h"

Client::Client() {
    findProcesses();
}

Client::Client(std::shared_ptr<grpc::Channel> channel) : _stub(messages::Client::NewStub(channel)) {
    findProcesses();
}

void Client::findProcesses() {
    std::string type, address;
    std::ifstream addressList(SERVER_LIST_PATH, std::ios::in);

    if (addressList.is_open() && addressList.good()) {
        while(getline(addressList, type)) {
            getline(addressList, address);  // Get address
            address.erase(remove_if(address.begin(), address.end(), isspace), address.end());

            if (!(type.compare("server:"))) {
                this->_servers.push_back(address);

            } else if (!(type.compare("client:"))) {
                continue;  // Discard client address
            
            } else {
                std::cerr << SERVER_LIST_PATH << " with bad entry: " << type << std::endl;
            }
        }
    
    } else {
        std::cerr << "Could not open the addresses' file correctly." << std::endl;
    }

    addressList.close();
}

void Client::printLog(messages::LogReply *reply) {
    std::cout << "-> LOG - " << reply->address() << std::endl;
    for (const std::string &msg : reply->log()) {
        std::cout << "\t" << msg << std::endl;
    }
    std::cout << std::endl;
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
    for (std::string address : this->_servers) {
        sendMessage(address, &request);
    }
}

void Client::fetchLog() {
    messages::LogRequest request;
    messages::LogReply reply;
    
    // Broadcast log request
    for (std::string address : this->_servers) {
        createStub(address);
        grpc::ClientContext context;
        grpc::Status status = _stub->log(&context, request, &reply);

        if (status.ok()) {
            printLog(&reply);

        } else {
            std::cerr << "-> Failed to fetch log from " << address << "\n" <<
                "\tError " << status.error_code() << ": " << status.error_message() << '\n' << std::endl;
        }
    }
}

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