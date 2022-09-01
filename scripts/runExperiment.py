import argparse

import redis
import logging
import sys
import time
from datetime import datetime
import cloudpickle as cp
import yaml

import os

logging.basicConfig(filename='log_trigger.txt', level=logging.INFO,
                    format='%(asctime)s %(message)s')



import numpy as np

#benchmark nodes: 16
#Memory nodes: 50
def getTime():
    return datetime.now().timestamp() * 1000

def load_conf(filename):
    with open(filename) as f:
        return yaml.safe_load(f)

def printSetup(setup, logger):
    logger.info('*** CONFIGURATION ***')
    for category in setup:
        logger.info("%s: " % category)
        for value in setup[category]:
            logger.info("  %s: %s" % (value, str(setup[category][value])))

def print_latency_stats(data, ident, log=False, epoch=0):
    npdata = np.array(data)
    tput = 0

    if epoch > 0:
        tput = len(data) / epoch

    mean = np.mean(npdata)
    median = np.percentile(npdata, 50)
    p75 = np.percentile(npdata, 75)
    p95 = np.percentile(npdata, 95)
    p99 = np.percentile(npdata, 99)
    mx = np.max(npdata)

    p25 = np.percentile(npdata, 25)
    p05 = np.percentile(npdata, 5)
    p01 = np.percentile(npdata, 1)
    mn = np.min(npdata)

    output = ('%s LATENCY:\n\tsample size: %d\n' +
              '\tTHROUGHPUT: %.4f\n'
              '\tmean: %.6f, median: %.6f\n' +
              '\tmin/max: (%.6f, %.6f)\n' +
              '\tp25/p75: (%.6f, %.6f)\n' +
              '\tp5/p95: (%.6f, %.6f)\n' +
              '\tp1/p99: (%.6f, %.6f)') % (ident, len(data), tput, mean,
                                           median, mn, mx, p25, p75, p05, p95,
                                           p01, p99)

    if log:
        log.info(output)
    else:
        print(output)


def setupNamespace(hydroHome):
    path = os.path.join(hydroHome, "cluster")
    os.chdir(path)
    os.environ["HYDRO_HOME"] = hydroHome
    os.system("kubectl create namespace hydro")
    os.system("kubectl create secret docker-registry regcred --namespace=hydro --docker-username=tiagopm --docker-password=bET!pr8bRlPHa7=iPraC")
    os.system("kubectl config set-context --current --namespace=hydro")


def setupKubernetes(deployment, local):
    os.system("python3 -m hydro.cluster.create_cluster -m %s -r %s -f 0 -s 0 -b %s -l %s" % (deployment["memory"], deployment["routing"], deployment["benchmark"], local))


def populate(r,p, bname, workload, total_nodes, create = True):
    msg = "%s:" % bname
    for entry in workload:
        msg += "%s:" % workload[entry]
        if entry == "num_requests":
            msg += "dag:"
    msg += "%s:" % create
    msg += str(total_nodes)
    msg += ":False"

    if create:
        sent_msgs = total_nodes * 3
        start_time = getTime()
        while getTime() - start_time < 20:
            p.get_message()
        r.publish('benchmarkt', msg)
        end_recv = 0


        for msg in p.listen():
            if "subscribe" in msg["type"]:
                continue
            msg = msg['data']
            if b'END' in msg:
                end_recv += 1
                if end_recv >= sent_msgs:
                   break

