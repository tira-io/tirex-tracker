FROM ubuntu:16.04 AS ubuntu-16.04-gcc-5
# 🧰 Add basic tools.
RUN apt-get update
RUN DEBIAN_FRONTEND=noninteractive apt-get install -y software-properties-common python3-pip git
# Note: We need to constrain the Pip version here because later Pip versions use f-strings internally which are only available in Python 3.6 and later but Ubuntu 16.04 only runs Python 3.5.
RUN python3 -m pip install --upgrade pip~=20.3
# 🧰 Install GCC compiler.
RUN apt-get install -y gcc-5 g++-5
ENV CC=gcc-5
ENV CXX=g++-5
# 🧰 Install CMake.
RUN apt-get remove -y cmake
RUN python3 -m pip install cmake~=3.24.0
# 🧰 Install Doxygen.
RUN DEBIAN_FRONTEND=noninteractive apt-get -y install doxygen


# Note: GCC 8 is the latest available on Ubuntu 20.04
FROM ubuntu:18.04 AS ubuntu-18.04-gcc-8
# 🧰 Add basic tools.
RUN apt-get update
RUN DEBIAN_FRONTEND=noninteractive apt-get install -y software-properties-common python3-pip git
RUN python3 -m pip install --upgrade pip
# 🧰 Install GCC compiler.
RUN apt-get install -y gcc-8 g++-8
ENV CC=gcc-8
ENV CXX=g++-8
# 🧰 Install CMake.
RUN apt-get remove -y cmake
RUN python3 -m pip install cmake~=3.24.0
# 🧰 Install Doxygen.
RUN DEBIAN_FRONTEND=noninteractive apt-get -y install doxygen


# Note: GCC 9 is the latest available on Ubuntu 20.04
FROM ubuntu:20.04 AS ubuntu-20.04-gcc-9
# 🧰 Add basic tools.
RUN apt-get update
RUN DEBIAN_FRONTEND=noninteractive apt-get install -y software-properties-common python3-pip git
RUN python3 -m pip install --upgrade pip
# 🧰 Install GCC compiler.
RUN apt-get install -y gcc-9 g++-9
ENV CC=gcc-9
ENV CXX=g++-9
# 🧰 Install CMake.
RUN apt-get remove -y cmake
RUN python3 -m pip install cmake~=3.24.0
# 🧰 Install Doxygen.
RUN DEBIAN_FRONTEND=noninteractive apt-get -y install doxygen
