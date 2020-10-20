#!/bin/bash

$PD -noprefs -nrt -nogui -stderr -batch -open test.${TRIPLET}.pd -send "test bang" \
    > result.${TRIPLET}.out.txt 2> result.${TRIPLET}.err.txt

rm -f result.${TRIPLET}.txt

cat result.${TRIPLET}.out.txt result.${TRIPLET}.err.txt \
    > result.${TRIPLET}.txt
