#ifndef SERVER_STRUCT_H
#define SERVER_STRUCT_H

#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <grpcpp/grpcpp.h>

#include "proto/messages.grpc.pb.h"

const std::string SERVER_LIST_PATH = "hydro/cluster/server_ips.yml";

/**
 * @brief Generic server representation.
 * 
 */
class ServerStruct {

private:
    std::vector<std::string> _servers;
    std::string _seq;   // Sequencer

    std::unique_ptr<messages::Messenger::Stub> _stub;

    std::string _host;
    std::string _port;

    int _msgCounter;
    std::vector<std::string> _log;      // Message log

    int _seqN;  // Sequence number counter (used by the sequencer)

    // Mutexes
    std::mutex _seqNMutex;
    std::shared_mutex _msgCounterMutex, _logMutex;

public:
    ServerStruct(std::string host, std::string port);

    ~ServerStruct() {}

    std::vector<std::string> servers() {
        return this->_servers;
    }

    std::vector<std::string> clients() {
        return this->_clients;
    }

    std::string seq() {
        return this->_seq;
    }

    std::string host() {
        return this->_host;
    }

    std::string port() {
        return this->_port;
    }

    void seq(std::string seq) {
        this->_seq = seq;
    }

    void host(std::string host) {
        this->_host = host;
    }

    void port(std::string port) {
        this->_port = port;
    }

    int msgCounter() {
        //std::lock_guard<std::mutex> lockGuard(_msgCounterMutex);
        return this->_msgCounter;
    }

    void incrementMsgCounter() {
        std::lock_guard<std::shared_mutex> lockGuard(_msgCounterMutex);
        this->_msgCounter++;
    }

    std::vector<std::string> log() {;
        return this->_log;
    }

    std::shared_mutex* msgCounterMutex() {
        return &this->_msgCounterMutex;
    }

    std::shared_mutex* logMutex() {
        return &this->_logMutex;
    }

    /**
     * @brief Finds all the processes alive and stores them.
     * 
     */
    void findProcesses();

    /**
     * @brief Adds to the log the register with the specified format.
     * 
     * @param address Address of the sending server.
     * @param msgID ID of the message from the server.
     */
    void insertLog(std::string address, int msgID);

    void createStub(std::string address);

    /**
     * @brief Leader election: chooses a leader based on the server list.
     * The file is the same for all processes and so it's possible to elect a
     * leader based on a deterministic computation, without the need to exchange
     * messages.
     * 
     * @return Address of the leader.
     */
    std::string electLeader();

    /**
     * @brief Sends a message to the specified address.
     * 
     * @param address 
     * @param request Message request object.
     * @param reply Message Reply object.
     */
    void sendMessage(std::string address, messages::MessageRequest request,
            messages::MessageReply *reply);

    /**
     * @brief Sends a sequence number to a process regarding the
     * message specified by it's message ID.
     * 
     * @param address 
     * @param msgId 
     */
    void sendSequencerNumber(std::string address, int msgId);

};

#endif