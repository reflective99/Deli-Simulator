#!/bin/bash
g++ -o deli thread.o -g deli.cc libinterrupt.a -ldl
./deli 3 sw.in0 sw.in1 sw.in2 sw.in3 sw.in4
