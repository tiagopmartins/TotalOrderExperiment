#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <exception>
#include <vector>

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
        // TODO: use redis pod ip
        //config.host = argv[1];
        redis = new sw::redis::Redis(config);

    } catch (const std::exception &e) {
        std::cout << "Error while starting up Redis: " << e.what() << std::endl;
    }

    Client *client = new Client();

    // Redis subscriber
    sw::redis::Subscriber sub = redis->subscriber();
    sub.subscribe("to-exp");

    sub.on_message([&client, &redis](std::string channel, std::string msg) {
        std::stringstream ss(msg);
        std::string cmd;
        getline(ss, cmd, ' ');

        if (cmd.compare("begin") == 0) {
            std::string msgN;
            getline(ss, msgN, ' ');
            client->begin(atoi(msgN.c_str()));

        } else if (cmd.compare("fetch") == 0) {
            std::vector<std::string> *logs = client->fetchLog();
            redis->rpush("logs", logs->begin(), logs->end());
            redis->publish("to-exp", "benchmarks");
            delete logs;
        
        } else {
            std::cerr << "Invalid command specified.\n" << std::endl;
        }
    });

    do {
        try {
            sub.consume();
        
        } catch (const sw::redis::Error &err) {
            std::cerr << "Redis error: " << err.what() << std::endl;
        }

    } while (true);

    delete redis;
    return 0;
}