//
// Created by jrsoares on 17-10-2022.
// Based on these implementations:
//      https://cse.usf.edu/~kchriste/tools/genzipf.c
//      https://stackoverflow.com/questions/9983239/how-to-generate-zipf-distributed-numbers-efficiently
//

#include "ZipfGenerator.h"
#include <math.h>               // Needed for pow()
#include <assert.h>             // Needed for assert() macro
#include <random>               // Needed for normal_distribution class
#include <algorithm>            // Needed for shuffle()

#include <iostream>


ZipfGenerator::ZipfGenerator(double _alpha = 1.0, int _n = 1000) : alpha(_alpha), n(_n) {
    sum_probs = new std::vector<double>(n + 1);
    // Null alpha is equivalent to a normal distribution
    if (alpha > 0) {
        zipf();

    } else {
        normal(mean(n), stdDeviationSquared(n));
    }

    // Shuffle keys for a better load-balancing
    std::shuffle(sum_probs->begin(), sum_probs->end(), std::default_random_engine(rand_val(0)));
}

ZipfGenerator::~ZipfGenerator() {
    delete sum_probs;
}

void ZipfGenerator::zipf() {
    double c = 0;
    for (int i = 1; i <= n; i++) {
        c = c + (1.0 / pow((double) i, alpha));
    }
    c = 1.0 / c;

    sum_probs->at(0) = 0;
    for (int i = 1; i <= n; i++) {
        sum_probs->at(i) = sum_probs->at(i - 1) + c / pow((double) i, alpha);
    }
}

void ZipfGenerator::normal(double mean, double stddev_sqrd) {
    sum_probs->at(0) = 0;
    for (int i = 1; i <= n; i++) {
        //sum_probs->at(i) = 0.5 * (1 + erf((i - mean) / sqrt(2 * stddev_sqrd)));

        double pdf = 1 / (sqrt(2 * M_PI * stddev_sqrd)) * exp(-0.5 * pow((i - mean) / sqrt(stddev_sqrd), 2));
        sum_probs->at(i) = sum_probs->at(i - 1) + pdf;

        // Poisson approximation
        //sum_probs->at(i) = sum_probs->at(i - 1) + (pow(rate, i) * exp(-n)) / factorial(i);

        std::cout << i << ": " << sum_probs->at(i) << std::endl;
    }
}

int ZipfGenerator::next() {
    double z;                           // Uniform random number (0 < z < 1)
    int low, high, mid, zipf_value;     // Binary-search bounds

    do {
        z = rand_val(0);
    } while ((z == 0) || (z == 1));

    // Map z to the value
    low = 1, high = n;
    do {
        mid = floor((low + high) / 2);
        if (sum_probs->at(mid) >= z && sum_probs->at(mid - 1) < z) {
            zipf_value = mid;
            break;

        } else if (sum_probs->at(mid) >= z) {
            high = mid - 1;

        } else {
            low = mid + 1;
        }
    } while (low <= high);

    // Assert that zipf_value is between 1 and N
    assert((zipf_value >= 1) && (zipf_value <= n));

    return (zipf_value - 1);
}

double ZipfGenerator::mean(int n) {
    return (1 + n) / 2.0;
}

double ZipfGenerator::stdDeviationSquared(int n) {
    double m = mean(n), sum = 0;
    for (double i = 1; i <= n; i++) {
        sum += pow((i - m), 2);
    }
    return sum / n;
}

long ZipfGenerator::factorial(int n) {
    long value = 1;
    for (int i = n; i > 0; i--) {
        value *= i;
    }
    return value;
}