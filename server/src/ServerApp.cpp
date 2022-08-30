#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <sstream>
#include <grpcpp/grpcpp.h>
#include <sw/redis++/redis++.h>

#include "ServerStruct.h"
#include "ClientServiceImpl.h"
#include "MessageServiceImpl.h"

#include "proto/messages.grpc.pb.h"

const int EXPECTED_ARGS_N = 3;      // Expected number of arguments passed to the program

/**
 * @brief Runs the server and it's services.
 * 
 * @param host 
 * @param port 
 */
void runServer(std::string host, std::string port) {
    std::shared_ptr<ServerStruct> serverStruct(new ServerStruct(host, port));
    sw::redis::Redis redis = sw::redis::Redis("tcp://" + serverStruct->clients()[0] + ":6739");
    std::cout << "tcp://" + serverStruct->clients()[0] + ":6739" << std::endl;

    // Building the services
    ClientServiceImpl clientService = ClientServiceImpl(serverStruct);
    MessageServiceImpl messageService = MessageServiceImpl(serverStruct);
    grpc::ServerBuilder builder;
    builder.AddListeningPort(host + ":" + port, grpc::InsecureServerCredentials());
    builder.RegisterService(&clientService);
    builder.RegisterService(&messageService);
    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());

    // Redis subscriber
    sw::redis::Subscriber sub = redis.subscriber();
    sub.on_message([&serverStruct](std::string channel, std::string msg) {
        std::stringstream ss(msg);
        std::string cmd;
        getline(ss, cmd, ' ');

        if (cmd.compare("begin")) {
            std::string msgN;
            getline(ss, msgN, ' ');
            serverStruct->begin(atoi(msgN.c_str()));

        } else if (cmd.compare("fetch")) {
            serverStruct->fetch();
        }
    });

    do {
        try {
            sub.consume();
        
        } catch (const sw::redis::Error &err) {
            std::cerr << "Redis error: " << err.what() << std::endl;
        }

    } while (true);

    server->Wait();
}

int main(int argc, char *argv[]) {
    if (argc != EXPECTED_ARGS_N) {
        std::cerr << "Invalid number of arguments. Please, specify only the host and port of the server." << std::endl;
        return -1;
    }

    std::string host(argv[1]);
    std::string port(argv[2]);
    
    runServer(host, port);

    return 0;
}