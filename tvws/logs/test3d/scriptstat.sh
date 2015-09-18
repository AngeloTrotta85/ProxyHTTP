#!/bin/bash

MINROWS=$1

for dir in `ls`
do
	if [ -d $dir ]; then
		echo $dir
		cd $dir

		for i in `ls test*`; 
		do 
			row=$(wc -l < $i); 
			if [ "$row" -gt "$MINROWS" ]; then 
				cat $i | awk 'BEGIN{C=0;S=0}{S+=$2;C+=1}END{printf("%d\n", S/C)}'; 
			fi 
		done | awk 'BEGIN{S=0;C=0}{S+=$1;C+=1}END{printf("%d\n", S/C)}'

		for i in `ls test*`; 
		do 
			row=$(wc -l < $i); 
			if [ "$row" -gt "$MINROWS" ]; then 
				cat $i | awk 'BEGIN{C=0;S=0}{S+=$2;C+=1}END{printf("%d\n", S/C)}'; 
			fi 
		done | awk 'BEGIN{S=0;C=0}{S+=$1;C+=1}END{printf("%d\n", S/C)}' > stat.dat

		cat stat.dat

		cd ..
	fi
done
