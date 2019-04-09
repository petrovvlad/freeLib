#!/bin/bash
number=`awk '{print $3}' ../../freeLib/src/build_number.h`
let number++
echo "#define BUILD ""$number" | tee ../../freeLib/src/build_number.h
