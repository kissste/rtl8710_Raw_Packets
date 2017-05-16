##!/bin/bash 

# compile ameba tools
make -C pick_padding_checksum

# download librtlhttpd
git clone -b rtl8710 https://f28@bitbucket.org/f28/librtlhttpd.git

cd librtlhttpd

# download heatshrink library
git submodule init
git submodule update

cd ..


#download SDK
#cd ..
#git clone https://github.com/pvvx/RTL00MP3.git
