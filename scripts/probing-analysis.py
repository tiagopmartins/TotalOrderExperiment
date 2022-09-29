import json
import matplotlib.pyplot as plt

PROBING_FILE = "probing.json"

def parse():
    '''
    Parses the probing results file. Returns a list of lists where each list
    represents a second.
    '''

    with open(PROBING_FILE, 'r') as j:
        contents = json.loads(j.read())

    results = []
    for second in contents:
        # Convert string array to int array
        values = [int(str_value) for str_value in contents[second]]
        results.append(values)
    
    return results


if __name__ == "__main__":
    results = parse()

    fig = plt.figure(figsize=(7, 10))
    
    ax = fig.add_subplot(111)
    ax.set_title('Stability of the GRID5000 network')
    ax.set_xlabel('Time (s)')
    ax.set_ylabel('Round Trip Time (ms)')
    
    bp = ax.boxplot(results, showfliers=False)
    plt.savefig('probing.png')