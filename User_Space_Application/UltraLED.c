//################################################################################################
//
// Program     : LED matrix control with Ultrasound sensor interface
// Source file : UltraLED.c
// Authors     : Sarvesh Patil & Nagarjun Chinnari
// Date        : 10 November 2017
//
//################################################################################################

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "UltraLED_lib.h"


// Define Trigger & Echo pins
#define TRIG 2
#define ECHO 3


// Thread function to control Ultrasonic sensor
//================================================
void * distance_tfunc()
{
	Ultrasonic_ctrl( TRIG, ECHO);

	return 0;
}


// Thread function to control LED matrix
//=======================================
void * display_tfunc()
{		
	LEDmatrix_ctrl();	
	
	return 0;
}

//#########################################################
//===============================
// Main
//===============================
int main(int argc, char **argv)
{    
    //==================================================
    // Capturing inputs provided by user during runtime
    // ani_sel  = animation selection
    //==================================================
    ani_sel = atoi(argv[1]);

	pthread_t distance_tid;
	pthread_t display_tid;
	int tret;

	// Setup GPIO pins
    //=================
    IO_Setup( TRIG, "out");
    IO_Setup( ECHO, "in");

    // LED matrix setup
    //==================
    LEDmat_setup();
    
   
    // Ultrasonic sensor thread create
	tret=pthread_create(&distance_tid,NULL,distance_tfunc,NULL);
    if (tret)
    {
      printf("ERROR. Return code from pthread_create() for Ultrasonic thread is %d\n", tret);
      exit(-1);
    }

    // LED matrix thread create
    tret=pthread_create(&display_tid,NULL,display_tfunc,NULL);
    if (tret)
    {
      printf("ERROR. Return code from pthread_create() for LED matrix thread is %d\n", tret);
      exit(-1);
    }

	while(1){}; // Runs forever


    //=================================
	// Closing opened file descriptors
	//=================================
	IO_Release();


return 0;
}

//##################################################
// End of UltraLED source file
//##################################################