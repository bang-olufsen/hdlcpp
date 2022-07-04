FROM ubuntu:focal

ENV USER=user

RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get -y upgrade \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
    software-properties-common \
    apt-transport-https \
    locales \
    apt-utils \
    build-essential \
    cmake \
    ccache \
    git \
    ssh \
    file \
    sudo \
    libboost-test-dev \
    gcc \
    g++ \
    gcc-10 \
    g++-10 \
    gcc-multilib \
    g++-multilib \
    gdb \
    lcov \
    python3.8 \
    python3.8-dev \
    python3.8-distutils \
    && update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-10 80 --slave /usr/bin/g++ g++ /usr/bin/g++-10 --slave /usr/bin/gcov gcov /usr/bin/gcov-10 \
    && apt-get clean \
    && apt-get autoremove -y \
    && rm -rf /var/lib/apt/lists/*

RUN useradd -s /bin/bash $USER
RUN usermod -a -G sudo $USER
RUN echo "$USER ALL=(root) NOPASSWD:ALL" > /etc/sudoers.d/$USER
RUN chmod 0440 /etc/sudoers.d/$USER
RUN chown -R $USER:$USER /root
RUN chmod 755 /root

RUN locale-gen en_US.UTF-8
RUN rm -f .bash_history
