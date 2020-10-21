#!/bin/bash

if [ "$OS" = "Windows_NT" ]; then
    export TRIPLET="x64-windows-static"
    pacman -q --noconfirm -S diffutils
elif [ "$OSTYPE" = "linux-gnu" ]; then
    if [ "$HOSTTYPE" = "x86_64" ]; then
        export TRIPLET="x64-linux"
    fi
fi

export PD="pd/${TRIPLET}/bin/pd"

. run.sh

diff --strip-trailing-cr result.${TRIPLET}.txt ./result.txt

SUCCESS=$?

if [ "$SUCCESS" = 0 ]; then
    echo "Test completed successfully."
else
    echo "Test failed."
fi

exit $SUCCESS
