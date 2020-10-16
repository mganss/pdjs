#!/bin/bash

$PD -nogui -stderr -batch -open test.${TRIPLET}.pd -send "test bang" \
    > result.${TRIPLET}.out.txt 2> result.${TRIPLET}.err.txt

rm -f result.${TRIPLET}.txt

cat result.${TRIPLET}.out.txt result.${TRIPLET}.err.txt \
    | egrep -v "priority [0-9]+ scheduling failed" \
    > result.${TRIPLET}.txt
