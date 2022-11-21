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
#include <iostream>


ZipfGenerator::ZipfGenerator(double _alpha = 1.0, int _n = 1000) : alpha(_alpha), n(_n) {
    sum_probs = static_cast<double*>(malloc((n + 1) * sizeof(*sum_probs)));
    // Null alpha is equivalent to a normal distribution
    if (alpha > 0) {
        zipf();

    } else {
        normal(0, 1);
    }
}

void ZipfGenerator::zipf() {
    double c = 0;
    for (int i = 1; i <= n; i++) {
        c = c + (1.0 / pow((double) i, alpha));
    }
    c = 1.0 / c;

    sum_probs[0] = 0;
    for (int i = 1; i <= n; i++) {
        sum_probs[i] = sum_probs[i - 1] + c / pow((double) i, alpha);
    }
}

void ZipfGenerator::normal(float mean, float stddev) {
    sum_probs[0] = 0;
    for (int i = 1; i <= n; i++) {
        //float pdf = 1 / (stddev * sqrt(2 * M_PI)) * exp(-0.5 * pow((i - mean) / stddev, 2));
        //sum_probs[i] = sum_probs[i - 1] + pdf;

        sum_probs[i] = 0.5 * (1 + erf(i / sqrt(2)));

        std::cout << "prob: " << sum_probs[i] << std::endl;
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
        if (sum_probs[mid] >= z && sum_probs[mid - 1] < z) {
            zipf_value = mid;
            break;

        } else if (sum_probs[mid] >= z) {
            high = mid - 1;

        } else {
            low = mid + 1;
        }
    } while (low <= high);

    // Assert that zipf_value is between 1 and N
    assert((zipf_value >= 1) && (zipf_value <= n));

    return (zipf_value - 1);
}