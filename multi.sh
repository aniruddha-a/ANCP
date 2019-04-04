#!/bin/bash 
for i in `seq 1 30`
do
# ./client localhost &
./client $HOST  &
 #sleep 1
done
