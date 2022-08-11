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
CMD bash scripts/build_client.sh -j2 -bRelease
WORKDIR /hydro