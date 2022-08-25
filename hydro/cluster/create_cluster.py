#!/usr/bin/env python3

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
#  Modifications copyright (C) 2021 Taras Lykhenko, Rafael Soares

import argparse
import os

import yaml

from hydro.cluster.add_nodes import batch_add_nodes, get_current_pod_container_pairs
from hydro.shared import util

BATCH_SIZE = 100


def create_cluster(client_count, server_count, local, cfile):

    if 'HYDRO_HOME' not in os.environ:
        raise ValueError('HYDRO_HOME environment variable must be set to be '
                         + 'the directory where all Hydro project repos are '
                         + 'located.')
    print(os.environ['HYDRO_HOME'])
    prefix = os.path.join(os.environ['HYDRO_HOME'], 'cluster/hydro/cluster')
    print(prefix)
    client, apps_client = util.init_k8s()

    kubecfg = os.path.join(os.environ['HOME'], '.kube/config')
    os.system('cp %s %s' % (kubecfg,  os.environ['HOME']))

    kube_config_copy = util.load_yaml('config', os.environ['HOME'])
    kubecfg = os.path.join(os.environ['HOME'], 'config')

    if local:
        cert_aut = kube_config_copy['clusters'][0]['cluster']['certificate-authority']
        cert = kube_config_copy['users'][0]['user']['client-certificate']
        key = kube_config_copy['users'][0]['user']['client-key']

        kube_config_copy['clusters'][0]['cluster']['certificate-authority'] = os.path.join('/root/.kube/',os.path.basename(cert_aut))
        kube_config_copy['users'][0]['user']['client-certificate'] = os.path.join('/root/.kube/',os.path.basename(cert))
        kube_config_copy['users'][0]['user']['client-key'] = os.path.join('/root/.kube/',os.path.basename(key))

        with open(kubecfg, 'w') as file:
            yaml.dump(kube_config_copy, file)

    print('Creating %d server nodes...' % (server_count))
    batch_add_nodes(client, apps_client, cfile, ['server'], [server_count], BATCH_SIZE, prefix)
    x = util.get_pod_ips(client, 'role=server')
    with open('server_ips.yml', 'w') as file:
        yaml.dump({'ips': x}, file)
    pods = client.list_namespaced_pod(namespace=util.NAMESPACE, label_selector='role=server').items

    #for pname, cname in get_current_pod_container_pairs(pods):
    #    util.copy_file_to_pod(client, 'server_ips.yml', pname, 'hydro/cluster', cname)

    for pod in pods:
        util.copy_file_to_pod(client, 'server_ips.yml', pod.metadata.name, 'hydro/cluster', 'default')

    print('Creating %d client nodes...' % (client_count))
    batch_add_nodes(client, apps_client, cfile, ['client'], [client_count], BATCH_SIZE, prefix)
    util.get_pod_ips(client, 'role=client')
    pods = client.list_namespaced_pod(namespace=util.NAMESPACE, label_selector='role=client').items

    for pname, cname in get_current_pod_container_pairs(pods):
        util.copy_file_to_pod(client, 'server_ips.yml', pname, 'hydro/cluster', cname)
    os.system('rm server_ips.yml')

    print('Setup complete')

def str2bool(v):
    if isinstance(v, bool):
       return v
    if v.lower() in ('yes', 'true', 't', 'y', '1'):
        return True
    elif v.lower() in ('no', 'false', 'f', 'n', '0'):
        return False
    else:
        raise argparse.ArgumentTypeError('Boolean value expected.')

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='''Creates a Hydro cluster
                                     using Kubernetes and kops. If no SSH key
                                     is specified, we use the default SSH key
                                     (~/.ssh/id_rsa), and we expect that the
                                     correponding public key has the same path
                                     and ends in .pub.

                                     If no configuration file base is
                                     specified, we use the default
                                     ($HYDRO_HOME/anna/conf/anna-base.yml).''')

    parser.add_argument('-s', '--server', nargs=1, type=int, metavar='S',
                        help='The number of server nodes to start with ' +
                        '(required)', dest='server', required=True)
    parser.add_argument('-c', '--client', nargs=1, type=int, metavar='C',
                        help='The number of client nodes to start with ' +
                             '(required)', dest='client', required=True)
    parser.add_argument('-l', '--local', nargs='?', type=str2bool, metavar='L',
                        help='Minikube cluster ' +
                             '(optional)', dest='local', default=False)



    args = parser.parse_args()

    print(args.local)
    os.system("kubectl config set-context --current --namespace=default")
    os.system("kubectl delete namespaces hydro")
    os.system("kubectl create namespace hydro")
    os.system("kubectl create secret docker-registry regcred --namespace=hydro --docker-username=tiagopm --docker-password=bET!pr8bRlPHa7=iPraC")
    os.system("kubectl config set-context --current --namespace=hydro")

    create_cluster(args.client[0], args.server[0], args.local, [])
