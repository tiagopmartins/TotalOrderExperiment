#include <fstream>
#include <string>
#include <mutex>
#include <grpcpp/grpcpp.h>

#include "yaml-cpp/yaml.h"
#include "proto/messages.grpc.pb.h"

#include "ServerStruct.h"

ServerStruct::ServerStruct(std::string host, std::string port) : _host(host), _port(port),
        _msgCounter(0), _seqN(0) {
    findProcesses();
    this->_seq = electLeader();
}

void ServerStruct::findProcesses() {
    YAML::Node addresses = YAML::LoadFile(SERVER_LIST_PATH);

    if (addresses["servers"]) {
        for (std::size_t i = 0; i < addresses["servers"].size(); i++) {
            _servers.push_back(addresses["servers"][i].as<std::string>());
        }
    }

    if (addresses["clients"]) {
        for (std::size_t i = 0; i < addresses["clients"].size(); i++) {
            _clients.push_back(addresses["clients"][i].as<std::string>());
        }
    }
}

void ServerStruct::insertLog(std::string address, int msgID) {
    std::lock_guard<std::shared_mutex> lockGuard(_logMutex);
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

void ServerStruct::sendSequencerNumber(std::string address, int msgId) {
    messages::SeqNumberRequest seqNumRequest;
    messages::SeqNumberReply seqNumReply;

    seqNumRequest.set_msgid(msgId);
    std::unique_lock<std::mutex> lock(_seqNMutex);
    seqNumRequest.set_seqn(this->_seqN);
    this->_seqN++;
    lock.unlock();

    createStub(address);
    grpc::ClientContext context;
    grpc::Status status = _stub->sendSeqNumber(&context, seqNumRequest, &seqNumReply);

    if (status.ok()) {
        std::cout << "-> Successfully sent sequence number to " + address << " (message ID: " << msgId << ")\n" << std::endl;

    } else {
        std::cerr << "-> Failed to send sequence number to " + address << " (message ID: " << msgId << ")\n" + address << "\n" <<
            "\tError " << status.error_code() << ": " << status.error_message() << '\n' << std::endl;
    }
}