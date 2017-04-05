#!/bin/bash

DISC_SIZE=$1
DATA_SIZE=`du -h -I ".svn" -I ".DS_Store" cd | sed -e 's/[a-z,A-Z]//g'`
DUMMY_SIZE=$((((DISC_SIZE-DATA_SIZE)-10)*1000000))

if [ "$DUMMY_SIZE" -gt 0 ]; then
  echo "Generating 00DUMMY.DAT ($DUMMY_SIZE Zero'ed Bytes)"
  dd if=/dev/zero of=cd/00DUMMY.DAT bs=$DUMMY_SIZE count=1
fi
