#!/bin/bash

DIR="/proc/m0dLiSt"
productor() {
	for i in $(seq $1)
	do
		echo -n "PROD $i:" `date +%H:%M:%S` 
		echo "add $i" > $DIR
	done
}

consumidor() {
	for i in $(seq $1)
	do
		echo -n "CONS $i:" `date +%H:%M:%S` 
		echo "remove $i" > $DIR
		echo " elems: $(cat $DIR)"
	done
}


productor $1 &
consumidor $1 &
wait $(jobs -p)
