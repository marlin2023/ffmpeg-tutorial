#!/bin/bash

FFMPEG_ROOT=`pwd`

# Detect OS
OS=`uname`
HOST_ARCH=`uname -m`
export CCACHE=; type ccache >/dev/null 2>&1 && export CCACHE=ccache
if [ $OS == 'Linux' ]; then
  export HOST_SYSTEM=linux-$HOST_ARCH
elif [ $OS == 'Darwin' ]; then
  export HOST_SYSTEM=darwin-$HOST_ARCH
fi

platform="$1"
version_type="$2"

SOURCE=`pwd`
DEST=$SOURCE/build/android

TOOLCHAIN=$SOURCE/build
SYSROOT=$TOOLCHAIN/sysroot/

function arm_toolchain()
{
  export CROSS_PREFIX=arm-linux-androideabi-
  $ANDROID_NDK/build/tools/make_standalone_toolchain.py --arch arm --api 21 \
    --install-dir=$TOOLCHAIN
}

function armv8_toolchain()
{
  export CROSS_PREFIX=aarch64-linux-android-
  $ANDROID_NDK/build/tools/make_standalone_toolchain.py --arch=arm64 --api 21 \
    --install-dir=$TOOLCHAIN
}

if [ "$platform" = "armv8" ];then
  echo "Build Android armv8 ffmpeg\n"
  armv8_toolchain
  TARGET="armv8"
else
  echo "Build Android arm ffmpeg\n"
  arm_toolchain
  TARGET="armv7"
#  TARGET="neon armv7 vfp armv6"
fi
export PATH=$TOOLCHAIN/bin:$PATH
export CC="$CCACHE ${CROSS_PREFIX}gcc"
export CXX=${CROSS_PREFIX}g++
export LD=${CROSS_PREFIX}ld
export AR=${CROSS_PREFIX}ar
export STRIP=${CROSS_PREFIX}strip


CFLAGS="-std=c99 -O3 -Wall -pipe -fpic \
  -fstrict-aliasing -Werror=strict-aliasing \
  -Wno-psabi -Wa,--noexecstack \
  -DANDROID -DNDK_BUILD -DNDEBUG -U_FILE_OFFSET_BITS -D__ANDROID_API__=22"

LDFLAGS="-lm -lz -llog -fPIC -Wl,--no-undefined -D__ANDROID_API__=22"

FFMPEG_FLAGS_COMMON="--target-os=linux \
    --enable-cross-compile \
    --cross-prefix=$CROSS_PREFIX \
    --prefix=$FFMPEG_ROOT\build \
    --sysroot=$TOOLCHAIN/sysroot/ \
    --enable-version3 \
    --enable-optimizations \
    --disable-fast-unaligned \
    --enable-static \
    --disable-shared \
    --disable-symver \
    --disable-programs \
    --disable-doc \
    --disable-avdevice \
    --disable-postproc"
    
    cd $SOURCE

    FFMPEG_FLAGS="$FFMPEG_FLAGS_COMMON"

    if [ $TARGET == 'armv7' ]; then
        ARCH=arm
        CPU=armv7-a
        #arm v7
        FFMPEG_FLAGS="--arch=armv7-a \
          --cpu=cortex-a8 \
          --disable-runtime-cpudetect \
          $FFMPEG_FLAGS"
        EXTRA_CFLAGS="-march=armv7-a -mfpu=neon -mfloat-abi=softfp -mvectorize-with-neon-quad -fpic"
        EXTRA_LDFLAGS="-L$ANDROID_NDK/platforms/android-21/arch-arm/usr/lib/ "
    fi
    
    if [ $TARGET == 'armv8' ]; then
        #arm v8
        ARCH=arm64
        CPU=armv8-a
        FFMPEG_FLAGS="--arch=$ARCH \
          --cpu=$CPU \
          --disable-runtime-cpudetect \
          $FFMPEG_FLAGS"
        EXTRA_CFLAGS="-march=$CPU -fpic"
        EXTRA_LDFLAGS="-L$ANDROID_NDK/platforms/android-21/arch-arm64/usr/lib/ "
    fi

    PREFIX="$DEST/$version" && rm -rf $PREFIX && mkdir -p $PREFIX
    FFMPEG_FLAGS="$FFMPEG_FLAGS --prefix=$PREFIX"

    ./configure $FFMPEG_FLAGS --extra-cflags="$CFLAGS $EXTRA_CFLAGS" --extra-ldflags="$LDFLAGS $EXTRA_LDFLAGS" | tee $PREFIX/configuration.txt
    cp config.* $PREFIX
    [ $PIPESTATUS == 0 ] || exit 1

    make clean
    find . -name "*.o" -type f -delete
    make -j4

    make install

    LD_SONAME="-Wl,-soname,libffmpeg.so"

    echo "===========>>>>>>>"
    
    if [ $TARGET == 'armv7' ]; then
        $CC -o $PREFIX/libffmpeg.so -shared $LDFLAGS $LD_SONAME $EXTRA_LDFLAGS libavutil/*.o libavutil/arm/*.o libavcodec/*.o libavcodec/arm/*.o libavcodec/neon/*.o  compat/*.o libavformat/*.o  libswscale/*.o  libswscale/arm/*.o libswresample/arm/*.o libswresample/*.o
    fi

    if [ $TARGET == 'armv8' ]; then
        cd $SOURCE && ln -s $ANDROID_NDK/platforms/android-21/arch-arm64/usr/lib/crtbegin_so.o
        cd $SOURCE && ln -s $ANDROID_NDK/platforms/android-21/arch-arm64/usr/lib/crtend_so.o

        #arm v8
        $CC -o $PREFIX/libffmpeg.so -shared $LDFLAGS $LD_SONAME $EXTRA_LDFLAGS libavutil/*.o libavutil/aarch64/*.o libavcodec/*.o libavcodec/aarch64/*.o libavcodec/neon/*.o  compat/*.o libavformat/*.o  libswscale/*.o  libswscale/aarch64/*.o libswresample/aarch64/*.o libswresample/*.o
    fi

    echo "----------------------$version -----------------------------"
