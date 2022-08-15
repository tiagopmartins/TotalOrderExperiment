#include <fstream>
#include <string>
#include <mutex>
#include <grpcpp/grpcpp.h>

#include "ServerStruct.h"

#include "proto/messages.grpc.pb.h"

ServerStruct::ServerStruct(std::string host, std::string port) : _host(host), _port(port),
        _msgCounter(0) {
    findProcesses();
    this->_seq = electLeader();
    std::cout << this->_seq << std::endl;
}

void ServerStruct::findProcesses() {
    std::string type, address;
    std::ifstream addressList(SERVER_LIST_PATH, std::ios::in);

    if (addressList.is_open() && addressList.good()) {
        while(getline(addressList, type)) {
            getline(addressList, address);  // Get address
            address.erase(remove_if(address.begin(), address.end(), isspace), address.end());

            if (!(type.compare("server:"))) {
                this->_servers.push_back(address);

            } else if (!(type.compare("client:"))) {
                this->_clients.push_back(address);
            
            } else {
                std::cerr << SERVER_LIST_PATH << " with bad entry: " << type << std::endl;
            }
        }
    
    } else {
        std::cerr << "Could not open the addresses' file correctly." << std::endl;
    }

    addressList.close();
}

void ServerStruct::insertLog(std::string address, int msgID) {
    std::lock_guard<std::mutex> lockGuard(_logMutex);
    this->_log.push_back(address + " " + std::to_string(msgID));
}

void ServerStruct::createStub(std::string address) {
    _stub = messages::Messenger::NewStub(grpc::CreateChannel(address, grpc::InsecureChannelCredentials()));
}

std::string ServerStruct::electLeader() {
    return this->_servers[0];
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