//
// Created by jrsoares on 17-10-2022.
// Based on these implementations:
//      https://cse.usf.edu/~kchriste/tools/genzipf.c
//      https://stackoverflow.com/questions/9983239/how-to-generate-zipf-distributed-numbers-efficiently
//

#ifndef ZIPF_GENERATOR_H
#define ZIPF_GENERATOR_H

#include "LoadGeneratorInterface.h"
#include <vector>


class ZipfGenerator : public LoadGeneratorInterface {

private:
    std::vector<double> *sum_probs;
    double alpha;
    int n;

public:

    ZipfGenerator(double _alpha, int _n);

    ~ZipfGenerator();

    std::vector<double>* sumProbs();

    /**
     * Cumulative function of a discrete Zipf distribution.
     */
    void zipf();

    /**
     * Selects the next value for the next key.
     *
     * @return
     */
    int next();

};

#endif //ZIPF_GENERATOR_H
