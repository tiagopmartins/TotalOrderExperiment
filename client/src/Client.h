#ifndef CLIENT_H
#define CLIENT_H

#include <string>
#include <vector>
#include <map>
#include <memory>

#include <grpcpp/grpcpp.h>

#include "proto/client.grpc.pb.h"
#include "transaction-generator/TransactionGenerator.h"

/**
 * @brief Client representation.
 * 
 */
class Client {

private:
    int _id = 0;            // Client ID
    long _counter = 0;      // Local message counter

    std::map<int, std::string> _servers;
    std::map<std::string, std::vector<long>*> _transactions;
    long _keyN = 0;
    
    std::unique_ptr<messages::Client::Stub> _stub;

    TransactionGenerator *_transactionGenerator;

public:
    Client(int id, long keyN, int transactionKeyN, double zipfAlpha);

    ~Client();

    int id() {
        return this->_id;
    }

    long counter() {
        return this->_counter;
    }

    std::map<int, std::string> servers() {
        return this->_servers;
    }

    std::map<std::string, std::vector<long>*> transactions() {
        return this->_transactions;
    }

    long keyN() {
        return this->_keyN;
    }

    TransactionGenerator* transactionGenerator() {
        return this->_transactionGenerator;
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

};

#endif