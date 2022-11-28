#ifndef TRANSACTION_SIMULATOR_H
#define TRANSACTION_SIMULATOR_H

#include <map>
#include <string>
#include <vector>

#include "../zipf/ZipfGenerator.h"

/**
 * @brief Generator for transactions between the client and the server.
 */
class TransactionGenerator {

private:
    long KEY_MAX = 4;           // Maximum number of keys accessed
    double ZIPF_ALPHA = 1;      // Zipf distribution parameter

    long keyN;
    int serverN;
    ZipfGenerator *generator;   // Probabilities generator

    std::map<long, int> *keys;  // Mapping between keys and server IDs

public:

    /**
     * Constructor. Constructs the probability table based on a Zipf distribution.
     *
     * @param n Number of transactions.
     * @param serverN Number of servers.
     */
    TransactionGenerator(int n, int serverN);

    ~TransactionGenerator();

    /**
     * @brief Gets the correspondent server which stores the specified key.
     *
     * @param key
     * @return Server ID.
     */
    int keyServer(long key);

    /**
     * Generates a mock transaction. This transaction is composed of various
     * randomly-selected keys.
     *
     * @return Key vector.
     */
    std::vector<long>* transaction();

private:

    /**
     * @brief Matches keys to the correspondent server IDs.
     */
    void matchKeys();

};

#endif