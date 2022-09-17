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
    _times = new std::map<std::string, std::vector<std::vector<int64_t>>>();
    findProcesses();

    for (const std::string address : _servers) {
        _times->insert({address, std::vector<std::vector<int64_t>>()});
    }
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

std::vector<int64_t>* Prober::averageValue(std::string address) {
    std::vector<int64_t> *averages = new std::vector<int64_t>;
    std::vector<std::vector<int64_t>> times = _times->at(address);   // Second partitions for the address

    for (std::vector<int64_t> const &values : times) {
        int64_t sum = 0;
        for (int64_t val : values) {
            sum += val;
        }
        averages->push_back(sum / values.size());
    }

    return averages;
}

std::vector<int64_t>* Prober::stdDeviation(std::string address) {
    std::vector<int64_t> *deviations = new std::vector<int64_t>;
    std::vector<std::vector<int64_t>> times = _times->at(address);   // Second partitions for the address
    std::vector<int64_t> *averages = averageValue(address);

    for (size_t i = 0; i < times.size(); i++) {
        int64_t sum = 0;
        for (int64_t val : times[i]) {
            sum += pow((val - averages->at(i)), 2);
        }
        deviations->push_back(sqrt(sum / times[i].size()));
    }
    
    return deviations;
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

std::map<std::string, std::vector<std::vector<int64_t>>>* Prober::stability(int duration) {
    int seconds = 0, res = 0;
    while (seconds < duration) {
        _times->at(_servers[0]).push_back(std::vector<int64_t>());
        // Probing for one second at a time
        std::chrono::steady_clock::time_point startSecond = std::chrono::steady_clock::now();
        while (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - startSecond).count() < 1) {
            // TODO: turn this into an async method that sends to every server
            res = sendProbingMessage(_servers[0]);
            _times->at(_servers[0])[seconds].push_back(res);
        }

        seconds++;
    }

    return _times;
}