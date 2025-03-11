FROM ubuntu:18.04 AS ubuntu-18.04-base
# 🧰 Add basic tools.
RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y software-properties-common build-essential git python3-pip
RUN add-apt-repository ppa:ubuntu-toolchain-r/test
RUN apt-get remove -y cmake


FROM ubuntu:20.04 AS ubuntu-20.04-base
# 🧰 Add basic tools.
RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y software-properties-common build-essential git python3-pip
RUN add-apt-repository ppa:ubuntu-toolchain-r/test
RUN apt-get remove -y cmake


FROM ubuntu-18.04-base AS ubuntu-18.04-gcc-13
# 🧰 Install compiler.
RUN apt-get install -y gcc-13 g++-13
RUN python3 -m pip install cmake==3.31.6


FROM ubuntu-20.04-base AS ubuntu-20.04-gcc-13
# 🧰 Install compiler.
RUN apt-get install -y gcc-13 g++-13
RUN python3 -m pip install cmake==3.31.6