def run_expermiment(r,p, bname, workload, total_nodes, logging = False, result_log = False, init_operation = False):
    msg = "%s:" % bname
    for entry in workload:
        msg += "%s:" % workload[entry]
        if entry == "num_requests":
            msg += "dag:"
    msg += "False:"
    msg += str(total_nodes)
    msg += ":%s" % str(init_operation)

    #r.publish('benchmark0', msg)
    #r.publish('benchmark1', msg)
    r.publish('benchmarkt', msg)

    epoch_total = []
    total_read = []
    total_write = []
    total_read_rounds = []
    total_statistics = [0, 0, 0]

    end_recv = 0

    epoch_recv = 0
    epoch = 1

    epoch_total_read = []
    epoch_total_write = []
    epoch_total_read_rounds = []
    epoch_total_statistics = [0, 0, 0]

    epoch_throughput_read = []
    epoch_throughput_write = []
    epoch_read_rounds = []
    epoch_statistics = [0, 0, 0]

    epoch_start = time.time()
    sent_msgs = total_nodes*3
    #sent_msgs = total_nodes

    for msg in p.listen():
        if "subscribe" in msg["type"]:
            continue
        msg = msg['data']

        if b'END' in msg:
            end_recv += 1
            if end_recv >= sent_msgs:
                break
        else:
            msg = cp.loads(msg)

            if type(msg) == tuple:
                epoch_throughput_read = msg[0]
                epoch_throughput_write = msg[1]
                epoch_read_rounds = msg[2]
                epoch_statistics = msg[3]

            else:
                new_tot = msg

            epoch_total_read += epoch_throughput_read
            epoch_total_write += epoch_throughput_write
            epoch_total_read_rounds += epoch_read_rounds
            epoch_total_statistics[0] += epoch_statistics[0]
            epoch_total_statistics[1] += epoch_statistics[1]
            epoch_total_statistics[2] += epoch_statistics[2]

            total_read += epoch_throughput_read
            total_write += epoch_throughput_write
            total_read_rounds += epoch_read_rounds
            total_statistics[0] += epoch_statistics[0]
            total_statistics[1] += epoch_statistics[1]
            total_statistics[2] += epoch_statistics[2]
            epoch_recv += 1

            if epoch_recv == sent_msgs:
                epoch_end = time.time()
                elapsed = epoch_end - epoch_start
                if logging:
                    logging.info('\n\n*** EPOCH %d ***' % (epoch))
                    if epoch_throughput_read:
                        logging.info('\tREAD TRANSACTIONS:')
                        read_elapsed = elapsed
                        for tx_latency in epoch_throughput_read:
                            read_elapsed -= tx_latency
                        print_latency_stats(epoch_throughput_read, 'E2E', logging, read_elapsed)
                        read_rounds = 0
                        for read_round in epoch_total_read_rounds:
                            read_rounds += read_round
                        read_rounds = read_rounds / len(epoch_total_read_rounds)
                        logging.info("READ ROUNDS: %s" % read_rounds)
                        logging.info("Average Time on Anna (ms): %.4f " %
                                     ((epoch_total_statistics[0] / len(epoch_total_read_rounds)) / 1000))
                        logging.info("Average Time on Anna Network (ms): %.4f " %
                                     ((epoch_total_statistics[1] / len(epoch_total_read_rounds)) / 1000))
                        logging.info("Average Time on Cache (ms): %.4f " %
                                     ((epoch_total_statistics[2] / len(epoch_total_read_rounds)) / 1000))
                    if epoch_throughput_write:
                        logging.info('\tWRITE TRANSACTIONS:')
                        write_elapsed = elapsed
                        for tx_latency in epoch_throughput_write:
                            write_elapsed -= tx_latency
                        print_latency_stats(epoch_total_write, 'E2E', logging, write_elapsed)

                epoch_recv = 0
                epoch_thruput = 0
                epoch_statistics = [0, 0, 0]
                epoch_total_statistics = [0, 0, 0]
                epoch_total_read = []
                epoch_total_write = []
                epoch_throughput_read = []
                epoch_throughput_write = []
                epoch_total_read_rounds = []
                epoch_read_rounds = []
                epoch_total.clear()
                epoch_start = time.time()
                epoch += 1

    total_read_time = 0
    total_write_time = 0
    if total_read:
        # For now, remove the 10% worst values
        #total_read.sort()
        #total_read = total_read[:int(len(total_read)*0.9)]
        for read_time in total_read:
            total_read_time += read_time
        print_latency_stats(total_read, 'READ TRANSACTIONS', result_log, total_read_time)
        read_rounds = 0
        round_percentage = {}
        for read_round in total_read_rounds:
            read_rounds += read_round
            if read_round not in round_percentage:
                round_percentage[read_round] = 0
            round_percentage[read_round] += 1
        read_rounds = read_rounds / len(total_read_rounds)
        result_log.info("\tREAD ROUNDS: %s" % read_rounds)
        result_log.info("ROUND PERCENTAGE:")
        for rounds in round_percentage:
            result_log.info("\t%s : %.2f" % (rounds, round_percentage[rounds] / len(total_read_rounds)))
        result_log.info("Average Time on Anna (ms): %.4f " %
                     ((total_statistics[0] / len(total_read_rounds)) / 1000))
        result_log.info("Average Time on Anna Network (ms): %.4f " %
                     ((total_statistics[1] / len(total_read_rounds)) / 1000))
        result_log.info("Average Time on Cache (ms): %.4f " %
                     ((total_statistics[2] / len(total_read_rounds)) / 1000))
    if total_write:
        #total_write.sort()
        #total_write = total_write[:int(len(total_write) * 0.9)]
        for write_time in total_write:
            total_write_time += write_time
        print_latency_stats(total_write, 'WRITE TRANSACTIONS', result_log, total_write_time)
    total = total_read + total_write
    total_time = total_read_time + total_write_time

    print_latency_stats(total, 'SYSTEM PERFORMANCE', result_log, total_time)
    return total

def cleanUpKubernetes():
    os.system("kubectl delete --all rs --namespace=hydro")
    os.system("kubectl delete --all pods --namespace=hydro")

def cleanUpNamespace():
    os.system("kubectl config set-context --current --namespace=default")

    os.system("kubectl delete namespaces hydro")

