FROM ubuntu:18.04 AS ubuntu-18.04-base
# 🧰 Add basic tools.
RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y software-properties-common build-essential git python3-pip
RUN add-apt-repository ppa:ubuntu-toolchain-r/test
RUN apt-get remove -y cmake
RUN python3 -m pip install --upgrade pip


FROM ubuntu:20.04 AS ubuntu-20.04-base
# 🧰 Add basic tools.
RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y software-properties-common build-essential git python3-pip
RUN add-apt-repository ppa:ubuntu-toolchain-r/test
RUN apt-get remove -y cmake
RUN python3 -m pip install --upgrade pip


FROM ubuntu-18.04-base AS ubuntu-18.04-gcc-13
# 🧰 Install compiler.
RUN apt-get install -y gcc-13 g++-13
# Cmake 3.31.6 is not available on Ubuntu 18.04, hence, we try with 3.28.4.
RUN python3 -m pip install cmake==3.28.4


FROM ubuntu-20.04-base AS ubuntu-20.04-gcc-13
# 🧰 Install compiler.
RUN apt-get install -y gcc-13 g++-13
RUN python3 -m pip install cmake==3.31.6
