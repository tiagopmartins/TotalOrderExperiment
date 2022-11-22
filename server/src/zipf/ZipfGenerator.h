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

public:
    std::vector<double> *sum_probs;
    double alpha;
    int n;

    ZipfGenerator(double _alpha, int _n);

    ~ZipfGenerator();

    /**
     * Cumulative function of a discrete Zipf distribution.
     */
    void zipf();

    /**
     * Cumulative function of a normal distribution.
     *
     * @param mean
     * @param stddev_sqrd Squared standard deviation.
     */
    void normal(double mean, double stddev_sqrd);

    int next();

private:
    /**
     * Mean of n consecutive numbers.
     *
     * @param n
     * @return
     */
    double mean(int n);

    /**
     * Squared standard deviation of n consecutive numbers.
     *
     * @param n
     * @return
     */
    double stdDeviationSquared(int n);

    /**
     * Factorial function.
     *
     * @param n
     * @return
     */
     long factorial(int n);

};

#endif //ZIPF_GENERATOR_H
