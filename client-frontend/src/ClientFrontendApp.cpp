#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>

#include <sw/redis++/redis++.h>

#include "Commands.h"

// Expected number of arguments passed to the program
const int EXPECTED_ARGS_N = 2;
const int EXPECTED_ARGS_N_FILE = 4;

const int REDIS_EXTERNAL_PORT = 30000;
//const int REDIS_EXTERNAL_PORT = 6379;


// ------- INPUT OUTPUT

/**
 * @brief Reads the logs list containing the logs for each server divided by its
 * respective address.
 * 
 * @param logs Map between the server address and a vector of logs for it.
 * @param logsList List of strings containing data.
 *      Form: ["SERVER$address1", "log1", "log2", ..., "SERVER$address2", ...]
 */
void readLogs(std::map<std::string, std::vector<std::string>> *logs, std::vector<std::string> *logsList) {
    std::string currentServer = "";
    for (auto it = logsList->begin(); it != logsList->end(); it++) {
        std::string token;
        std::stringstream ss(*it);
        getline(ss, token, '$');

        if (!token.compare("SERVER")) {
            getline(ss, token, ' ');    // get new server address
            logs->insert({token, std::vector<std::string>()});
            currentServer = token;
            continue;
        }

        logs->at(currentServer).push_back(token);   // push log info
    }
}

/**
 * @brief Reads the probing results for each server divided by its respective
 * address.
 *
 * @param probing Vector divided into vectors representing each second,
 * containing the results inside that same second.
 * @param probingList List of strings containing data.
 *      Form: ["SECOND$1", "value1", "value2", ..., "SECOND$2", ...]
 */
void readProbing(std::map<int, std::vector<std::string>> *probing, std::vector<std::string> *probingList) {
    int currentSecond = 0;
    for (auto it = probingList->begin(); it != probingList->end(); it++) {
        std::string token;
        std::stringstream ss(*it);
        getline(ss, token, '$');

        if (!token.compare("SECOND")) {
            getline(ss, token, ' ');    // get the new second
            currentSecond = std::atoi(token.c_str());

            // Not in the map
            if (probing->find(currentSecond) == probing->end()) {
                probing->insert({currentSecond, std::vector<std::string>()});
            }

            continue;
        }

        probing->at(currentSecond).push_back(token);   // push probing value
    }
}

/**
 * @brief Reads the server list containing the addresses of the servers for each
 * datacenter.
 *
 * @param servers Map between the datacenter and a vector of addresses.
 * @param serverList List of strings containing data.
 *      Form: ["datacenter1$addr1", "datacenter2$addr2", ...]
 */
void readServers(std::map<std::string, std::vector<std::string>> *servers, std::vector<std::string> *serverList) {
    std::string currentDatacenter = "";
    for (auto it = serverList->begin(); it != serverList->end(); it++) {
        std::string datacenter, ip;
        std::stringstream ss(*it);
        getline(ss, datacenter, '$');

        if (!servers->count(datacenter)) {   // check the existence of the datacenter
            servers->insert({datacenter, std::vector<std::string>()});
        }

        getline(ss, ip, ' ');    // get ip
        servers->at(datacenter).push_back(ip);
    }
}

/**
 * @brief Prints the servers that are active.
 *
 * @param servers Mapping between datacenters and addresses.
 */
void printServers(std::map<std::string, std::vector<std::string>> *servers) {
    std::cout << "Servers:" << std::endl;
    for (auto const &[datacenter, addresses] : *servers) {
        std::cout << "\t-> " << datacenter << '\n';
        for (std::string const &addr : addresses) {
            std::cout << "\t\t- " << addr << '\n';
        }
    }
    std::cout << std::endl;
}


// ------- COMMANDS




/**
 * @brief Executes the commands specified in the file.
 *
 * @param file File name.
 */
 void executeFile(std::string file) {
     std::ifstream commands(file);

     if (commands.is_open()) {
         std::string cmd;
         while (commands.good()) {
            commands >> cmd;
            if (cmd == "probe") {

            }
         }

     } else {
         std::cerr << "No commands file found with the given name" << std::endl;
     }
 }


/**
 * @brief Frontend program to serve as a means of communication with the real
 * client, through a Redis database.
 * 
 * @return int 
 */
int main(int argc, char *argv[]) {
    if (argc != EXPECTED_ARGS_N && argc != EXPECTED_ARGS_N_FILE) {
        std::cerr << "Invalid number of arguments. Please, specify the Redis address and optionally a file with commands to execute." << std::endl;
        return -1;
    }

    // Execute commands inside the file passed as input
    if (argc == EXPECTED_ARGS_N_FILE) {
        executeFile(argv[EXPECTED_ARGS_N_FILE - 1]);
    }

    bool consumed = false;
    auto servers = new std::map<std::string, std::vector<std::string>>();
    auto logs = new std::map<std::string, std::vector<std::string>>();
    auto probing = new std::map<int, std::vector<std::string>>();
    sw::redis::Redis *redis;

    try {
        sw::redis::ConnectionOptions config;
        config.host = argv[1];
        config.port = REDIS_EXTERNAL_PORT;
        redis = new sw::redis::Redis(config);

    } catch (const std::exception &e) {
        std::cout << "Error while starting up Redis: " << e.what() << std::endl;
    }

    // Redis subscriber
    sw::redis::Subscriber sub = redis->subscriber();
    sub.subscribe("to-client");

    sub.on_message([&consumed, &servers, &logs, &probing, &redis](std::string channel, std::string msg) {
        if (!msg.compare("benchmarks")) {
            std::vector<std::string> *logsList = new std::vector<std::string>();
            redis->lrange("logs", 0, -1, std::back_inserter(*logsList));
            readLogs(logs, logsList);

        } else if (!msg.compare("probing")) {
            probing->clear();
            std::vector<std::string> *probingRes = new std::vector<std::string>();
            redis->lrange("probe", 0, -1, std::back_inserter(*probingRes));
            readProbing(probing, probingRes);

        } else if (!msg.compare("servers")) {
            std::vector<std::string> *serverList = new std::vector<std::string>();
            redis->lrange("serverList", 0, -1, std::back_inserter(*serverList));
            readServers(servers, serverList);
            printServers(servers);
        }

        consumed = true;
    });

    while (true) {
        std::string cmd, address, duration, target;
        std::cin >> cmd;

        if (!cmd.compare("begin")) {
            std::cin >> duration;
            begin(redis, duration);
        
        } else if (!cmd.compare("dump")) {
            std::cin >> target;
            if (!target.compare("probing")) {
                dumpProbing(probing, target);

            } else if (!target.compare("logs")) {
                dumpLogs(logs, target);

            } else {
                std::cerr << "Invalid probing target." << std::endl;
            }

        } else if (!cmd.compare("fetch")) {
            fetch(redis, &sub, &consumed);

        } else if (!cmd.compare("probe")) {
            std::cin >> address;
            std::cin >> duration;
            probe(redis, &sub, &consumed, address, duration);

        } else if (!cmd.compare("get-servers")) {
            getServers(redis, &sub, &consumed);

        } else if (!cmd.compare("exit")) {
            std::cout << "Exiting :)" << std::endl;
            delete redis;
            delete logs;
            delete probing;
            return 0;

        } else {
            std::cerr << "Invalid command specified.\n" << std::endl;
        }
    }
}