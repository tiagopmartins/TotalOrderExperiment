import random
import bisect
import math
from functools import reduce
import json
import os
import numpy

class LoadGenerator:

    def __init__(self, n, alpha, unique_zipf=False):
        if alpha > 0:
            self.generator = ZipfGenerator(n, alpha)
        else:
            self.generator = UniformGenerator(n)
        self.key_mapping = list(range(0, n))
        self.unique_zipf = unique_zipf

        if unique_zipf:
            random.seed(5)
            random.shuffle(self.key_mapping)

    def next(self):
        if self.unique_zipf:
            return self.key_mapping[self.generator.next()]

        return self.generator.next()

class ZipfGenerator:

    def __init__(self, n, alpha):
        tmp = numpy.power(numpy.arange(1, n + 1), -float(alpha))
        zeta = numpy.r_[0.0, numpy.cumsum(tmp)]

        # Store the translation map:
        self.distMap = [x / zeta[-1] for x in zeta]

    def next(self):
        # Take a uniform 0-1 pseudo-random value:
        u = random.random()

        # Translate the Zipf variable:
        return bisect.bisect(self.distMap, u) - 1

class UniformGenerator:

    def __init__(self, n):
        self.db_size = n

    def next(self):
        return random.randint(0, self.db_size - 1)
