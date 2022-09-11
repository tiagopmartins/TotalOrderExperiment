#include <fstream>
#include <string>
#include <grpcpp/grpcpp.h>

#include "yaml-cpp/yaml.h"
#include "proto/prober.grpc.pb.h"

#include "Prober.h"

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

int Prober::sendProbingMessage(std::string address) {
    // Send a probing message

    return 0;
}

void stability() {
    // Calculates the stability
}