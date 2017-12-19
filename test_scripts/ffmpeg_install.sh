#!/bin/bash

BDIR=${HOME}/
FFDIR=${BDIR}/ffmpeg-src
FFSRC=${FFDIR}/ffmpeg
FFINSTDIR=${FFDIR}/install
FFBIN=${FFINSTDIR}/bin
NASMSRC=${FFDIR}/nasm-2.13.01
X264SRC=${FFDIR}/x264
AACSRC=${FFDIR}/fdk-aac

origdir=$(pwd)

mkdir -p ${FFDIR}
cd ${FFDIR}

# GET NASM
if [ ! -d "${NASMSRC}" ]; then
  wget http://www.nasm.us/pub/nasm/releasebuilds/2.13.01/nasm-2.13.01.tar.bz2
  tar xjvf nasm-2.13.01.tar.bz2
fi

echo "Entering ${NASMSRC}"
cd ${NASMSRC}

./autogen.sh
PATH="${FFBIN}:$PATH" ./configure --prefix="${FFINSTDIR}" --bindir="${FFBIN}"
make -j 2
make install

echo "Exiting ${NASMSRC}"
cd ${FFDIR}

#GET libx264
if [ ! -d "${X264SRC}" ]; then
  git clone http://git.videolan.org/git/x264 ${X264SRC}
  git checkout aaa9aa83a111ed6f1db253d5afa91c5fc844583f
fi

echo "Entering ${X264SRC}"
cd ${X264SRC}

PATH="${FFBIN}:$PATH" PKG_CONFIG_PATH="${FFINSTDIR}/lib/pkgconfig" ./configure \
  --prefix="${FFINSTDIR}" \
  --bindir="${FFBIN}" \
  --enable-static \
  --disable-opencl
PATH="${FFBIN}:$PATH" make -j 2
make install

echo "Exiting ${X264SRC}"
cd ${FFDIR}

if [ ! -d "${AACSRC}" ]; then
  git clone --branch v0.1.5 https://github.com/mstorsjo/fdk-aac ${AACSRC}
fi

echo "Entering ${AACSRC}"
cd ${AACSRC}

autoreconf -fiv
./configure --prefix="${FFINSTDIR}" --disable-shared
make -j 2
make install

echo "Exiting ${AACSRC}"
cd ${FFDIR}

if [ ! -d "${FFSRC}" ]; then
  git clone --branch n3.4 https://git.ffmpeg.org/ffmpeg.git "${FFSRC}"
fi

echo "Entering ${FFSRC}"
cd ${FFSRC}

PATH="${FFBIN}:$PATH" PKG_CONFIG_PATH="${FFINSTDIR}/lib/pkgconfig" ./configure \
  --prefix="${FFINSTDIR}" \
  --pkg-config-flags="--static" \
  --extra-cflags="-I${FFINSTDIR}/include" \
  --extra-ldflags="-L${FFINSTDIR}/lib" \
  --extra-libs="-lpthread -lm" \
  --bindir="${FFBIN}" \
  --enable-gpl \
  --disable-cuda \
  --disable-cuvid \
  --enable-libx264 \
  --enable-libfdk-aac \
  --enable-nonfree

PATH="${FFBIN}:$PATH" make -j 4
make install

cd ${origdir}
