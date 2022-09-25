#ifndef PROBER_H
#define PROBER_H

#include <vector>
#include <string>
#include <map>
#include <grpcpp/grpcpp.h>

#include "proto/prober.grpc.pb.h"

#include "Constants.h"

/**
 * @brief Entity which tests the state of the network.
 * 
 */
class Prober {

private:
    std::vector<std::string> _servers;

    std::unique_ptr<messages::Prober::Stub> _stub;

    std::vector<std::vector<int64_t>>* _times;   // Mapping between addresses and times (per second)

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

    std::vector<std::vector<int64_t>>* times() {
        return this->_times;
    }    

    /**
     * @brief Sends a probing message to the requested address.
     * 
     * @param address
     * @return Time it took for the message to arrive at the specified address.
     */
    int64_t sendProbingMessage(std::string address);

    /**
     * @brief Evaluates the stability of the network, to a certain address,
     * including the time messages take to arrive. Sends the results to the Redis database.
     *
     * @duration Duration of the probing.
     * @return Vector divided into sections (vectors) containing the values of the
     * probing during each second.
     */
    std::vector<std::vector<int64_t>>* stability(std::string address, int duration);

};

#endif