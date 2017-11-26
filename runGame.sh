#!/bin/bash

./Game Sugus_Pere Demo Demo Demo -s $1 -i default.cnf -o default.out 2> cerr.file;
cp default.out Viewer/default.ork

tail cerr.file;
cat cerr.file|grep state;
rm default.out cerr.file;