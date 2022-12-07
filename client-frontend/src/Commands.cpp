#include <iostream>
#include <fstream>
#include <utility>
#include <string>
#include <nlohmann/json.hpp>

#include "Commands.h"

using json = nlohmann::json;

void execute(sw::redis::Redis *redis) {
    redis->publish("to-exp", "execute");
    std::cout << "Successfully sent execute request.\n" << std::endl;
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

void dumpLogs(std::map<std::string, std::vector<std::string>> *logs, std::string file) {
    std::ofstream output(file + ".txt");
    for (auto const &[address, vector] : *logs) {
        output << "-> " << address << '\n';
        for (std::string const &value : vector) {
            output << '\t' << value << '\n';
        }
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

/*
std::map<std::string, float>* toAccuracy(std::map<std::string, std::vector<std::string>> *logs, std::string *sequencer) {
    std::vector<std::string> *seqOrder = &(logs->at(*sequencer));
    auto accuracy = new std::map<std::string, float>();

    for (auto &[address, log] : *logs) {
        long serverAcc = 0; // Sum of all the subsequences found
        std::string subSequence = "";

        ssize_t i = 0;
        while (i < log.size()) {
            // Search for the message in the sequencer's log
            ssize_t s = 0;
            for (; s < seqOrder->size(); s++) {
                if (seqOrder->at(s) == log[i]) {
                    break;
                }
            }

            // Compare sub-sequence
            while (i < log.size() && s < seqOrder->size() && log[i] == seqOrder->at(s)) {
                subSequence.append(log[i]);
                i++;
                s++;
            }

            // First/last message in the correct position or a sequence with size greater than one
            // For a subsequence to count, it needs to be at the correct position or after it
            if ((subSequence.size() > 1 && i >= s) || subSequence == log[0] || subSequence == log[log.size() - 1]) {
                serverAcc += subSequence.size();
            }

            subSequence.clear();
        }

        accuracy->insert({address, ((float) serverAcc / log.size()) * 100});
    }

    return accuracy;
}
*/

std::pair<int, float> toAnalysis(std::map<int, std::vector<std::string>> *logs) {
    int bestId = 0;
    float bestMean = 0;
    for (auto &[toId, toLog] : *logs) {
        float sum = 0;
        for (auto &[id, log] : *logs) {
            if (id == toId) {
                continue;
            }
            sum += toAccuracy(&toLog, &log);
        }

        float mean = sum / (logs->size() - 1);
        if (mean > bestMean) {
            bestMean = mean;
            bestId = toId;
        }
    }

    return std::make_pair(bestId, bestMean);
}

float toAccuracy(std::vector<std::string> *toLog, std::vector<std::string> *log) {
    // Positions of the elements inside toLog
    std::map<std::string, int> toPos = std::map<std::string, int>();
    for (size_t i = 0; i < toLog->size(); i++) {
        toPos.insert({toLog->at(i), i});
    }

    // Evaluate every log entry
    int allPoints = 0;
    for (size_t i = 0; i < log->size(); i++) {
        // Search for it's position
        size_t j = 0;
        for (; j < toLog->size(); j++) {
            if (log->at(i) == toLog->at(j)) {
                break;
            }
        }

        // Sum the points: every message that comes correctly after the one being
        // examined earns a point
        int points = 0;
        for (size_t k = i + 1; k < log->size(); k++) {
            if (toPos[log->at(k)] >= j) {
                points++;
            }
        }
        allPoints += points;
    }

    // Percentage of "total order"
    return ((float) allPoints / ((log->size() - 1) * log->size() / 2));
}