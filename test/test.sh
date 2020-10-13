#!/bin/bash

if [ "$OS" = "Windows_NT" ]; then
    TRIPLET="x64-windows"
    pacman -S curl unzip -q --noconfirm
    curl -s http://msp.ucsd.edu/software.html | egrep -o 'http[^"]*\.msw\.zip' | head -1 | xargs -n 1 curl -s -o /tmp/pd.zip
    unzip -o -qq /tmp/pd.zip
    PD="`ls | grep pd-`/bin/pd.com"
fi

$PD -nogui -stderr -batch -open test.${TRIPLET}.pd -send "test bang" 2>&1 | diff --strip-trailing-cr - ./result.txt
