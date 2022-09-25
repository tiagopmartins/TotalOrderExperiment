#include <chrono>
#include <cmath>
#include <iostream>
#include <fstream>
#include <string>
#include <grpcpp/grpcpp.h>

#include "yaml-cpp/yaml.h"
#include "proto/prober.grpc.pb.h"

#include "Prober.h"

Prober::Prober() {
    _times = new std::vector<std::vector<int64_t>>();
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

int64_t Prober::sendProbingMessage(std::string address) {
    messages::ProbingRequest req;
    messages::ProbingReply reply;

    this->createStub(address + ":" + SERVER_PORT);
    grpc::ClientContext context;

    std::chrono::time_point now = std::chrono::system_clock::now();
    std::chrono::time_point nowMS = std::chrono::time_point_cast<std::chrono::milliseconds>(now);
    auto currentTime = std::chrono::duration_cast<std::chrono::milliseconds>(nowMS.time_since_epoch());

    grpc::Status status = _stub->probing(&context, req, &reply);
    int64_t duration = reply.arrival() - currentTime.count();

    if (status.ok()) {
        std::cout << "-> Successfully sent probing message to " + address << '\n' << std::endl;
        return duration;

    } else {
        std::cerr << "-> Failed to send probing message to " + address << '\n' <<
            "\tError " << status.error_code() << ": " << status.error_message() << '\n' << std::endl;

        return -1;
    }
}

std::vector<std::vector<int64_t>>* Prober::stability(std::string address, int duration) {
    this->_times->clear();

    int seconds = 0, res = 0;
    for (int s = 0; s < duration; s++) {
        _times->push_back(std::vector<int64_t>());
        // Probing for one second at a time
        std::chrono::steady_clock::time_point startSecond = std::chrono::steady_clock::now();
        while (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - startSecond).count() < 1) {
            res = sendProbingMessage(address);
            _times->at(s).push_back(res);
        }
    }

    return _times;
}