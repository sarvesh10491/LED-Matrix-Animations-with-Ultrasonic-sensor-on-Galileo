User space program to generate animation on LED matrix using ultrasonic sensor

   Following project is used to control the PWM and the intensity of RGB led patterns.

Getting Started

    These instructions will get you a copy of the project up and running on your local machine for development and testing purposes. 
    See deployment for notes on how to deploy the project on a live system.

Prerequisites

  Linux kernel (preferably linux 2.6.19 and above)
  GNU (preferably gcc 4.5.0 and above)

Installing

Download below files in user directory on your machine running linux distribution.

   1) UltraLED.c
   2) UltraLED_lib.h
   3) UltraLED_ctrl.sh
   4) Makefile


Deployment

   Make following connections on Galileo board.
	1] Connect Trigger pin of Ultrasonic sensor to Pin 2
	2] Connect Echo pin of Ultrasonic sensor to Pin 3

	1] Connect Din pin of LED matrix to Pin 11
	2] Connect CS pin of LED matrix to Pin 12
	3] Connect CLK pin of LED matrix to Pin 13




   Open the terminal & go to directory where files in installing part have been downloaded. [cd <Directory name>] 

   In the make file we gave the path as "/opt/iot-devkit/1.7.2/sysroots/x86_64-pokysdk-linux/usr/bin/i586-poky-linux" for compiler

   if you have a different location then change it.
   
   Use below command to to compile 
 
   make

   Now send the shell script and object file to the galileo
   board using the follwing command (change IP & home
   accordingly)

   sudo scp UltraLED root@192.168.1.100:/home

   sudo scp UltraLED_ctrl.sh root@192.168.1.100:/home

   Connect to Galileo board with root login


   On Galileo2 board, ensure that 666(rw- rw- rw-) file
   permissions exist for /dev/spidev1.0

   You can check by the following command 
   ls -lrt /dev/spidev1.0 

   Otherwise change using the following command with root user
   chmod 666 /dev/spidev1.0

   Change permission of shell script to 755
   chmod 755 UltraLED_ctrl.sh

   Once above completed then run the below command to execute
   the program code
   
   cd /home
   sh UltraLED_ctrl.sh


Expected results

   It asks for selection of animation pattern to be displayed on LED matrix.

   Subsequent animation is set to run on LED matrix. Speed & pattern changes as per object movement happening in front of ultrasonic sensor.

Built With

  Linux 4.10.0-28-generic
  x86_64 GNU/Linux
  64 bit x86 machine

Authors

Sarvesh Patil 
Nagarjun chinnari 

License

This project is licensed under the ASU License

