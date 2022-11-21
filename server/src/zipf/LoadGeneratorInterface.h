//
// Created by jrsoares on 17-10-2022.
//

#ifndef LOAD_GENERATOR_INTERFACE_H
#define LOAD_GENERATOR_INTERFACE_H


class LoadGeneratorInterface {

public:
    virtual int next() = 0;

    double rand_val(int seed) {
        const long  a =      16807;  // Multiplier
        const long  m = 2147483647;  // Modulus
        const long  q =     127773;  // m div a
        const long  r =       2836;  // m mod a
        static long x =  749275725;  // Random int value
        long        x_div_q;         // x divided by q
        long        x_mod_q;         // x modulo q
        long        x_new;           // New x value

        // Set the seed if argument is non-zero and then return zero
        if (seed > 0) {
            x = seed;
            return 0.0;
        }

        // RNG using integer arithmetic
        x_div_q = x / q;
        x_mod_q = x % q;
        x_new = (a * x_mod_q) - (r * x_div_q);
        if (x_new > 0)
            x = x_new;
        else
            x = x_new + m;

        // Return a random value between 0.0 and 1.0
        return ((double) x / m);
    }
};


#endif //LOAD_GENERATOR_INTERFACE_H