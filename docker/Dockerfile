FROM ubuntu:18.04

ARG TOOLCHAIN_URL="https://developer.arm.com/-/media/Files/downloads/gnu-rm/5_4-2016q3/gcc-arm-none-eabi-5_4-2016q3-20160926-linux.tar.bz2"

WORKDIR /toolchain

RUN export DEBIAN_FRONTEND=noninteractive \
	&& dpkg --add-architecture i386 \
	&& apt-get -y update \
	&& apt-get -y install libc6:i386 libstdc++6:i386 \
	&& apt-get -y install coreutils wget make lbzip2 python python-numpy figlet cowsay \
	&& apt-get purge -y --auto-remove

RUN wget "$TOOLCHAIN_URL"
RUN export TOOLCHAIN_TARBALL="$(basename "$TOOLCHAIN_URL")" \
	&& tar -xvf "$TOOLCHAIN_TARBALL" \
	&& rm "$TOOLCHAIN_TARBALL"

RUN export TOOLCHAIN_PATH="$(find . -maxdepth 1 -name "gcc-arm-none-eabi*")" \
	&& rm -rf "$TOOLCHAIN_PATH/share/doc"

RUN wget "https://raw.githubusercontent.com/tnalpgge/rank-amateur-cowsay/master/cows/small.cow" -O /usr/share/cowsay/cows/small.cow
ENV PATH=/usr/games:$PATH

WORKDIR /root
COPY run.sh .

WORKDIR /build
ENTRYPOINT ["/root/run.sh"]
