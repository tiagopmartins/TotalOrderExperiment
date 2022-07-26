#include <chrono>
#include <cmath>
#include <iostream>
#include <fstream>
#include <string>
#include <grpcpp/grpcpp.h>
#include <google/protobuf/util/time_util.h>

#include "yaml-cpp/yaml.h"
#include "proto/prober.grpc.pb.h"

#include "Prober.h"

using namespace google::protobuf;

Prober::Prober() {
    _times = new std::vector<std::vector<double>>();
    findProcesses();
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

double Prober::sendProbingMessage(std::string address) {
    messages::ProbingRequest req;
    messages::ProbingReply reply;

    this->createStub(address + ":" + SERVER_PORT);
    grpc::ClientContext context;

    auto start = std::chrono::steady_clock::now();
    grpc::Status status = _stub->probing(&context, req, &reply);
    auto end = std::chrono::steady_clock::now();

    if (status.ok()) {
        std::cout << "-> Successfully sent probing message to " + address << '\n' << std::endl;

        // Return the value converted to milliseconds
        std::chrono::duration<double, std::milli> rtt = end - start;
        return rtt.count();

    } else {
        std::cerr << "-> Failed to send probing message to " + address << '\n' <<
            "\tError " << status.error_code() << ": " << status.error_message() << '\n' << std::endl;

        return -1;
    }
}

std::vector<std::vector<double>>* Prober::stability(std::string address, int duration) {
    this->_times->clear();

    double res = 0;
    for (int s = 0; s < duration; s++) {
        _times->push_back(std::vector<double>());
        // Probing for one second at a time
        std::chrono::steady_clock::time_point startSecond = std::chrono::steady_clock::now();
        while (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - startSecond).count() < 1) {
            res = sendProbingMessage(address);
            _times->at(s).push_back(res);
        }
    }

    return _times;
}