#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>

#include "Commands.h"

using json = nlohmann::json;

void begin(sw::redis::Redis *redis, std::string duration) {
    redis->publish("to-exp", "begin " + duration);
    std::cout << "Successfully sent begin signal.\n" << std::endl;
}

void fetch(sw::redis::Redis *redis, sw::redis::Subscriber *sub, bool *consumed) {
    redis->publish("to-exp", "fetch");
    waitConsume(sub, consumed);
    std::cout << "Successfully fetched messages.\n" << std::endl;
}

void probe(sw::redis::Redis *redis, sw::redis::Subscriber *sub, bool *consumed, std::string address, std::string duration) {
    // Building the message to send
    std::string msg = "probe ";
    msg.append(address);
    msg.append(" ");
    msg.append(duration);

    redis->publish("to-exp", msg);
    waitConsume(sub, consumed);
    std::cout << "Successfully obtained probing results.\n" << std::endl;
}

void getServers(sw::redis::Redis *redis, sw::redis::Subscriber *sub, bool *consumed) {
    redis->publish("to-exp", "get-servers");
    waitConsume(sub, consumed);
}

void dumpLogs(std::map<std::string, std::vector<std::string>> *logs, std::string *sequencer, std::string file) {
    std::map<std::string, float>* accuracy = toAccuracy(logs, sequencer);

    std::ofstream output(file + ".txt");
    for (auto const &[address, vector] : *logs) {
        output << "-> " << address << '\n';
        for (std::string const &value : vector) {
            output << '\t' << value << '\n';
        }

        float acc = 100;
        if (address != *sequencer) {
            acc = accuracy->at(address);
        }
        output << '\n' << "\tAccuracy: " << acc << '%' << std::endl;
    }
    std::cout << "Successfully dumped contents into file.\n" << std::endl;
}

void dumpProbing(std::map<int, std::vector<std::string>> *probing, std::string file) {
    std::ofstream output(file + ".txt");
    std::ofstream jsonFile(file + ".json");
    json j;

    for (auto const &[second, perSecondValues] : *probing) {
        output << "-> " << second << "s\n";
        for (std::string const &value : perSecondValues) {
            output << '\t' << value << '\n';
        }

        // Send the values into a JSON object
        json jValues(perSecondValues);
        j[std::to_string(second)] = jValues;
    }

    jsonFile << j << std::endl;
    std::cout << "Successfully dumped contents into file.\n" << std::endl;
}

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

std::map<std::string, float>* toAccuracy(std::map<std::string, std::vector<std::string>> *logs, std::string *sequencer) {
    std::vector<std::string> *seqOrder = &(logs->at(*sequencer));
    auto accuracy = new std::map<std::string, float>();

    for (auto &[address, log] : *logs) {
        long serverAcc = 0;
        for (ssize_t i = 0; i < log.size(); i++) {
            // Match between orders
            if (log.at(i) == seqOrder->at(i)) {
                serverAcc++;
            }
        }
        accuracy->insert({address, (serverAcc / log.size()) * 100});
    }

    return accuracy;
}