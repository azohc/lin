#!/bin/bash
while true
do
	for (( i = 0 ; i <= 7 ; i++ ))
	do
		sudo ./ledctl_invoke $i ; sleep 1
	done
done