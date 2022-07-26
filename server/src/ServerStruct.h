#ifndef SERVER_STRUCT_H
#define SERVER_STRUCT_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <grpcpp/grpcpp.h>

#include "Sequencer.h"

#include "proto/messages.grpc.pb.h"

// Path to the file containing the IPs of the servers
const std::string SERVER_LIST_PATH = "hydro/cluster/server_ips.yml";

// Port the servers receive connections on
const std::string SERVER_PORT = "50001";

/**
 * @brief Generic server representation.
 * 
 */
class ServerStruct : public Sequencer {

private:
    int _id;
    std::string _host;
    std::map<int, std::string> _servers;
    std::string _seq;

    std::unique_ptr<messages::Messenger::Stub> _stub;

    std::shared_mutex _logMutex;
    std::vector<std::string> _log;      // Message log

    //std::map<std::pair<int, std::string>> messages; // Mapping between sequence numbers and messages

    // Queues
    //std::vector<int> rQ:        // Messages received
    //std::vector<int> sQ;        // Sequence numbers
    //std::vector<int> oQ;        // Messages optimally delivered
    //std::vector<int> fQ;        // Messages finally delivered

    // Delay information
    //std::map<std::pair<std::string, int>> delays;

public:
    ServerStruct(std::string host);

    std::map<int, std::string> servers() {
        return this->_servers;
    }

    std::string seq() {
        return this->_seq;
    }

    int id() {
        return this->_id;
    }

    std::string host() {
        return this->_host;
    }

    std::string port() {
        return SERVER_PORT;
    }

    std::string address() {
        return this->_host + ":" + SERVER_PORT;
    }

    void host(std::string host) {
        this->_host = host;
    }

    std::vector<std::string> log() {;
        return this->_log;
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
     * @param clientId ID of the client issuing the message.
     * @param msgID ID of the message from the server.
     */
    void insertLog(int clientId, long msgID);

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

    void sendSequencerNumber(std::string address, int msgId) override;

};

#endif