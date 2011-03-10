#!/usr/bin/env bash
temp=$(tempfile 2>/dev/null)
if [[ ! $temp ]] ; then
	temp=/tmp/fixindent.tmp
fi
rm -rf $temp
if [[ -e $1 ]] ; then
	cat $1 | perl -pe 'if((@x = /^(?:    )+/g)) { s|^(    )+|"\t" x (length($x[0]) / 4)|eg; }' >> $temp
	if [[ -e $temp ]] ; then
		mv $temp $1
	fi
fi
