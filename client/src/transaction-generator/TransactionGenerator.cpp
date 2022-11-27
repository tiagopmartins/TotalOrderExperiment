#include <vector>
#include <map>
#include <string>
#include <math.h>

#include "TransactionGenerator.h"
#include "../zipf/ZipfGenerator.h"

TransactionGenerator::TransactionGenerator(double alpha, int n, int serverN) :
        keyN(n), serverN(serverN) {
    this->generator = new ZipfGenerator(alpha, n);
}

TransactionGenerator::~TransactionGenerator() {
    delete this->generator;
}

int TransactionGenerator::keyServer(long key) {
    long partitionSize = this->keyN / this->serverN;

    // Odd number of keys (0 - keyN): last key is in the last server
    if (key == this->keyN && this->keyN % 2 == 1) {
        return this->serverN - 1;

    } else {
        return floor(key / partitionSize);
    }
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