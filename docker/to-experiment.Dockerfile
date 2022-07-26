FROM to-experiment-base:latest

ARG source_branch=master
ARG build_branch=build

USER root

# Check out to the appropriate branch on the appropriate fork of the repository
# and build TotalOrderExperiment.
RUN rm -f ~/.gitconfig
RUN git config --global url."https://X:@github.com/".insteadOf "https://github.com/"
RUN git config --global url."https://X:@github.com/".insteadOf "git@github.com:"

# Re-cloning the latest version of the repository
RUN rm -r $HYDRO_HOME
WORKDIR $HYDRO_HOME
RUN git clone --recurse-submodules https://github.com/tiagopmartins/TotalOrderExperiment.git to-experiment
WORKDIR $HYDRO_HOME/to-experiment

RUN git pull --recurse-submodules
RUN git remote remove origin && git remote add origin https://github.com/tiagopmartins/TotalOrderExperiment.git

RUN git fetch origin && git checkout -b $build_branch origin/$source_branch
RUN bash scripts/build.sh -j2 -bRelease
WORKDIR /hydro