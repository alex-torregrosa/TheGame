#!/bin/bash


./Game $1 $2 $2 $2 -s $3 -i default.cnf -o tester.out 2>&1 | grep "info: player(s)";