## Build instructions for GYP/CMake under Ubuntu 18.04

### Prepare folder

Choose an empty folder for the future build, for example **/home/user/TBuild**. It will be named ***BuildPath*** in the rest of this document.

### Obtain your API credentials

You will require **api_id** and **api_hash** to access the Telegram API servers. To learn how to obtain them [click here][api_credentials].

### Install software and required packages

You will need GCC 7.2 and CMake 3.2 installed. To install them and all the required dependencies run

    sudo apt-get install software-properties-common
    sudo apt-get install gcc g++ cmake

    sudo apt-get install git libexif-dev liblzma-dev libz-dev libssl1.0-dev libappindicator-dev libunity-dev libicu-dev libdee-dev libdrm-dev dh-autoreconf autoconf automake build-essential libass-dev libfreetype6-dev libgpac-dev libsdl1.2-dev libtheora-dev libtool libva-dev libvdpau-dev libvorbis-dev libxcb1-dev libxcb-image0-dev libxcb-shm0-dev libxcb-xfixes0-dev libxcb-keysyms1-dev libxcb-icccm4-dev libxcb-render-util0-dev libxcb-util0-dev libxrender-dev libasound-dev libpulse-dev libxcb-sync0-dev libxcb-randr0-dev libx11-xcb-dev libffi-dev libncurses5-dev pkg-config texi2html zlib1g-dev yasm cmake xutils-dev bison python-xcbgen libbsd-dev uuid-dev

You can set the multithreaded make parameter by running

    MAKE_THREADS_CNT=-j8

### Clone source code and prepare libraries

