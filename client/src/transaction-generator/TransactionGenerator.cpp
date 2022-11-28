#include <vector>
#include <map>
#include <string>
#include <math.h>
#include <algorithm>
#include <random>
#include <chrono>

#include "TransactionGenerator.h"


TransactionGenerator::TransactionGenerator(int n, int serverN, int transactionKeyN, double zipfAlpha) :
        keyN(n), serverN(serverN), transactionKeyN(transactionKeyN) {
    this->generator = new ZipfGenerator(zipfAlpha, n);
    this->keys = new std::map<long, int>();
    distributeKeys();
}

TransactionGenerator::~TransactionGenerator() {
    delete this->generator;
    delete this->keys;
}

int TransactionGenerator::keyServer(long key) {
    return this->keys->at(key);
}

std::vector<long>* TransactionGenerator::transaction() {
    std::vector<long> *transactionKeys = new std::vector<long>();
    int i = 0;
    while (i < this->transactionKeyN) {
        int key = this->generator->next();
        // Not a duplicate key
        if (std::find(transactionKeys->begin(), transactionKeys->end(), key) == transactionKeys->end()) {
            transactionKeys->push_back(key);
            i++;
        }
    }

    return transactionKeys;
}

void TransactionGenerator::distributeKeys() {
    long partitionSize = ceil((float) this->keyN / this->serverN);

    // Shuffle the keys to perform load-balancing
    std::vector<long> keyList = std::vector<long>();
    for (size_t i = 0; i < this->keyN; i++) {
        keyList.push_back(i);
    }
    std::shuffle(keyList.begin(), keyList.end(),
                 std::default_random_engine(std::chrono::system_clock::now().time_since_epoch().count()));

    int id = 0;
    while (id < this->serverN) {
        for (size_t i = partitionSize * id; i < partitionSize * (id + 1) && i < this->keyN; i++) {
            this->keys->insert({keyList[i], id});
        }
        id++;
    }
}