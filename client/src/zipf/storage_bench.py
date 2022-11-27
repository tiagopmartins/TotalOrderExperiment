#  Copyright 2019 U.C. Berkeley RISE Lab
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.

import logging
import secrets
import sys
from datetime import datetime
import os
import cloudpickle as cp
from cloudburst.shared.reference import CloudburstReference

from cloudburst.server.benchmarks.ZipfGenerator import LoadGenerator
from cloudburst.shared.proto.cloudburst_pb2 import CloudburstError
from cloudburst.server.benchmarks import utils

from cloudburst.shared.proto.cloudburst_pb2 import (
    Continuation,
    DagTrigger,
    FunctionCall,
    NORMAL, MULTI,  # Cloudburst's consistency modes,
    EXECUTION_ERROR, FUNC_NOT_FOUND,  # Cloudburst's error types
    MULTIEXEC # Cloudburst's execution types
)

from cloudburst.shared.proto.shared_pb2 import (
    AbstractTimestamp
)

from anna.shared_pb2 import (
    # Protobuf enum lattices types
    EIGER_PORT
)
import pymongo
import random


READ = 0
WRITE = 1
import numpy as np

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


def getTime():
    return datetime.now().timestamp() * 1000


def initOperation(dc_to_key, transaction_client, db_size, tup, dependecy_time):
    keys = set()
    for i in range(db_size):
        keys.add("k" + str(i))
        if len(keys) == 1000:
            result = None
            while(result == None):
                result = transaction_client.causal_get_storage(keys, 0, 0,
                           consistency=MULTI, client_id=0,tup = tup)

            for partition in result.dependency.vector_clock:
                dependecy_time.vector_clock[partition] = result.dependency.vector_clock[partition]

            for cr in result.cr_tuples:
                if cr.dc_id not in dc_to_key:
                    dc_to_key[cr.dc_id] = list()
                dc_to_key[cr.dc_id].append(cr.key)
            keys.clear()

    # DB size is not divisible by 100
    if len(keys) > 0:
        result = None
        while (result == None):
            result = transaction_client.causal_get_storage(keys, 0, 0,
                                                           consistency=MULTI, client_id=0, tup=tup)

        for cr in result.cr_tuples:
            if cr.dc_id not in dc_to_key:
                dc_to_key[cr.dc_id] = list()
            dc_to_key[cr.dc_id].append(cr.key)
        keys.clear()

def getKey(zipfGenerator_to_dc, dc_to_keys, local_dc, remote_ratio):
    local_or_remote = random.uniform(0, 1)
    if(local_or_remote > remote_ratio):
        return dc_to_keys[local_dc][zipfGenerator_to_dc[local_dc].next()]
    else:
        dcs = list(dc_to_keys.keys())
        dcs_copy = dcs[:]
        if local_dc in dcs_copy:
            dcs_copy.remove(local_dc)
        dc = random.choice(list(dcs_copy))
        return dc_to_keys[dc][zipfGenerator_to_dc[dc].next()]


 

def executeOperation(transaction_client, zipfGenerator_to_dc, dc_to_keys, local_dc, remote_ratio, tx_size_read, tx_size_write, write_ratio, value_size, dependecy_time, tup):
    read_or_write = random.uniform(0, 1)
    if(read_or_write > write_ratio):
        keys = set()
        while len(keys) < tx_size_read:
            keys.add(getKey(zipfGenerator_to_dc, dc_to_keys, local_dc, remote_ratio))
        result = None
        while(result == None):
            result = transaction_client.causal_get_storage(keys, dependecy_time, dependecy_time,
                       consistency=MULTI, client_id=0,tup = tup)
            if result == None:
                logging.info("[RETRY] Retrying reads")
        return result.dependency, result.rounds, result.time_spent, READ
    else:
        key_value = dict()
        value = serializer.dump_lattice(generateValue(value_size), tup)
        while len(key_value.keys()) < tx_size_write:
            key_value[getKey(zipfGenerator_to_dc, dc_to_keys, local_dc, remote_ratio)] = value
        result = None
        while (result == None):
            result = transaction_client.causal_put_multi(key_value,client_id=0,tup = tup)
        return result.dependency, 0, result.time_spent, WRITE


from cloudburst.shared.serializer import Serializer
serializer = Serializer()

def generateValue(value_size):
    return secrets.token_bytes(value_size)

