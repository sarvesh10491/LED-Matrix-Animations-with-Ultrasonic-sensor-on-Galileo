Kernel space program to generate animation on LED matrix using ultrasonic sensor

   Following project is used to generate animation on LED matrix using ultrasonic sensor .

Getting Started

    These instructions will get you a copy of the project up and running on your local machine for development and testing purposes. 
    See deployment for notes on how to deploy the project on a live system.

Prerequisites

  Linux kernel (preferably linux 2.6.19 and above)
  GNU (preferably gcc 4.5.0 and above)

Installing

Download below files in user directory on your machine running linux distribution.

   1) UltraLED.c
   2) LED_mod.c
   3) Ultra_mod.c
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

   Now send the object file to the galileo
   board using the follwing command (change IP & home
   accordingly)

   sudo scp UltraLED root@192.168.0.100:/home

   sudo scp LED_mod.ko root@192.168.0.100:/home

   sudo scp Ultra_mod.ko root@192.168.0.100:/home

   Connect to Galileo board with root login

   
   Change permission of object file to 755
   chmod 755 /home/UltraLED

   Once above completed then run the below command to execute
   the program code
   
   cd /home
   rmmod spidev
   insmod Ultra_mod.ko
   insmod LED_mod.ko
   ./UltraLED


Expected results

   pattern changes as per object movement happening in front of ultrasonic sensor.
   when within 0 to 20 cm from sensor, it displays 0,1 & 2
   when within 20 to 50 cm from sensor, it displays 0,3 & 4
   when within 50 cm and above from sensor, it displays 0,5 & 6
	

Built With

  Linux 4.10.0-28-generic
  x86_64 GNU/Linux
  64 bit x86 machine

Authors

Sarvesh Patil 
Nagarjun chinnari 

License

This project is licensed under the ASU License

