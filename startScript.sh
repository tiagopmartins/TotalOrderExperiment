#!/bin/bash

if [ -z "$1" ]; then
  echo "No argument provided. Exiting."
  exit 1
fi

# A helper function that takes a space separated list and generates a string
# that parses as a YAML list.
gen_yml_list() {
  IFS=' ' read -r -a ARR <<< $1
  RESULT=""

  for IP in "${ARR[@]}"; do
    RESULT=$"$RESULT        - $IP\n"
  done

  echo -e "$RESULT"
}

cd $HYDRO_HOME/anna
mkdir -p conf

# Getting the IP of the network interface (eth0, wlan0, wlan1, ...).
PRIVATE_IP=`ifconfig wlx7c8bc10c3f1 | grep 'inet' | grep -v inet6 | sed -e 's/^[ \t]*//' | cut -d' ' -f2`
PUBLIC_IP=$PRIVATE_IP


# Download latest version of the code from relevant repository & branch -- if
# none are specified, we use tiagopmartins/TotalOrderExperiment by default.
git remote remove origin
if [[ -z "$REPO_ORG" ]]; then
  REPO_ORG="tiagopmartins"
fi

if [[ -z "$REPO_BRANCH" ]]; then
  REPO_BRANCH="master"
fi

git remote add origin https://github.com/$REPO_ORG/TotalOrderExperiment.git
while ! (git fetch -p origin)
do
  echo "git fetch failed, retrying"
done
git checkout -b build origin/$REPO_BRANCH

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