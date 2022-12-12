#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>

#include <sw/redis++/redis++.h>

#include "Commands.h"

// Expected number of arguments passed to the program
const int EXPECTED_ARGS_N = 2;
const int EXPECTED_ARGS_N_FILE = 3;

const int REDIS_EXTERNAL_PORT = 30000;
//const int REDIS_EXTERNAL_PORT = 6379;


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

        if (token == "SERVER") {
            getline(ss, token, ' ');    // new server address
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

/**
 * Parses the specified command to be called.
 *
 * @param cmd Command to execute.
 * @param redis
 * @param sub Redis subscriber.
 * @param consumed Pointer to a flag to know if the message was already
 * consumed by the subscriber.
 * @param logs Pointer to a vector containing the logs.
 * @param probing Pointer to a vector containing the probing results.
 * @return true if the program exited.
 */
bool executeCall(std::string cmd, sw::redis::Redis *redis, sw::redis::Subscriber *sub,
                 bool *consumed, std::map<std::string, std::vector<std::string>> *logs,
                 std::map<int, std::vector<std::string>> *probing) {
    std::string address, duration, target, transactionN;

    if (cmd == "execute") {
        std::cin >> transactionN;
        execute(redis, sub, consumed, transactionN);

    } else if (cmd == "dump") {
        std::cin >> target;
        if (target == "probing") {
            dumpProbing(probing, target);

        } else if (target == "logs") {
            dumpLogs(logs, target);

        } else {
            std::cerr << "Invalid probing target.\n" << std::endl;
        }

    } else if (cmd == "fetch") {
        fetch(redis, sub, consumed);

    } else if (cmd == "probe") {
        std::cin >> address;
        std::cin >> duration;
        probe(redis, sub, consumed, address, duration);
        dumpProbing(probing, "probing_" + address);

    } else if (cmd == "get-servers") {
        getServers(redis, sub, consumed);

    } else if (cmd == "exit") {
        std::cout << "Exiting :)" << std::endl;
        return true;

    } else {
        std::cerr << "Invalid command specified.\n" << std::endl;
    }

    return false;
}

/**
 * @brief Executes the commands specified in the file.
 *
 * @param file File name.
 * @param redis
 * @param sub Redis subscriber.
 * @param consumed Pointer to a flag to know if the message was already
 * consumed by the subscriber.
 * @param logs Pointer to a vector containing the logs.
 * @param probing Pointer to a vector containing the probing results.
 * @return true if the program exited.
 */
bool executeFile(std::string file, sw::redis::Redis *redis, sw::redis::Subscriber *sub,
                 bool *consumed, std::map<std::string, std::vector<std::string>> *logs,
                 std::map<int, std::vector<std::string>> *probing) {
    std::ifstream commands(file);

    if (commands.is_open()) {
        while (!commands.eof()) {
            std::string line, operation;
            std::string address, duration, target, transactionN;

            getline(commands, line);
            std::stringstream command(line);
            command >> operation;

            std::cout << line << std::endl;

            if (operation == "execute") {
                command >> transactionN;
                execute(redis, sub, consumed, transactionN);

            } else if (operation == "dump") {
                command >> target;
                if (target == "probing") {
                    dumpProbing(probing, target);

                } else if (target == "logs") {
                    dumpLogs(logs, target);

                } else {
                    std::cerr << "Invalid probing target.\n" << std::endl;
                }

            } else if (operation == "fetch") {
                fetch(redis, sub, consumed);

            } else if (operation == "probe") {
                command >> address;
                command >> duration;
                probe(redis, sub, consumed, address, duration);

            } else if (operation == "get-servers") {
                getServers(redis, sub, consumed);

            } else if (operation == "exit") {
                std::cout << "Exiting :)" << std::endl;
                return true;

            } else {
                std::cerr << "Invalid command specified.\n" << std::endl;
            }
        }

    } else {
        std::cerr << "No commands file found with the given name.\n" << std::endl;
    }

    return false;
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
        std::cout << "Error while starting up Redis: " << e.what() << '\n' << std::endl;
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

    // Execute commands inside the file passed as input
    if (argc == EXPECTED_ARGS_N_FILE) {
        // Execute call and check if the program exited
        if (executeFile(argv[EXPECTED_ARGS_N_FILE - 1], redis, &sub, &consumed, logs, probing)) {
            delete servers;
            delete logs;
            delete probing;

            return 0;
        }
    }

    // TO Accuracy function test
    /*
    std::vector<std::string> toLog = std::vector<std::string>();
    toLog.push_back("A");
    toLog.push_back("B");
    toLog.push_back("C");
    toLog.push_back("D");
    toLog.push_back("E");

    std::vector<std::string> log = std::vector<std::string>();
    log.push_back("A");
    log.push_back("B");
    log.push_back("E");
    log.push_back("C");
    log.push_back("D");

    std::cout << "Acc: " << toAccuracy(&toLog, &log) << std::endl;
    */

    while (true) {
        std::string cmd;
        std::cin >> cmd;

        // Execute call and check if the program exited
        if(executeCall(cmd, redis, &sub, &consumed, logs, probing)) {
            delete servers;
            delete logs;
            delete probing;

            return 0;
        }
    }

    return 0;
}