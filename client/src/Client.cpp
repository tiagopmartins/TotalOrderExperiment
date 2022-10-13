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
}

Client::Client(std::shared_ptr<grpc::Channel> channel) : _stub(messages::Client::NewStub(channel)) {
    findProcesses();
}

std::vector<std::string>* Client::serverList() {
    std::vector<std::string>* serverList = new std::vector<std::string>();
    for (auto const &[id, ip] : this->_servers) {
        std::string datacenter = getDatacenter(ip);
        serverList->push_back(datacenter + "$" + ip);

        std::cout << "DATACENTER: " << datacenter << std::endl;
    }

    return serverList;
}

void Client::findProcesses() {
    YAML::Node addresses = YAML::LoadFile(SERVER_LIST_PATH);

    int id = 0;
    if (addresses["ips"]) {
        for (std::size_t i = 0; i < addresses["ips"].size(); i++) {
            _servers.insert({id, addresses["ips"][i].as<std::string>()});
            id++;
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
    auto args = grpc::ChannelArguments();
    args.SetMaxReceiveMessageSize(1000 * 1024 * 1024);  // 1 GB
    _stub = messages::Client::NewStub(grpc::CreateCustomChannel(address, grpc::InsecureChannelCredentials(), args));
}

std::string Client::getDatacenter(std::string ip) {
    messages::DatacenterRequest request;
    messages::DatacenterReply reply;
    grpc::ClientContext context;

    createStub(ip + ":" + SERVER_PORT);
    grpc::Status status = _stub->datacenter(&context, request, &reply);

    if (status.ok()) {
        return reply.datacenter();

    } else {
        std::cerr << "-> Failed to get datacenter from " << ip + ":" + SERVER_PORT << "\n" <<
                  "\tError " << status.error_code() << ": " << status.error_message() << '\n' << std::endl;
        return "";
    }
}

void Client::begin(int duration) {
    messages::BeginRequest request;
    request.set_duration(duration);

    // Thread to wait for replies
    std::thread (&Client::AsyncCompleteRpc, this).detach();

    // Broadcast begin to servers
    for (auto const &[id, ip] : this->_servers) {
        sendMessage(ip + ":" + SERVER_PORT, &request);
    }
}

std::vector<std::string>* Client::fetchLog() {
    std::vector<std::string> *logs = new std::vector<std::string>();
    messages::LogRequest request;
    messages::LogReply reply;
    
    // Broadcast log request
    for (auto const &[id, ip] : this->_servers) {
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

std::vector<std::string>* Client::probe(std::string address, int duration) {
    Prober prober = Prober();
    std::vector<std::vector<uint64_t>> *times = prober.stability(address, duration);

    int s = 1;
    std::vector<std::string> *probing = new std::vector<std::string>();
    // Format list to send to Redis
    for (auto const &perSecondValues : *times) {
        probing->push_back("SECOND$" + std::to_string(s));
        for (double const &value : perSecondValues) {
            probing->push_back(std::to_string(value));
        }
        s++;
    }

    return probing;
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