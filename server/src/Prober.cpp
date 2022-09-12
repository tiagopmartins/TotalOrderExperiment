#include <chrono>
#include <iostream>
#include <fstream>
#include <string>
#include <grpcpp/grpcpp.h>

#include "yaml-cpp/yaml.h"
#include "proto/prober.grpc.pb.h"

#include "Prober.h"

Prober::Prober() {
    findProcesses();

    for (const std::string address : _servers) {
        _times.insert({address, std::vector<int64_t>()});
    }

    stability();
}

void Prober::findProcesses() {
    YAML::Node addresses = YAML::LoadFile(SERVER_LIST_PATH);

    if (addresses["ips"]) {
        for (std::size_t i = 0; i < addresses["ips"].size(); i++) {
            _servers.push_back(addresses["ips"][i].as<std::string>());
        }
    }
}

void Prober::createStub(std::string address) {
    _stub = messages::Prober::NewStub(grpc::CreateChannel(address, grpc::InsecureChannelCredentials()));
}

void Prober::sendProbingMessage(std::string address) {
    messages::ProbingRequest req;
    messages::ProbingReply reply;

    this->createStub(address);
    grpc::ClientContext context;

    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    grpc::Status status = _stub->probing(&context, req, &reply);
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();

    _times.at(address).push_back(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count());

    if (status.ok()) {
        std::cout << "-> Successfully sent probing message to " + address << '\n' << std::endl;

    } else {
        std::cerr << "-> Failed to send probing message to " + address << '\n' <<
            "\tError " << status.error_code() << ": " << status.error_message() << '\n' << std::endl;
    }
}

void Prober::stability() {
    for (const std::string address : _servers) {
        sendProbingMessage(address);
    }

    for (const auto [key, list] : _times) {
        std::cout << key << ":" << std::endl;

        for (auto value : list) {
            std::cout << value << " ";
        }
        std::cout << std::endl;
    }
}