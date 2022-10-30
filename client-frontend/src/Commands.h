#ifndef TOTAL_ORDER_EXPERIMENT_COMMANDS_H
#define TOTAL_ORDER_EXPERIMENT_COMMANDS_H

#include <string>
#include <sw/redis++/redis++.h>

/**
 * Sends a begin signal through Redis for the message exchange to start.
 *
 * @param redis
 * @param duration
 */
void begin(sw::redis::Redis *redis, std::string duration);

/**
 * Sends a fetch signal through Redis to get the results from the "begin".
 *
 * @param redis
 * @param sub Redis subscriber.
 * @param consumed Pointer to a consumed flag to know if the message was
 * already consumed by the subscriber.
 */
void fetch(sw::redis::Redis *redis, sw::redis::Subscriber *sub, bool *consumed);

/**
 * Sends a probing signal to the client through Redis to probe the network and
 * obtain data on its latency.
 *
 * @param redis
 * @param address Address to send messages to.
 * @param duration Duration of the probing operation.
 * @param sub Redis subscriber.
 * @param consumed Pointer to a consumed flag to know if the message was already
 * consumed by the subscriber.
 */
void probe(sw::redis::Redis *redis, sw::redis::Subscriber *sub, bool *consumed, std::string address, std::string duration);

/**
 * Gets the active servers in the network.
 *
 * @param redis
 * @param sub Redis subscriber.
 * @param consumed Pointer to a consumed flag to know if the message was
 * already consumed by the subscriber.
 */
void getServers(sw::redis::Redis *redis, sw::redis::Subscriber *sub, bool *consumed);

/**
 * @brief Prints the logs and statistics for each server in a human-friendly way.
 *
 * @param logs Map between the server address and a vector of logs for it.
 * @param file File to dump the contents into.
 */
void dumpLogs(std::map<std::string, std::vector<std::string>> *logs, std::string file);

/**
 * Dumps the probing results from the vector given as input into the specified file.
 *
 * @param probing Probing vector, divided into vectors representing each second.
 * @param file File to dump the contents into.
 */
void dumpProbing(std::map<int, std::vector<std::string>> *probing, std::string file);

/**
 * @brief Waits for a subscriber to consume a message.
 *
 * @param sub
 * @param consumed Flag to know if the output was consumed.
 */
void waitConsume(sw::redis::Subscriber *sub, bool *consumed);

#endif //TOTAL_ORDER_EXPERIMENT_COMMANDS_H
