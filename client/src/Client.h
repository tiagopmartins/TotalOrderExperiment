#ifndef CLIENT_H
#define CLIENT_H

#include <string>
#include <vector>
#include <map>
#include <memory>

#include <grpcpp/grpcpp.h>

#include "proto/client.grpc.pb.h"

/**
 * @brief Client representation.
 * 
 */
class Client {

private:
    int _id = 0;            // Client ID
    long _counter = 0;      // Local message counter

    std::map<int, std::string> _servers;
    long _keyN = 0;
    
    std::unique_ptr<messages::Client::Stub> _stub;

public:
    Client(int id, long keyN);

    Client(int id, long keyN, std::shared_ptr<grpc::Channel> channel);

    ~Client() {}

    int id() {
        return this->_id;
    }

    long counter() {
        return this->_counter;
    }

    std::map<int, std::string> servers() {
        return this->_servers;
    }

    long keyN() {
        return this->_keyN;
    }

    /**
     * Converts the server map into a list of addresses.
     *
     * @return Vector with the addresses of the servers.
     */
    std::vector<std::string>* serverList();

    /**
     * Increments the value of the local client counter.
     */
     void incrementCounter() {
         this->_counter++;
     }

    /**
     * @brief Finds all the processes alive and stores them.
     * 
     */
    void findProcesses();

    void createStub(std::string address);

    /**
     * Executes a remote request for a transaction.
     */
    void execute();

    /**
     * @brief Gets the datacenter of the specified server.
     *
     * @param ip
     * @return Datacenter name.
     */
    std::string getDatacenter(std::string ip);

    /**
     * @brief Fetches the log of messages from the servers.
     * 
     * @return Vector with the logs, separated by the address of the respective server.
     */
    std::vector<std::string>* fetchLog();

    /**
     * @brief Starts probing the network to check it's stabilty.
     *
     * @param address Address of the server to probe.
     * @param duration Duration of the probing.
     * @return Vector with the times.
     */
    std::vector<std::string>* probe(std::string address, int duration);

private:
    /**
     * Gets the partitions to contact pertaining the specified transaction.
     * @param keys Keys used by the transaction.
     * @return Vector of partitions to contact with the respective keys.
     */
    std::map<std::string, std::vector<long>>* getPartitions(std::vector<long> keys);

};

#endif