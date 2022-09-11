#ifndef PROBER_H
#define PROBER_H

#include <vector>
#include <string>
#include <map>
#include <grpcpp/grpcpp.h>

#include "proto/prober.grpc.pb.h"

// Path to the file containing the IPs of the servers
const std::string SERVER_LIST_PATH = "hydro/cluster/server_ips.yml";

/**
 * @brief Entity which tests the state of the network.
 * 
 */
class Prober {

private:
    const std::string SERVER_PORT = "50001";    // Port to receive connections on

    std::vector<std::string> _servers;

    std::unique_ptr<messages::Prober::Stub> _stub;

    std::map<std::string, std::vector<int>> times;   // Mapping between addresses and times

    /**
     * @brief Finds all the processes alive and stores them.
     * 
     */
    void findProcesses();

    void createStub(std::string address);

public:
    Prober();

    std::vector<std::string> servers() {
        return this->_servers;
    }

    /**
     * @brief Sends a probing message to the requested address.
     * 
     * @param address 
     * @return int Time it took for the message to get there.
     */
    int sendProbingMessage(std::string address);

    /**
     * @brief Evaluates the stability of the network, including the time messages
     * take to arrive. Sends the results to the Redis database.
     * 
     */
    void stability();

};

#endif