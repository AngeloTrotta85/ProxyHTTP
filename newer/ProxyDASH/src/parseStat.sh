#!/bin/bash

DIRS=`find logs -maxdepth 1 -mindepth 1 -type d`
for dir in $DIRS
do
	echo $dir
	for test in {1..3}
	do
		for run in {1..1}
		do
			cat "$dir/test${test}_${run}*" > ${dir}/test_total_${test}_${run}
		done
	done
	echo $FILES
	
done
echo $DIRS
exit 0
FILES="logs/*"
TMP="test_"

	for f in $FILES
	do
		TMPNAME=$($f | cut -d'_' -f 1)
		#echo $f | cut -d'_' -f 1
		echo $TMPNAME 
		if [ "$TMP" == "$TMPNAME" ]
		then
			touch $f | cut -d'_' -f 1
			echo $f | cut -d'_' -f 1
		fi
	done
