#include <iostream>
#include <memory>
#include <string>
#include <sstream>
#include <grpcpp/grpcpp.h>

#include "ServerStruct.h"
#include "ClientServiceImpl.h"
#include "MessageServiceImpl.h"

#include "proto/messages.grpc.pb.h"

const int EXPECTED_ARGS_N = 2;      // Expected number of arguments passed to the program

/**
 * @brief Runs the server and it's services.
 * 
 * @param host 
 * @param port 
 */
void runServer(std::string host) {
    std::shared_ptr<ServerStruct> serverStruct(new ServerStruct(host));

    // Building the services
    ClientServiceImpl clientService = ClientServiceImpl(serverStruct);
    MessageServiceImpl messageService = MessageServiceImpl(serverStruct);
    grpc::ServerBuilder builder;
    builder.AddListeningPort(serverStruct->address(), grpc::InsecureServerCredentials());
    builder.RegisterService(&clientService);
    builder.RegisterService(&messageService);
    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());

    server->Wait();
}

int main(int argc, char *argv[]) {
    if (argc != EXPECTED_ARGS_N) {
        std::cerr << "Invalid number of arguments. Please, specify only the host of the server." << std::endl;
        return -1;
    }

    std::string host(argv[1]);
    
    runServer(host);

    return 0;
}