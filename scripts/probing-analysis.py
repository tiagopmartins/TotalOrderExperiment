import sys
import json
import matplotlib.pyplot as plt
import numpy as np

def parse(probing_file):
    '''
    Parses the probing results file. Returns a list of lists where each list
    represents a second.
    '''

    with open(probing_file, 'r') as j:
        contents = json.loads(j.read())

    results = []
    for second in contents:
        # Convert string array to int array
        values = [float(str_value) for str_value in contents[second]]
        results.append(values)
    
    return results

if __name__ == "__main__":
    probing_file = sys.argv[1]
    results = parse(probing_file)

    fig = plt.figure(figsize=(25, 6))
    
    ax = fig.add_subplot(111)
    ax.set_title('Stability of the GRID5000 network')
    ax.set_xlabel('Time (s)')
    ax.set_ylabel('Round Trip Time (ms)')

    bp = ax.boxplot(results, showfliers=False, widths=0.4)
    plt.xticks(np.round(np.linspace(0, len(results), num=25), 0), np.round(np.linspace(0, len(results), num=25), 0))
    
    plt.savefig('probing.png')