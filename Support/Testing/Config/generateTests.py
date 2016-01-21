#!/usr/bin/env python
##
## Copyright (c) 2014, Facebook, Inc.
## All rights reserved.
##
## This source code is licensed under the University of Illinois/NCSA Open
## Source License found in the LICENSE file in the root directory of this
## source tree. An additional grant of patent rights can be found in the
## PATENTS file in the same directory.
##

def printLine( testList, outFile ):
    uniqueList = [];
    prevTest = "";
    for test in testList:
        if test != prevTest:
            uniqueList.append(test);
            prevTest = test

    line = '"';
    first = True;
    for unique in uniqueList:
       if (not first):
           line = line + "|"
       first = False;
       line = line + str(unique)

    line = line + '"\n'
    outFile.write(line)

# list.txt contains a raw list of all tests run by lldb, with
# an asterisk inserted roughly every 100 lines
listFile = open('list.txt', 'r')
outFile = open('testConfig.txt', 'w')
testList = [];
set = 1;
for line in listFile:
    if line[0] == '*':
        outFile.write("*" + str(set) + "*\n");
        set = set + 1;
        printLine( testList , outFile);
        testList = [];
    else:
        testList.append(line.rstrip());
