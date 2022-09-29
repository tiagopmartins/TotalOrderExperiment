#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>

#include <sw/redis++/redis++.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

const int EXPECTED_ARGS_N = 2;      // Expected number of arguments passed to the program

const int REDIS_EXTERNAL_PORT = 30000;
//const int REDIS_EXTERNAL_PORT = 6379;


// STATISTICS

/**
 * Calculates the average value of a vector.
 *
 * @param vec
 * @return
 */
double averageValue(std::vector<std::string> *vec) {
    double sum = 0;
    for (std::string val : *vec) {
        sum += strtol(val.c_str(), nullptr, 10);
    }

    return sum / vec->size();
}

/**
 * Calculates the standard deviation of a vector.
 *
 * @param vec
 * @return
 */
double stdDeviation(std::vector<std::string> *vec) {
    int average = averageValue(vec);
    double sum = 0;
    for (std::string val : *vec) {
        sum += pow((strtol(val.c_str(), nullptr, 10) - average), 2);
    }

    return sqrt(sum / vec->size());
}


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
 * @param file File to dump the contents into.
 */
void dumpLogs(std::map<std::string, std::vector<std::string>> *logs, std::string file) {
    std::ofstream output(file + ".txt");
    for (auto const &[address, vector] : *logs) {
        output << "-> " << address << '\n';
        for (std::string const &value : vector) {
            output << '\t' << value << '\n';
        }
    }
}


// ------- PROBING

/**
 * @brief Reads the probing results for each server divided by its respective
 * address.
 *
 * @param probing Vector divided into vectors representing each second,
 * containing the results inside that same second.
 * @param probingList List of strings containing data.
 *      Form: ["SECOND$1", "value1", "value2", ..., "SECOND$2", ...]
 */
void readProbing(std::vector<std::vector<std::string>> *probing, std::vector<std::string> *probingList) {
    int currentSecond = 0;
    for (auto it = probingList->begin(); it != probingList->end(); it++) {
        std::string token;
        std::stringstream ss(*it);
        getline(ss, token, '$');

        if (!token.compare("SECOND")) {
            probing->push_back(std::vector<std::string>());
            getline(ss, token, ' ');    // get the new second
            currentSecond = std::atoi(token.c_str());
            continue;
        }

        probing->at(currentSecond - 1).push_back(token);   // push probing value
    }
}

/**
 * Dumps the probing results from the vector given as input into the specified file.
 *
 * @param probing Probing vector, divided into vectors representing each second.
 * @param file File to dump the contents into.
 */
void dumpProbing(std::vector<std::vector<std::string>> *probing, std::string file) {
    std::ofstream output(file + ".txt");
    std::ofstream jsonFile(file + ".json");
    json j;

    int second = 1;
    for (auto const &perSecondValues : *probing) {
        output << "-> " << second << "s\n";
        for (std::string const &value : perSecondValues) {
            output << '\t' << value << '\n';
        }

        output << '\n';
        output << "\tAverage: " << averageValue(&(probing->at(second - 1))) << '\n';
        output << "\tStandard deviation: " << stdDeviation(&(probing->at(second - 1))) << '\n';
        output << std::endl;

        // Send the values into a JSON object
        json jValues(perSecondValues);
        j[std::to_string(second)] = jValues;

        second++;
    }

    jsonFile << j << std::endl;
}


// SERVERS

/**
 * @brief Prints the servers that are active.
 *
 * @param servers List containing the addresses of the servers.
 */
void printServers(std::vector<std::string> *servers) {
    std::cout << "Servers:" << std::endl;
    for (std::string const &address : *servers) {
        std::cout << "\t- " << address << '\n';
    }
    std::cout << std::endl;
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
    auto logs = new std::map<std::string, std::vector<std::string>>();
    auto probing = new std::vector<std::vector<std::string>>();
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

    sub.on_message([&consumed, &logs, &probing, &redis](std::string channel, std::string msg) {
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
            printServers(serverList);
        }

        consumed = true;
    });

    while (true) {
        std::string cmd, address, duration, target;
        std::cin >> cmd;

        if (!cmd.compare("begin")) {
            std::cin >> duration;
            redis->publish("to-exp", "begin " + duration);
            std::cout << std::endl;
        
        } else if (!cmd.compare("dump")) {
            std::cin >> target;
            if (!target.compare("probing")) {
                dumpProbing(probing, target);

            } else if (!target.compare("logs")) {
                dumpLogs(logs, target);
            }

            std::cout << "Successfully dumped contents into file.\n" << std::endl;

        } else if (!cmd.compare("fetch")) {
            redis->publish("to-exp", "fetch");
            waitConsume(&sub, &consumed);
            std::cout << "Successfully fetched messages.\n" << std::endl;

        } else if (!cmd.compare("probe")) {
            std::cin >> address;
            std::cin >> duration;

            std::string msg = "probe ";
            msg.append(address);
            msg.append(" ");
            msg.append(duration);

            redis->publish("to-exp", msg);
            waitConsume(&sub, &consumed);
            std::cout << "Successfully obtained probing results.\n" << std::endl;

        } else if (!cmd.compare("get-servers")) {
            redis->publish("to-exp", "get-servers");
            waitConsume(&sub, &consumed);

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