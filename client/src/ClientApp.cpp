#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <exception>

#include <sw/redis++/redis++.h>
#include <grpcpp/grpcpp.h>

#include "Client.h"

const int EXPECTED_ARGS_N = 2;      // Expected number of arguments passed to the program

int main(int argc, char *argv[]) {
    if (argc != EXPECTED_ARGS_N) {
        std::cerr << "Invalid number of arguments. Please, specify only the host of the server." << std::endl;
        return -1;
    }

    sw::redis::Redis *redis;

    try {
        sw::redis::ConnectionOptions config;
        config.host = argv[1];
        redis = new sw::redis::Redis(config);

    } catch (const std::exception &e) {
        std::cout << "Error while starting up Redis: " << e.what() << std::endl;
    }

    Client *client = new Client();

    int msgN = 0;
    std::string command;
    // User input reading
    while (true) {
        std::cin >> command;

        if (command.compare("begin") == 0) {
            std::cout << "Number of messages for the servers to exchange each: ";
            std::cin >> msgN;
            std::cout << std::endl;

            if (msgN <= 0) {
                std::cerr << "Invalid number of messages specified.\n" << std::endl;
                continue;
            }

            std::string_view beginCmd{ "begin " + std::to_string(msgN) };
            redis->publish("", beginCmd);
            //client->begin(msgN);
        
        } else if (command.compare("fetch") == 0) {
            std::string_view fetchCmd{ "fetch" };
            redis->publish("", fetchCmd);
            //client->fetchLog();
        
        } else if (command.compare("exit") == 0) {
            break;
            
        } else {
            std::cerr << "Invalid command specified.\n" << std::endl;
        }
    }

    delete redis;
    return 0;
}