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
    
    // Initializing Redis connection
    sw::redis::Redis *redis;
    try {
        sw::redis::ConnectionOptions options;
        options.host = serverStruct->clients()[0];
        options.port = 6379;
        redis = new sw::redis::Redis(options);

    } catch (const std::exception &e) {
        std::cout << "Error while starting up Redis: " << e.what() << std::endl;
    }

    // Building the services
    ClientServiceImpl clientService = ClientServiceImpl(serverStruct);
    MessageServiceImpl messageService = MessageServiceImpl(serverStruct);
    grpc::ServerBuilder builder;
    builder.AddListeningPort(host + ":" + port, grpc::InsecureServerCredentials());
    builder.RegisterService(&clientService);
    builder.RegisterService(&messageService);
    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());

    // Redis subscriber
    sw::redis::Subscriber sub = redis->subscriber();
    sub.subscribe("to-exp");

    sub.on_message([&serverStruct](std::string channel, std::string msg) {
        std::stringstream ss(msg);
        std::string cmd;
        getline(ss, cmd, ' ');

        std::cout << cmd << std::endl;

        if (cmd.compare("begin") == 0) {
            std::string msgN;
            getline(ss, msgN, ' ');
            serverStruct->begin(atoi(msgN.c_str()));

        } else if (cmd.compare("fetch") == 0) {
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