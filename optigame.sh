#!/bin/bash

 ./Game -s $5 $1 $2 $3 $4 -i default.cnf -o /dev/null 2>&1|grep "info: player "