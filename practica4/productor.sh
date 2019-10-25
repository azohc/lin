#!/bin/bash


productor() {
	for i in $(seq $1)
	do
		echo $i
		# print timestamp
		# insertar i
		# cat lista
	done
}

consumidor() {
	for i in $(seq $1)
	do
		echo 1
		# print timestamp
		# insertar i
		# cat lista
	done
}


productor $1 &
consumidor $1 &
wait $(jobs -p)
