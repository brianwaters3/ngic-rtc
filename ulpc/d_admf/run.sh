#!/bin/bash

BINDIR=./bin/
BIN=d_admf
CONFDIR=config/
CONF=d_admf.json

clear

if [ ! -d "$BINDIR" ]; 
then
	echo "Folder ${BINDIR} is not present"
	exit 1
fi

if [ ! -d "$CONFDIR" ]; 
then
	echo "Folder ${CONFDIR} is not present"
	exit 1
fi

if [ ! -e "$BINDIR$BIN" ];
then
	echo "File ${BINDIR}${BIN} is not present"
	exit 1
fi

if [ ! -e "$CONFDIR$CONF" ];
then
	echo "File ${CONFDIR}${CONF} is not present"
	exit 1
fi

$BINDIR$BIN -f $CONFDIR$CONF
#./bin/df -f config/df.json
