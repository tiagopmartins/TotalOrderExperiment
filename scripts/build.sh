#!/bin/bash

# Getting the IP of the network interface (eth0, wlan0, wlan1, ...).
PRIVATE_IP=`ifconfig eth0 | grep 'inet' | grep -v inet6 | sed -e 's/^[ \t]*//' | cut -d' ' -f2`
PUBLIC_IP=$PRIVATE_IP

# Compile the latest version of the code on the branch we just check out.
mkdir build && cd build && cmake .. && make -j2 && cd ..

# Do not start the server until conf/anna-config.yml has been copied onto this
# pod -- if we start earlier, we won't know how to configure the system.
while [[ ! -f "conf/anna-config.yml" ]]; do
  continue
done

# Tailor the config file to have process specific information.
echo -e "server:" >> conf/anna-config.yml
echo -e "    public_ip: $PUBLIC_IP" >> conf/anna-config.yml
./build/server/server localhost 50001