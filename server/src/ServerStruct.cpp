#include <fstream>
#include <string>
#include <mutex>
#include <grpcpp/grpcpp.h>

#include "ServerStruct.h"

#include "proto/messages.grpc.pb.h"

ServerStruct::ServerStruct(std::string host, std::string port) : _host(host), _port(port),
        _msgCounter(0) {
    findServers();
}

void ServerStruct::findServers() {
    std::string host;
    std::ifstream serverList(SERVER_LIST_PATH, std::ios::in);

    if (serverList.is_open() && serverList.good()) {
        while(getline(serverList, host)) {
            this->_servers.push_back(host);
        }
    
    } else {
        std::cerr << "Could not open the server's file correctly." << std::endl;
    }

    serverList.close();
}

void ServerStruct::insertLog(std::string address, int msgID) {
    std::mutex _logMutex;
    std::lock_guard<std::mutex> lockGuard(_logMutex);
    this->_log.push_back(address + " " + std::to_string(msgID));
}

void ServerStruct::createStub(std::string address) {
    _stub = messages::Messenger::NewStub(grpc::CreateChannel(address, grpc::InsecureChannelCredentials()));
}

void ServerStruct::sendMessage(std::string address, messages::MessageRequest request,
        messages::MessageReply *reply) {
    createStub(address);
    grpc::ClientContext context;
    grpc::Status status = _stub->send(&context, request, reply);

    if (status.ok()) {
        std::cout << "-> Successfully sent message to " + address << '\n' << std::endl;

    } else {
        std::cerr << "-> Failed to send message to " + address << "\n" <<
            "\tError " << status.error_code() << ": " << status.error_message() << '\n' << std::endl;
    }
}