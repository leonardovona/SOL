#!/bin/bash

if [ -z "$1" ]; then
    echo "usage: analisi.sh LOGFILE" 1>&2;
    exit 1;
fi

if [ ! -f "$1" ]; then
    echo "unvalid LOGFILE" 1>&2;
    exit 1;
fi

echo CUSTOMERS;

FIRST=1;

for ELEMENT in $(cat "$1"); do
	IFS=';' DATA=( $ELEMENT );
	if [ ${DATA[0]} == "customer" ]; then
		echo id: ${DATA[1]} '|' purchased products: ${DATA[2]} '|' time in supermarket: ${DATA[3]} '|' time in queue: ${DATA[4]} '|' checkout lines visited: ${DATA[5]} '|';
	else
		if [ $FIRST -eq 1 ]; then
			echo CHECKOUT LINES;
			FIRST=0;
		fi;
		if [ ${DATA[3]} -eq 0 ]; then
			MEAN_SERVICE_TIME=0;
		else
			MEAN_SERVICE_TIME=$(echo "scale=3; ${DATA[4]}/${DATA[3]}" | bc -q);
		fi
	
		echo id: ${DATA[1]} '|' processed products : ${DATA[2]} '|' served customers: ${DATA[3]} '|' open time: ${DATA[4]} '|' mean service time: $MEAN_SERVICE_TIME '|' \
		closures: ${DATA[5]} '|';	
	fi
done

#for CUSTOMER in $(cat "$1" | grep "customer"); do
#    IFS=';' DATA_C=( $CUSTOMER );
#   echo id: ${DATA_C[1]} '|' purchased products: ${DATA_C[2]} '|' time in supermarket: ${DATA_C[3]} '|' time in queue: ${DATA_C[4]} '|' checkout lines visited: ${DATA_C[5]} '|';
#done

#echo CHECKOUT LINES;
#IFS=' ';

#for CHECKOUT_LINE in $(cat "$1" | grep "checkout_line"); do
#	echo $CHECKOUT_LINE
 #	IFS=';' DATA_L=( $CHECKOUT_LINE );
#	MEAN_SERVICE_TIME=$(echo "scale=3; ${DATA_L[4]}/${DATA_L[3]}" | bc -q)
#	SERVICE_TIME=$(( (${DATA[4]}) / ${DATA[3]} ))
#	echo id: ${DATA_L[1]} '|' processed products : ${DATA_L[2]} '|' served customers: ${DATA_L[3]} '|' open time: ${DATA_L[4]} '|' mean service time: $MEAN_SERVICE_TIME '|' \
#	closures: ${DATA_L[5]} '|';	
#done
