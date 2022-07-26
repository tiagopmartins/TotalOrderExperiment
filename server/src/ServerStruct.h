#ifndef SERVER_STRUCT_H
#define SERVER_STRUCT_H

#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <grpcpp/grpcpp.h>

#include "proto/messages.grpc.pb.h"

/**
 * @brief Generic server representation.
 * 
 */
class ServerStruct {

private:
    const std::vector<std::string> SERVERS = {"localhost:50001", "localhost:50002"};   // Server list for testing purposes

    std::unique_ptr<messages::Messenger::Stub> _stub;

    std::string _host;
    std::string _port;

    int _msgCounter;
    std::vector<std::string> _log;      // Message log

public:
    ServerStruct(std::string host, std::string port) : _host(host), _port(port),
            _msgCounter(0) {}

    ~ServerStruct() {}

    std::vector<std::string> servers() {
        return this->SERVERS;
    }

    std::string host() {
        return this->_host;
    }

    std::string port() {
        return this->_port;
    }

    void host(std::string host) {
        this->_host = host;
    }

    void port(std::string port) {
        this->_port = port;
    }

    int msgCounter() {
        std::mutex _msgCounterMutex;
        std::lock_guard<std::mutex> lockGuard(_msgCounterMutex);
        return this->_msgCounter;
    }

    void incrementMsgCounter() {
        std::mutex _msgCounterMutex;
        std::lock_guard<std::mutex> lockGuard(_msgCounterMutex);
        this->_msgCounter++;
    }

    std::vector<std::string> log() {
        std::mutex _logMutex;
        std::lock_guard<std::mutex> lockGuard(_logMutex);
        return this->_log;
    }

    /**
     * @brief Adds to the log the register with the specified format.
     * 
     * @param address Address of the sending server.
     * @param msgID ID of the message from the server.
     */
    void insertLog(std::string address, int msgID);

    void createStub(std::string address);

    /**
     * @brief Sends a message to the specified address.
     * 
     * @param address 
     * @param request Message request object.
     * @param reply Message Reply object.
     */
    void sendMessage(std::string address, messages::MessageRequest request,
            messages::MessageReply *reply);

};

#endif