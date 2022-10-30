import sys
import json
import matplotlib.pyplot as plt
import numpy as np
import statistics
from scipy import stats

# Distance between datacenters (Km)
NANCY_LUXEMBOURG = 134.96
NANCY_LYON = 341.13
NANCY_RENNES = 584.38
NANCY_SOPHIA = 568.35

def parse(probing_file):
    '''
    Parses the probing results file. Returns a list of lists where each list
    represents a second.
    '''

    with open(probing_file, 'r') as j:
        contents = json.loads(j.read())

    results = []
    for second in contents:
        # Convert string array to float array
        values = [float(str_value) for str_value in contents[second]]
        results.append(values)
    
    return results


def compute_median(probing_file):
    '''
    Parses the probing results file and returns the median of all values.
    '''

    with open(probing_file, 'r') as j:
        contents = json.loads(j.read())

    results = []
    for second in contents:
        for value in contents[second]:
            results.append(float(value))
    
    return statistics.median(results)

def boxplot(results):
    fig = plt.figure(figsize=(25, 6))
    
    ax = fig.add_subplot(111)
    ax.set_title('Stability of the GRID5000 network')
    ax.set_xlabel('Time (s)')
    ax.set_ylabel('Round Trip Time (ms)')

    bp = ax.boxplot(results, showfliers=False, widths=0.4, whis=(5, 95))
    plt.xticks(np.round(np.linspace(0, len(results), num=25), 0), np.round(np.linspace(0, len(results), num=25), 0))
    
    plt.savefig('probing.png')

def linear_regression(x, y):
    slope, intercept, r, p, std_err = stats.linregress(x, y)

    def line_equation(value):
        return slope * value + intercept

    line = list(map(line_equation, x))

    plt.scatter(x, y)
    plt.plot(x, line)
    plt.savefig('rtt-dist-relation.png')

if __name__ == "__main__":
    #probing_file = sys.argv[1]

    #results = parse(probing_file)
    #boxplot(results)
    #print(compute_median(probing_file))

    sites_distances = [NANCY_LUXEMBOURG, NANCY_LYON, NANCY_RENNES, NANCY_SOPHIA]
    rtt_medians = [10.17, 34.88, 40.5, 54.35]
    linear_regression(sites_distances, rtt_medians)