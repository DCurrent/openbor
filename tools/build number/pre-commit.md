#!/bin/sh
#pre-hook to log master commit count then add branch ahead of master and record into a build_number.h file.
countM=$(git rev-list master --count) #master commit count
countB=$(($(git rev-list master.. --count) + 2)) #branch ahead of master commit count +1 for the merge +1 for this commit.
total=$(($countM + $countB))
echo "#define BUILD_NUMBER $total
#define BUILD_STRING \"$total\"
" > source/build_number.h
git add source/build_number.h
exit 0