Go to ***BuildPath*** and run

    git clone --recursive https://github.com/AntonCoon/tdesktop.git

    mkdir Libraries
    cd Libraries
    
    sudo apt-get install libpciaccess-dev
    git clone https://salsa.debian.org/xorg-team/lib/libdrm
    cd libdrm
    git checkout libdrm-2.4.95-1
    ./autogen.sh
    ./configure --enable-static
    make
    sudo make install
    cd ..
    
    wget https://launchpad.net/ubuntu/+archive/primary/+sourcefiles/libsm/2:1.2.1-2/libsm_1.2.1.orig.tar.gz
    tar -xzvf libsm_1.2.1.orig.tar.gz
    cd libSM-1.2.1
    ./configure
    make $MAKE_THREADS_CNT
    sudo make install
    cd ..
    
    wget https://launchpad.net/ubuntu/+archive/primary/+sourcefiles/libxext/2:1.3.2-1/libxext_1.3.2.orig.tar.gz
    tar -xzvf libxext_1.3.2.orig.tar.gz
    cd libXext-1.3.2
    ./configure
    make $MAKE_THREADS_CNT
    sudo make install
    cd ..

    git clone https://github.com/ericniebler/range-v3

    git clone https://github.com/telegramdesktop/zlib.git
    cd zlib
    ./configure
    make $MAKE_THREADS_CNT
    sudo make install
    cd ..

    git clone https://github.com/xiph/opus
    cd opus
    git checkout v1.3
    ./autogen.sh
    ./configure
    make $MAKE_THREADS_CNT
    sudo make install
    cd ..

    git clone https://github.com/01org/libva.git
    cd libva
    ./autogen.sh --enable-static
    make $MAKE_THREADS_CNT
    sudo make install
    cd ..

    git clone git://anongit.freedesktop.org/vdpau/libvdpau
    cd libvdpau
    git checkout libvdpau-1.2
    ./autogen.sh --enable-static
    make $MAKE_THREADS_CNT
    sudo make install
    cd ..

    git clone https://github.com/FFmpeg/FFmpeg.git ffmpeg
    cd ffmpeg
    git checkout release/3.4

    ./configure --prefix=/usr/local \
    --enable-protocol=file --enable-libopus \
    --disable-programs \
    --disable-doc \
    --disable-network \
    --disable-everything \
    --enable-hwaccel=h264_vaapi \
    --enable-hwaccel=h264_vdpau \
    --enable-hwaccel=mpeg4_vaapi \
    --enable-hwaccel=mpeg4_vdpau \
    --enable-decoder=aac \
    --enable-decoder=aac_at \
    --enable-decoder=aac_fixed \
    --enable-decoder=aac_latm \
    --enable-decoder=aasc \
    --enable-decoder=alac \
    --enable-decoder=alac_at \
    --enable-decoder=flac \
    --enable-decoder=gif \
    --enable-decoder=h264 \
    --enable-decoder=h264_vdpau \
    --enable-decoder=hevc \
    --enable-decoder=mp1 \
    --enable-decoder=mp1float \
    --enable-decoder=mp2 \
    --enable-decoder=mp2float \
    --enable-decoder=mp3 \
    --enable-decoder=mp3adu \
    --enable-decoder=mp3adufloat \
    --enable-decoder=mp3float \
    --enable-decoder=mp3on4 \
    --enable-decoder=mp3on4float \
    --enable-decoder=mpeg4 \
    --enable-decoder=mpeg4_vdpau \
    --enable-decoder=msmpeg4v2 \
    --enable-decoder=msmpeg4v3 \
    --enable-decoder=opus \
    --enable-decoder=pcm_alaw \
    --enable-decoder=pcm_alaw_at \
    --enable-decoder=pcm_f32be \
    --enable-decoder=pcm_f32le \
    --enable-decoder=pcm_f64be \
    --enable-decoder=pcm_f64le \
    --enable-decoder=pcm_lxf \
    --enable-decoder=pcm_mulaw \
    --enable-decoder=pcm_mulaw_at \
    --enable-decoder=pcm_s16be \
    --enable-decoder=pcm_s16be_planar \
    --enable-decoder=pcm_s16le \
    --enable-decoder=pcm_s16le_planar \
    --enable-decoder=pcm_s24be \
    --enable-decoder=pcm_s24daud \
    --enable-decoder=pcm_s24le \
    --enable-decoder=pcm_s24le_planar \
    --enable-decoder=pcm_s32be \
    --enable-decoder=pcm_s32le \
    --enable-decoder=pcm_s32le_planar \
    --enable-decoder=pcm_s64be \
    --enable-decoder=pcm_s64le \
    --enable-decoder=pcm_s8 \
    --enable-decoder=pcm_s8_planar \
    --enable-decoder=pcm_u16be \
    --enable-decoder=pcm_u16le \
    --enable-decoder=pcm_u24be \
    --enable-decoder=pcm_u24le \
    --enable-decoder=pcm_u32be \
    --enable-decoder=pcm_u32le \
    --enable-decoder=pcm_u8 \
    --enable-decoder=pcm_zork \
    --enable-decoder=vorbis \
    --enable-decoder=wavpack \
    --enable-decoder=wmalossless \
    --enable-decoder=wmapro \
    --enable-decoder=wmav1 \
    --enable-decoder=wmav2 \
    --enable-decoder=wmavoice \
    --enable-encoder=libopus \
    --enable-parser=aac \
    --enable-parser=aac_latm \
    --enable-parser=flac \
    --enable-parser=h264 \
    --enable-parser=hevc \
    --enable-parser=mpeg4video \
    --enable-parser=mpegaudio \
    --enable-parser=opus \
    --enable-parser=vorbis \
    --enable-demuxer=aac \
    --enable-demuxer=flac \
    --enable-demuxer=gif \
    --enable-demuxer=h264 \
    --enable-demuxer=hevc \
    --enable-demuxer=m4v \
    --enable-demuxer=mov \
    --enable-demuxer=mp3 \
    --enable-demuxer=ogg \
    --enable-demuxer=wav \
    --enable-muxer=ogg \
    --enable-muxer=opus

    make $MAKE_THREADS_CNT
    sudo make install
    cd ..

    git clone https://git.assembla.com/portaudio.git
    cd portaudio
    git checkout 396fe4b669
    ./configure
    make $MAKE_THREADS_CNT
    sudo make install
    cd ..

    git clone git://repo.or.cz/openal-soft.git
    cd openal-soft
    git checkout openal-soft-1.19.1
    cd build
    if [ `uname -p` == "i686" ]; then
    cmake -D LIBTYPE:STRING=STATIC -D ALSOFT_UTILS:BOOL=OFF ..
    else
    cmake -D LIBTYPE:STRING=STATIC ..
    fi
    make $MAKE_THREADS_CNT
    sudo make install
    cd ../..

    git clone https://github.com/openssl/openssl
    cd openssl
    git checkout OpenSSL_1_0_2-stable
    ./config
    make $MAKE_THREADS_CNT
    sudo make install
    cd ..

    git clone https://github.com/xkbcommon/libxkbcommon.git
    cd libxkbcommon
    ./autogen.sh --disable-x11
    make $MAKE_THREADS_CNT
    sudo make install
    cd ..

    git clone git://code.qt.io/qt/qt5.git qt5_6_2
    cd qt5_6_2
    perl init-repository --module-subset=qtbase,qtimageformats
    git checkout v5.6.2
    cd qtimageformats && git checkout v5.6.2 && cd ..
    cd qtbase && git checkout v5.6.2 && cd ..
    cd qtbase && git apply ../../../tdesktop/Telegram/Patches/qtbase_5_6_2.diff && cd ..
    cd qtbase/src/plugins/platforminputcontexts
    git clone https://github.com/telegramdesktop/fcitx.git
    git clone https://github.com/telegramdesktop/hime.git
    git clone https://github.com/telegramdesktop/nimf.git
    cd ../../../..

    OPENSSL_LIBS='-L/usr/local/ssl/lib -lssl -lcrypto' ./configure -prefix "/usr/local/tdesktop/Qt-5.6.2" -release -force-debug-info -opensource -confirm-license -qt-zlib -qt-libpng -qt-libjpeg -qt-freetype -qt-harfbuzz -qt-pcre -qt-xcb -qt-xkbcommon-x11 -no-opengl -no-gtkstyle -static -openssl-linked -nomake examples -nomake tests

    make $MAKE_THREADS_CNT
    sudo make install
    cd ..

    git clone https://chromium.googlesource.com/external/gyp
    cd gyp
    git checkout 702ac58e47
    git apply ../../tdesktop/Telegram/Patches/gyp.diff
    cd ..

    git clone https://chromium.googlesource.com/breakpad/breakpad
    cd breakpad
    git checkout bc8fb886
    git clone https://chromium.googlesource.com/linux-syscall-support src/third_party/lss
    cd src/third_party/lss
    git checkout a91633d1
    cd ../../..
    ./configure
    make $MAKE_THREADS_CNT
    sudo make install
    cd src/tools
    ../../../gyp/gyp  --depth=. --generator-output=.. -Goutput_dir=../out tools.gyp --format=cmake
    cd ../../out/Default
    cmake .
    make $MAKE_THREADS_CNT dump_syms
    cd ../../..

### Building the project

Go to ***BuildPath*/tdesktop/Telegram** and run (using [your **api_id** and **api_hash**](#obtain-your-api-credentials))

    gyp/refresh.sh --api-id YOUR_API_ID --api-hash YOUR_API_HASH

To make Debug version go to ***BuildPath*/tdesktop/out/Debug** and run

    make $MAKE_THREADS_CNT

To make Release version go to ***BuildPath*/tdesktop/out/Release** and run

    make $MAKE_THREADS_CNT

You can debug your builds from Qt Creator, just open **CMakeLists.txt** from ***BuildPath*/tdesktop/out/Debug** and launch with debug.

[api_credentials]: api_credentials.md
