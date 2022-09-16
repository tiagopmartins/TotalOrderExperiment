#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <map>

#include <sw/redis++/redis++.h>

const int EXPECTED_ARGS_N = 2;      // Expected number of arguments passed to the program

const int REDIS_EXTERNAL_PORT = 30000;

// ------- DATA LOGS

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
            logs->insert(std::pair<std::string, std::vector<std::string>>(token, std::vector<std::string>()));
            currentServer = token;
            continue;
        }

        logs->at(currentServer).push_back(token);   // push log info
    }
}

/**
 * @brief Prints the logs and statistics for each server in a human-friendly way.
 *
 * @param logs Map between the server address and a vector of logs for it.
 */
void printLogs(std::map<std::string, std::vector<std::string>> *logs) {
    for (auto const &[address, vector] : *logs) {
        std::cout << "=> " << address << '\n';
        for (std::string const &value : vector) {
            std::cout << "\t- " << value << '\n';
        }
        std::cout << std::endl;
    }
}


// ------- PROBING

/**
 * @brief Reads the probing results for each server divided by its respective
 * address.
 *
 * @param probing Map between the server address and a vector divided into vectors
 * representing each second, containing the results inside that same second.
 * @param probingList List of strings containing data.
 *      Form: ["SERVER$address1", "SECOND$1", "value1", "value2", ..., "SECOND$2", ..., "SERVER$address2", ...]
 */
void readProbing(std::map<std::string, std::vector<std::vector<std::string>>> *probing, std::vector<std::string> *probingList) {
    std::string currentServer = "";
    int currentSecond = 0;
    for (auto it = probingList->begin(); it != probingList->end(); it++) {
        std::string token;
        std::stringstream ss(*it);
        getline(ss, token, '$');

        if (!token.compare("SERVER")) {
            getline(ss, token, ' ');    // get new server address
            probing->insert({token, std::vector<std::vector<std::string>>()});
            currentServer = token;
            continue;

        } else if (!token.compare("SECOND")) {
            getline(ss, token, ' ');    // get the new second
            probing->at(currentServer).push_back(std::vector<std::string>());
            currentSecond = std::atoi(token.c_str());
            continue;
        }

        probing->at(currentServer)[currentSecond - 1].push_back(token);   // push probing value
    }
}

/**
 * @brief Prints the probing values for each server and for each second.
 *
 * @param probing Map between the server address and a vector of probing values,
 * divided by seconds.
 */
void printProbing(std::map<std::string, std::vector<std::vector<std::string>>> *probing) {
    for (auto const &[address, vector] : *probing) {
        std::cout << "=> " << address << '\n';
        int second = 1;
        for (std::vector<std::string> const &values : vector) {
            std::cout << "\t(" << second << "s)\n";
            for (std::string const &value : values) {
                std::cout << "\t- " << value << "s\n";
            }
        }
        second++;
        std::cout << std::endl;
    }
}


/**
 * @brief Waits for a subscriber to consume a message.
 * 
 * @param sub 
 * @param consumed Flag to know if the output was consumed.
 */
void waitConsume(sw::redis::Subscriber *sub, bool *consumed) {
    do {
        try {
            sub->consume();
        
        } catch (const sw::redis::Error &err) {
            std::cerr << "Redis error: " << err.what() << std::endl;
        }

    } while (!(*consumed));

    *consumed = false;  // reset flag
}

/**
 * @brief Frontend program to serve as a means of communication with the real
 * client, through a Redis database.
 * 
 * @return int 
 */
int main(int argc, char *argv[]) {
    if (argc != EXPECTED_ARGS_N) {
        std::cerr << "Invalid number of arguments. Please, specify the Redis address." << std::endl;
        return -1;
    }

    bool consumed = false;
    std::map<std::string, std::vector<std::string>> *logs = new std::map<std::string, std::vector<std::string>>();
    std::map<std::string, std::vector<std::vector<std::string>>> *probing = new std::map<std::string, std::vector<std::vector<std::string>>>();
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
    sub.subscribe("to-exp");

    sub.on_message([&consumed, &logs, &probing, &redis](std::string channel, std::string msg) {
        if (!msg.compare("benchmarks")) {
            std::vector<std::string> *logsList = new std::vector<std::string>();
            redis->lrange("logs", 0, -1, std::back_inserter(*logsList));

            readLogs(logs, logsList);
            printLogs(logs);

        } else if (!msg.compare("probing")) {
            std::vector<std::string> *probingRes = new std::vector<std::string>();
            redis->lrange("probe", 0, -1, std::back_inserter(*probingRes));

            readProbing(probing, probingRes);
            printProbing(probing);
        }

        consumed = true;
    });

    while (true) {
        std::string cmd, duration;
        std::cin >> cmd;

        if (!cmd.compare("begin")) {
            std::cin >> duration;
            redis->publish("to-exp", "begin " + duration);
        
        } else if (!cmd.compare("fetch")) {
            redis->publish("to-exp", "fetch");
            waitConsume(&sub, &consumed);

        } else if (!cmd.compare("probe ")) {
            std::cin >> duration;
            redis->publish("to-exp", "probe " + duration);
            waitConsume(&sub, &consumed);

        } else if (!cmd.compare("exit")) {
            delete redis;
            delete logs;
            delete probing;
            return 0;
        
        } else {
            std::cerr << "Invalid command specified.\n" << std::endl;
        }
    }

    return 0;
}