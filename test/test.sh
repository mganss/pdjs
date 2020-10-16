#!/bin/bash

if [ "$OS" = "Windows_NT" ]; then
    export TRIPLET="x64-windows-static"
    pacman -S curl unzip -q --noconfirm
    curl -s http://msp.ucsd.edu/software.html | egrep -o 'http[^"]*\.msw\.zip' | head -1 | xargs -n 1 curl -s -o /tmp/pd.zip
    unzip -o -qq /tmp/pd.zip
    export PD="`ls | grep pd-`/bin/pd.com"
elif [ "$OSTYPE" = "linux-gnu" ]; then
    if [ "$HOSTTYPE" = "x86_64" ]; then
        export TRIPLET="x64-linux"
    fi
    export PD="`which pd`"
fi

. run.sh

diff --strip-trailing-cr result.${TRIPLET}.txt ./result.txt
