FROM ubuntu:18.04 AS ubuntu-18.04-base
# 🧰 Add PPA for GCC.
RUN apt-get update && apt-get install -y software-properties-common
RUN add-apt-repository ppa:ubuntu-toolchain-r/test


FROM ubuntu:20.04 AS ubuntu-20.04-base
# 🧰 Add PPA for GCC.
RUN apt-get update && apt-get install -y software-properties-common
RUN add-apt-repository ppa:ubuntu-toolchain-r/test


FROM ubuntu-20.04-base AS ubuntu-20.04-gcc-13
# 🧰 Install compiler.
RUN apt-get update && apt-get install -y gcc-13 g++-13 cmake


FROM ubuntu-18.04-base AS ubuntu-18.04-gcc-13
# 🧰 Install compiler.
RUN apt-get update && apt-get install -y gcc-13 g++-13 cmake
