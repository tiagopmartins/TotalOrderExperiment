#include <vector>
#include <map>
#include <string>
#include <math.h>
#include <algorithm>
#include <random>
#include <chrono>

#include "TransactionGenerator.h"


TransactionGenerator::TransactionGenerator(int n, int serverN) :
        keyN(n), serverN(serverN) {
    this->generator = new ZipfGenerator(this->ZIPF_ALPHA, n);
    this->keys = new std::map<long, int>();
    matchKeys();
}

TransactionGenerator::~TransactionGenerator() {
    delete this->generator;
    delete this->keys;
}

int TransactionGenerator::keyServer(long key) {
    return this->keys->at(key);
}

std::vector<long>* TransactionGenerator::transaction() {
    srand(time(nullptr));
    int keyNumber = rand() % this->KEY_MAX + 1;
    std::vector<long> *transactionKeys = new std::vector<long>(keyNumber);

    for (int i = 0; i < keyNumber; i++) {
        transactionKeys->at(i) = this->generator->next();
    }

    return transactionKeys;
}

void TransactionGenerator::matchKeys() {
    //long partitionSize = ceil((float) this->keyN / this->serverN);
    long partitionSize = ceil((float) this->keyN / 4);

    // Shuffle the keys to perform load-balancing
    std::vector<long> keyList = std::vector<long>();
    for (size_t i = 0; i < this->keyN; i++) {
        keyList.push_back(i);
    }
    std::shuffle(keyList.begin(), keyList.end(),
                 std::default_random_engine(std::chrono::system_clock::now().time_since_epoch().count()));

    int id = 0;
    while (id < 4) {
        for (size_t i = partitionSize * id; i < partitionSize * (id + 1) && i < this->keyN; i++) {
            this->keys->insert({keyList[i], id});
        }
        id++;
    }
}