def experiment(test, experiments,redisService, redisPort, hydro, faasTCC, system_list, local):
    logger = logging.getLogger("result")
    logger.setLevel(logging.INFO)
    print(redisService)
    print(redisPort)

    conf = load_conf("setup.yml")
    workload = conf["workload"]

    systems = system_list
    flag = True
    while flag:
        try:
            r = redis.Redis(host=redisService, port=redisPort, db=0)
            p = r.pubsub()
            p.psubscribe('result')
            print('Connected to Reddis')
            flag = False
        except:
            continue
    # Gets or creates a logger
    logger = logging.getLogger("result")

    # set log level
    logger.setLevel(logging.INFO)

    # define file handler and set formatter
    file_header = ""
    for system in systems:
        file_header += "%s_" % str(system)
    file_name = file_header + '%s_%s_%s_%s.log' % (test, str(experiments[0]), str(experiments[-1]), datetime.now().strftime("%m_%d_%H_%M"))
    file_handler = logging.FileHandler(file_name)
    formatter = logging.Formatter('%(asctime)s %(message)s')
    file_handler.setFormatter(formatter)

    # add file handler to logger
    logger.addHandler(file_handler)
    printSetup(conf, logger)
    printSetup(conf, logging)
    #cleanUpKubernetes()
    #cleanUpNamespace()
    setupNamespace(faasTCC)
    setup_kub_flag = True
    populate_flag = True
    for system in systems:
        workload["typ"] = system
        for experiment in experiments:
            new_deployment = test in conf["deployment"]
            if new_deployment:
                conf["deployment"][test] = experiment
            else:
                workload[test] = experiment

            # We only setup on the first time or if the tests are from deployment
            if setup_kub_flag or new_deployment:
                logging.info('Starting deployment')
                setupKubernetes(conf["deployment"], local)
                setup_kub_flag = False
                time.sleep(600)
                logging.info('Finished deployment')

            # We only populate on the first time or if the tests change the submited data
            if populate_flag or new_deployment or test == "value_size" or test == "db_size":
                logging.info('Starting populate')
                populate(r, p, conf["setup"]["bname"], workload, conf["deployment"]["benchmark"])
                logging.info('Finished Populate')
                populate_flag = False
                time.sleep(60)

            total = []
            logging.info('*** Experiment system %s %s = %s ***' % (system, test, str(experiment)))
            logger.info('*** Experiment system %s %s = %s ***' % (system, test, str(experiment)))

            logging.info('*** Experiment ***')
            print("Starting experiment")

            total = run_expermiment(r,p, conf["setup"]["bname"], workload, conf["deployment"]["benchmark"], logging = logging, result_log = logger, init_operation=new_deployment)
            logging.info('*** Experiment Result***')
            logging.info('*** Experiment End***')
            time.sleep(120)
            if new_deployment:
                cleanUpKubernetes()
                time.sleep(180)

        cleanUpKubernetes()
        setup_kub_flag = True
        populate_flag = True
        time.sleep(60)

    cleanUpNamespace()
    logging.info('*** END ***')

def str2bool(v):
    if isinstance(v, bool):
        return v
    if v.lower() in ('yes', 'true', 't', 'y', '1', 'True'):
        return True
    elif v.lower() in ('no', 'false', 'f', 'n', '0', 'False'):
        return False
    else:
        raise argparse.ArgumentTypeError('Boolean value expected.')

def str2experiment(v):
    try:
        return int(v)
    except ValueError:
        return float(v)

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='''Runs experiments''')

    parser.add_argument('-t', '--test', nargs=1, type=str, metavar='T',
                        help='Experiemnt field' +
                             '(required)', dest='test', required=True)

    parser.add_argument('-e', '--experiments', nargs="+", type=str2experiment, metavar='E',
                        help='Experiemnt ' +
                             '(required)', dest='experiments', required=True)

    parser.add_argument('-r', '--redish', nargs=1, type=str, metavar='R',
                        help='redis host ' +
                             '(required)', dest='redish', required=True)

    parser.add_argument('-p', '--redisp', nargs=1, type=int, metavar='P',
                        help='redis port ' +
                             '(required)', dest='redisp', required=True)

    parser.add_argument('-hh', '--hydro', nargs=1, type=str, metavar='H',
                        help='hydro home ' +
                             '(required)', dest='hydro', required=True)

    parser.add_argument('-f', '--faastccs', nargs=1, type=str, metavar='F',
                        help='faastccs home ' +
                             '(required)', dest='faastccs', required=True)

    parser.add_argument('-s', '--system', nargs="+", type=str, metavar='S', 
                        help='System to test' + 
                             '(required)', dest='systems', required=True)

    parser.add_argument('-l', '--local', nargs=1, type=str2bool, metavar='L',
                        help='Local flag', dest='local', required=True)


    args = parser.parse_args()
    print(args.test[0])
    print(args.experiments)
    print(args.redish[0])
    print(args.redisp[0])
    print(args.hydro[0])
    print(args.faastccs[0])
    print(args.local[0])
    #time.sleep(60)
    experiment(args.test[0], args.experiments, args.redish[0], args.redisp[0],
                  args.hydro[0], args.faastccs[0], args.systems, args.local[0])
