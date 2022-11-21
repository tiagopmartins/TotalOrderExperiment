//
// Created by jrsoares on 17-10-2022.
// Based on these implementations:
//      https://cse.usf.edu/~kchriste/tools/genzipf.c
//      https://stackoverflow.com/questions/9983239/how-to-generate-zipf-distributed-numbers-efficiently
//

#ifndef ZIPF_GENERATOR_H
#define ZIPF_GENERATOR_H

#include "LoadGeneratorInterface.h"


class ZipfGenerator : public LoadGeneratorInterface {

public:
    double *sum_probs;
    double alpha;
    int n;

    ZipfGenerator(double _alpha, int _n);

    /**
     * Cumulative function of a discrete zipf distribution.
     */
    void zipf();

    /**
     * Cumulative function of a discrete normal distribution.
     */
    void normal(float mean, float stddev);

    int next();

};

#endif //ZIPF_GENERATOR_H
