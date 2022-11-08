#ifndef CONSTANTS_H
#define CONSTANTS_H

// Path to the file containing the IPs of the servers
const std::string SERVER_LIST_PATH = "hydro/cluster/server_ips.yml";

// Port the servers receive connections on
const std::string SERVER_PORT = "50001";

// Redis information
//const std::string REDIS_ADDRESS = "redis-master.default.svc.cluster.local";
const std::string REDIS_ADDRESS = "localhost";
const int REDIS_INTERNAL_PORT = 6379;

#endif