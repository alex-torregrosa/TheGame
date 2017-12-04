#!/bin/bash
echo "./Game Sugus_Pere $1 $1 $1 -s $2 -i default.cnf -o default.out 2> cerr.file;"
./Game Sugus_Pere $1 $1 $1 -s $2 -i default.cnf -o default.out 2> cerr.file;
cp default.out Viewer/default.ork

tail cerr.file | grep "info: player(s)";
cat cerr.file|grep 0_state;
rm default.out cerr.file;
