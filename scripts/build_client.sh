#!/bin/bash

# Compile the latest version of the code on the branch we just check out.
mkdir build && cd build && cmake .. && make -j2 && cd ..

# Do not start the server until hydro/cluster/server_ips.yml has been
# copied onto this pod -- if we start earlier, we won't know how to configure
# the system.
while [[ ! -f "hydro/cluster/server_ips.yml" ]]; do
  continue
done

# Running the client.
./build/client/client