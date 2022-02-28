#!/bin/bash
#
# This file does remove temporary build folders, builds the tests, runs them and 
# generates the code coverage test report files
#

rm -rf build bin
mkdir build
cd build && cmake .. && make

