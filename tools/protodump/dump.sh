#!/bin/bash

./luac $1
xxd -p luac.out > $1.out
./lua build_proto.lua $1.out > lua.dump
rm luac.out
rm $1.out
