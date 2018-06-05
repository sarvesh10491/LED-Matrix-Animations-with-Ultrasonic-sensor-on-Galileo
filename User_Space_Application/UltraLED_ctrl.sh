#!/bin/bash

flag=0

echo "============================================================="
echo "LED matrix animation program"
echo "============================================================="
echo ""
echo "1. Running Dog"
echo "2. Dancing man"
echo "3. Emoji face"
echo ""
read -p  'Enter Number of animation you wish to see : ' ani_num

echo ""
echo "============================================================="
echo ""

#=============================================================
# Checks for any of the following invalid input from user
# 1] input value out of bounds
# 2] No input
# 3] Input not an integer
#
# If it finds invalid input, sets flag to 1
#==============================================================

if ([ "$ani_num" -lt "1" ] || [ "$ani_num" -gt "3" ] || [ -z "$ani_num" ] || [ "`echo $ani_num |egrep ^[[:digit:]]+$`" = "" ]) 2> /dev/null
	then
	echo "Invalid Animation number given as input."
	export flag=1
fi

echo ""
# flag == 1 indicates there was error in one input
# Execution will not start in that case
#===========================================================
if [ $flag -eq "1" ]
	then
	echo "Terminationg execution due to invalid input."
else
	echo "Initiating program execution."
	echo ""
	./UltraLED $ani_num
fi
echo ""
