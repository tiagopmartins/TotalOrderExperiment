#include <iostream>
#include <memory>
#include <string>
#include <sstream>
#include <grpcpp/grpcpp.h>

#include "ServerStruct.h"
#include "ClientServiceImpl.h"
#include "MessageServiceImpl.h"
#include "ProberServiceImpl.h"
#include "zipf/ZipfGenerator.h"

#include "proto/messages.grpc.pb.h"

const int EXPECTED_ARGS_N = 4;      // Expected number of arguments passed to the program

/**
 * @brief Runs the server and it's services.
 * 
 * @param host 
 * @param keyN Number of keys.
 * @param zipfParam Zipf distribution parameter.
 */
void runServer(std::string host, long keyN, double zipfParam) {
    ZipfGenerator zipf = ZipfGenerator(zipfParam, keyN);
    std::shared_ptr<ServerStruct> serverStruct(new ServerStruct(host, zipf.sumProbs()));

    // Building the server and its services
    grpc::ServerBuilder builder;
    builder.AddListeningPort(serverStruct->address(), grpc::InsecureServerCredentials());

    ClientServiceImpl clientService = ClientServiceImpl(serverStruct);
    MessageServiceImpl messageService = MessageServiceImpl(serverStruct);
    ProberServiceImpl proberService = ProberServiceImpl();

    builder.RegisterService(&clientService);
    builder.RegisterService(&messageService);
    builder.RegisterService(&proberService);

    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    server->Wait();
}

int main(int argc, char *argv[]) {
    if (argc != EXPECTED_ARGS_N) {
        std::cerr << "Invalid number of arguments. Please, specify the host, the number of keys and the Zipf parameter." << std::endl;
        return -1;
    }

    std::string host(argv[1]);
    
    runServer(host, atol(argv[2]), std::stod(argv[3]));

    return 0;
}