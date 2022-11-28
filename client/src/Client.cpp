#include <iomanip>
#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <vector>

#include <grpcpp/grpcpp.h>

#include "yaml-cpp/yaml.h"
#include "proto/client.grpc.pb.h"

#include "Client.h"
#include "prober/Prober.h"
#include "transaction-generator/TransactionGenerator.h"


Client::Client(int id, long keyN, int transactionKeyN, double zipfAlpha) : _id(id), _keyN(keyN) {
    findProcesses();
    this->_transactionGenerator = new TransactionGenerator(keyN, this->servers().size(),
                                                          transactionKeyN, zipfAlpha);
    this->_transactions = std::map<std::string, std::vector<long>*>();
}

Client::~Client() {
    for (auto &[id, keys] : this->transactions()) {
        delete keys;
    }
}

std::vector<std::string>* Client::serverList() {
    std::vector<std::string>* serverList = new std::vector<std::string>();
    for (auto const &[id, ip] : this->_servers) {
        std::string datacenter = getDatacenter(ip);
        serverList->push_back(datacenter + "$" + ip);;
    }

    return serverList;
}

void Client::findProcesses() {
    YAML::Node addresses = YAML::LoadFile(SERVER_LIST_PATH);

    int id = 0;
    if (addresses["ips"]) {
        for (std::size_t i = 0; i < addresses["ips"].size(); i++) {
            _servers.insert({id, addresses["ips"][i].as<std::string>()});
            id++;
        }
    }
}

void Client::createStub(std::string address) {
    auto args = grpc::ChannelArguments();
    args.SetMaxReceiveMessageSize(1000 * 1024 * 1024);  // 1 GB
    _stub = messages::Client::NewStub(grpc::CreateCustomChannel(address, grpc::InsecureChannelCredentials(), args));
}

std::string Client::getDatacenter(std::string ip) {
    messages::DatacenterRequest request;
    messages::DatacenterReply reply;
    grpc::ClientContext context;

    createStub(ip + ":" + SERVER_PORT);
    grpc::Status status = _stub->datacenter(&context, request, &reply);

    if (status.ok()) {
        return reply.datacenter();

    } else {
        std::cerr << "-> Failed to get datacenter from " << ip + ":" + SERVER_PORT << "\n" <<
                  "\tError " << status.error_code() << ": " << status.error_message() << '\n' << std::endl;
        return "";
    }
}

void Client::execute() {
    messages::TransactionRequest request;
    messages::TransactionReply reply;
    request.set_clientid(this->id());
    request.set_messageid(this->counter());

    std::vector<long>* transaction = this->transactionGenerator()->transaction();
    this->_transactions.insert({std::to_string(this->id()) + ":" + std::to_string(this->counter()), transaction});
    this->incrementCounter();

    std::map<int, std::vector<long>> keySets;   // Mapping between server IDs and keys
    for (long &key : *transaction) {
        int id = this->transactionGenerator()->keyServer(key);
        if (keySets.count(id) == 0) {
            keySets.insert({id, std::vector<long>()});
        }
        keySets[id].push_back(key);
    }

    for (auto &[id, keys] : keySets) {
        std::string ip = this->servers()[id];
        createStub(ip + ":" + SERVER_PORT);
        grpc::ClientContext context;
        grpc::Status status = _stub->execute(&context, request, &reply);

        if (status.ok()) {
            std::cout << "Transaction executed: " << request.clientid() << "-" << request.messageid() << '\n' << std::endl;

        } else {
            std::cerr << "-> Failed to execute transaction " << request.clientid() << ":" << request.messageid() << "\n" <<
                      "\tError " << status.error_code() << ": " << status.error_message() << '\n' << std::endl;
        }
    }
}

std::vector<std::string>* Client::fetchLog() {
    std::vector<std::string> *logs = new std::vector<std::string>();
    messages::LogRequest request;
    messages::LogReply reply;
    
    // Broadcast log request
    for (auto const &[id, ip] : this->_servers) {
        createStub(ip + ":" + SERVER_PORT);
        grpc::ClientContext context;
        grpc::Status status = _stub->log(&context, request, &reply);

        if (status.ok()) {
            if (reply.sequencer()) {
                logs->push_back("SEQUENCER$" + reply.address());  // start of a sequencer log

            } else {
                logs->push_back("SERVER$" + reply.address());  // start of a server log
            }

            for (const std::string &msg : reply.log()) {
                logs->push_back(msg);
            }

        } else {
            std::cerr << "-> Failed to fetch log from " << ip + ":" + SERVER_PORT << '\n' <<
                "\tError " << status.error_code() << ": " << status.error_message() << '\n' << std::endl;
        }
    }

    return logs;
}

std::vector<std::string>* Client::probe(std::string address, int duration) {
    Prober prober = Prober();
    std::vector<std::vector<double>> *times = prober.stability(address, duration);

    int s = 1;
    std::vector<std::string> *probing = new std::vector<std::string>();
    // Format list to send to Redis
    for (auto const &perSecondValues : *times) {
        probing->push_back("SECOND$" + std::to_string(s));
        for (double const &value : perSecondValues) {
            // Set value precision
            std::stringstream valueStream;
            valueStream << std::fixed << std::setprecision(2) << value;
            probing->push_back(valueStream.str());
        }
        s++;
    }

    return probing;
}
