#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <exception>
#include <vector>

#include <sw/redis++/redis++.h>
#include <grpcpp/grpcpp.h>

#include "Client.h"
#include "Constants.h"

const int EXPECTED_ARGS_N = 2;      // Expected number of arguments passed to the program

int main(int argc, char *argv[]) {
    if (argc != EXPECTED_ARGS_N) {
        std::cerr << "Invalid number of arguments. Please, specify only the host of the client." << std::endl;
        return -1;
    }

    sw::redis::Redis *redis;

    try {
        sw::redis::ConnectionOptions config;
        config.host = REDIS_ADDRESS;
        config.port = REDIS_INTERNAL_PORT;
        redis = new sw::redis::Redis(config);

    } catch (const std::exception &e) {
        std::cout << "Error while starting up Redis: " << e.what() << std::endl;
        return -1;
    }

    Client *client = new Client();

    // Redis subscriber
    sw::redis::Subscriber sub = redis->subscriber();
    sub.subscribe("to-exp");

    sub.on_message([&client, &redis](std::string channel, std::string msg) {
        std::stringstream ss(msg);
        std::string cmd;
        getline(ss, cmd, ' ');

        if (!cmd.compare("begin")) {
            std::string duration;
            getline(ss, duration, ' ');
            client->begin(atoi(duration.c_str()));

        } else if (!cmd.compare("fetch")) {
            std::vector<std::string> *logs = client->fetchLog();
            redis->rpush("logs", logs->begin(), logs->end());
            redis->publish("to-exp", "benchmarks");
            delete logs;
        
        } else if (!cmd.compare("probe")) {
            std::string duration;
            getline(ss, duration, ' ');
            std::vector<std::string> *probing = client->probe(std::atoi(duration.c_str()));
            redis->rpush("probe", probing->begin(), probing->end());
            redis->publish("to-exp", "probing");
            delete probing;
        
        } else if (!cmd.compare("exit")) {
            std::cout << "Frontend exited" << std::endl;
        
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