def run(transaction_client, typ, num_requests, create, redis, dc_to_keys, dag_name, db_size, tx_size_read,
        tx_size_write, write_ratio, value_size, zipf, local_dc, remote_ratio, num_benchmark_servers, init_required):

    if create:
        logging.info("Populating")
        file_name = False

        for fname in os.listdir('/hydro'):
            if fname.startswith('populating_bench'):
                file_name = fname
                break

        if file_name:
            section = int(file_name.split("_")[2])
            logging.info("In charge of populating: " + str(section))

            value = serializer.dump_lattice(generateValue(value_size), typ)
            # Split the writes between 3 entities
            num_populaters = int(num_benchmark_servers * 3)
            key_value = dict()
            for i in range((db_size//num_populaters)*section, (db_size//num_populaters)*(section+1)+1):
                key_value["k" + str(i)] = value
                if len(key_value) == 100:
                    result = None
                    while (result == None):
                        result = transaction_client.causal_put_multi(key_value, client_id=0, tup=typ)
                    key_value.clear()

            if len(key_value) > 0:
                result = None
                while (result == None):
                    result = transaction_client.causal_put_multi(key_value, client_id=0, tup=typ)
                key_value.clear()

        return [], [], [], 0

    else:
        logging.info("Starting experiment")
        vc_dependency_time = AbstractTimestamp()
        if not dc_to_keys or init_required:
            logging.info("Initiating DC to keys")
            init_operation_start = getTime()
            initOperation(dc_to_keys, transaction_client, db_size, typ, vc_dependency_time)
            logging.info("Init Operation duration: %.4f seconds" % ((getTime()-init_operation_start)/1000))

        zipfGenerator_to_dc = dict()
        for dc in dc_to_keys:
            zipfGenerator_to_dc[dc] = LoadGenerator(len(dc_to_keys[dc]), zipf, True)


        total_time = []
        epoch_latencies_read = []
        epoch_latencies_write = []
        read_rounds = []
        epoch_start = getTime()
        epoch = 0
        executed_requests = 0
        start_log = num_requests // 4
        end_log = num_requests - start_log
        total_read = []
        total_write = []
        total_read_rounds = []
        total_statistics = [0, 0, 0]
        epoch_statistics = [0, 0, 0]
        dependency_time = AbstractTimestamp()
        while executed_requests < num_requests:
            start = getTime()
            if typ == EIGER_PORT:
                dependency_time.timestamp = min(vc_dependency_time.vector_clock.values())

            dependency_time, number_rounds, time_spent, operation_type = executeOperation(transaction_client, zipfGenerator_to_dc, dc_to_keys, local_dc, remote_ratio, tx_size_read, tx_size_write, write_ratio, value_size, dependency_time, typ)

            if typ == EIGER_PORT:
                for partition in dependency_time.vector_clock:
                    vc_dependency_time.vector_clock[partition] = dependency_time.vector_clock[partition]
                dependency_time = AbstractTimestamp()
            end = getTime()

            if operation_type == READ:
                epoch_latencies_read += [end - start]
                total_read += [end - start]
                read_rounds += [number_rounds]
                total_read_rounds += [number_rounds]

                epoch_statistics[0] += time_spent.time_anna
                epoch_statistics[1] += time_spent.time_network_anna
                epoch_statistics[2] += time_spent.time_cache

                total_statistics[0] += time_spent.time_anna
                total_statistics[1] += time_spent.time_network_anna
                total_statistics[2] += time_spent.time_cache

            else:
                epoch_latencies_write += [end - start]
                total_write += [end - start]

            epoch_end = getTime()
            if epoch_end - epoch_start > 10000:
                executed_requests += 10
                if redis and (executed_requests >= start_log and executed_requests <= end_log):
                    redis.publish("result",cp.dumps((epoch_latencies_read, epoch_latencies_write, read_rounds,
                                                     epoch_statistics)))


                epoch += 1
                if epoch_latencies_read:
                    print_latency_stats(epoch_latencies_read, 'Read Transactions', False, sum(epoch_latencies_read) / 1000)
                    print("Number of rounds: %.4f" % (sum(read_rounds)/len(read_rounds)))
                    logging.info("Average Time on Anna (ms): %.4f " %
                                 ((epoch_statistics[0] / len(read_rounds)) / 1000))
                    logging.info("Average Time on Anna Network (ms): %.4f " %
                                 ((epoch_statistics[1] / len(read_rounds)) / 1000))
                    logging.info("Average Time on Cache (ms): %.4f " %
                                 ((epoch_statistics[2] / len(read_rounds)) / 1000))
                if epoch_latencies_write:
                    print_latency_stats(epoch_latencies_write, 'Write Transactions', False, sum(epoch_latencies_write) / 1000)

                epoch_latencies_read.clear()
                epoch_latencies_write.clear()
                read_rounds.clear()
                epoch_statistics = [0, 0, 0]
                epoch_start = getTime()


        if total_read:
            print_latency_stats(total_read, 'Read Transactions', False, sum(total_read)/1000)
            print("Number of rounds: %.4f" % (sum(total_read_rounds) / len(total_read_rounds)))
            print("Average Time on Anna (ms): %.4f " %
                         ((total_statistics[0] / len(total_read)) / 1000))
            print("Average Time on Anna Network (ms): %.4f " %
                         ((total_statistics[1] / len(total_read)) / 1000))
            print("Average Time on Cache (ms): %.4f " %
                         ((total_statistics[2] / len(total_read)) / 1000))
            logging.info("Average Time on Anna (ms): %.4f " %
                         ((total_statistics[0] / len(total_read)) / 1000))
            logging.info("Average Time on Anna Network (ms): %.4f " %
                         ((total_statistics[1] / len(total_read)) / 1000))
            logging.info("Average Time on Cache (ms): %.4f " %
                         ((total_statistics[2] / len(total_read)) / 1000))

        if total_write:
            print_latency_stats(total_write, 'Write Transactions', False, sum(total_write)/1000)

        return total_time, [], [], 0