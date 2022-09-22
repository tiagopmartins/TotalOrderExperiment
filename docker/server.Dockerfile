FROM tiagopm/to-experiment:base

ARG source_branch=master
ARG build_branch=build

USER root

# Re-cloning the latest version of the repository
RUN rm -r $HYDRO_HOME
WORKDIR $HYDRO_HOME
RUN git clone https://github.com/tiagopmartins/TotalOrderExperiment.git to-experiment
WORKDIR $HYDRO_HOME/to-experiment

RUN git pull
RUN git remote remove origin && git remote add origin https://github.com/tiagopmartins/TotalOrderExperiment.git
RUN git fetch origin && git checkout -b $build_branch origin/$source_branch

WORKDIR $HYDRO_HOME/to-experiment
# Update the time using NTP and run the server
# Add artificial delays: tc qdisc add dev eth0 root netem delay 100ms 10ms distribution normal
CMD ntpd -g -d -q -n -p pool.ntp.org && \
    bash scripts/build_server.sh -j2 -bRelease
