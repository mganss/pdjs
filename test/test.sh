#!/bin/bash

if [ "$OS" = "Windows_NT" ]; then
    export TRIPLET="x64-windows-static"
    pacman -q --needed --noconfirm -S diffutils
elif [ "$OSTYPE" = "linux-gnu" ]; then
    if [ "$HOSTTYPE" = "x86_64" ]; then
        export TRIPLET="x64-linux"
    fi
fi

export PD="pd/${TRIPLET}/bin/pd"

. run.sh

V8_VERSION=`find ../vcpkg*/ -type f -name v8_monolith.pc -exec egrep -o 'Version: [0-9.]+' {} \; | egrep -o '[0-9.]+' | head -n1`
sed "s/pdjs version.*/pdjs version ${VERSION} (v8 version ${V8_VERSION})/" < ./result.txt > ./expected.txt

EXCEPTION_REGEX="s/^.+\.js:[0-9]+:(.+)/\1/"
sed -i -r "${EXCEPTION_REGEX}" ./expected.txt
sed -r "${EXCEPTION_REGEX}" < ./result.${TRIPLET}.txt > ./actual.txt

ERROR_REGEX="s/^(error: Error.*) '.*\.js':/\1/"
sed -i -r "${ERROR_REGEX}" ./expected.txt
sed -i -r "${ERROR_REGEX}" ./actual.txt

JSOBJECT_REGEX="s/jsobject [0-9]+/jsobject/"
sed -i -r "${JSOBJECT_REGEX}" ./expected.txt
sed -i -r "${JSOBJECT_REGEX}" ./actual.txt

diff --strip-trailing-cr actual.txt ./expected.txt

SUCCESS=$?

if [ "$SUCCESS" = 0 ]; then
    echo "Test completed successfully."
else
    echo "Test failed."
fi

exit $SUCCESS
