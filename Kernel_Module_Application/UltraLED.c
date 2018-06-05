//################################################################################################
//
// Program     : Ultrasonic & Led matrix control Kernel module
// Header file : UltraLED.c
// Authors     : Sarvesh Patil & Nagarjun Chinnari
// Date        : 19 November 2017
//
//################################################################################################

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/input.h>
#include <stdio.h>
#include <stdlib.h>
#include <linux/ioctl.h>
#include <sched.h>
#include <unistd.h>
#include <linux/kernel.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <inttypes.h>
#include <pthread.h>

#define CONFIG 1
#define DISPLAY "/dev/ledmat"
#define DISTANCE "/dev/ultrasonic"

//===========================================
// Global variables & functions declarations
//===========================================

int result;
pthread_mutex_t mutex;

pthread_mutex_t dist_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t disp_lock = PTHREAD_MUTEX_INITIALIZER;


// LED matrix pattern
//=====================
uint8_t LED_pat[10];

uint8_t LED_pat1[10]= {1,235,2,235,1,235,2,235,0,0};
uint8_t LED_pat2[10]= {3,235,4,235,3,235,4,235,0,0};
uint8_t LED_pat3[10]=  {5,235,6,235,5,235,6,235,0,0};

uint8_t array[10][24]= {
		{0x0C,0x01, 0x09,0x00,	0x0A,0x0F, 0x0B,0x07, 0x01,0x00, 0x02,0x7e, 0x03,0x42, 0x04,0x42, 0x05,0x42, 0x06,0x42, 0x07,0x7e, 0x08,0x00,},
		{0x0C,0x01, 0x09,0x00,	0x0A,0x0F, 0x0B,0x07, 0x01,0x00, 0x02,0x08, 0x03,0x18, 0x04,0x28, 0x05,0x08, 0x06,0x08, 0x07,0x3e, 0x08,0x00,},
		{0x0C,0x01, 0x09,0x00,	0x0A,0x0F, 0x0B,0x07, 0x01,0x00, 0x02,0x7e, 0x03,0x02, 0x04,0x02, 0x05,0x7e, 0x06,0x40, 0x07,0x7e, 0x08,0x00,},
		{0x0C,0x01, 0x09,0x00,	0x0A,0x0F, 0x0B,0x07, 0x01,0x00, 0x02,0x7e, 0x03,0x02, 0x04,0x7e, 0x05,0x02, 0x06,0x7e, 0x07,0x02, 0x08,0x7e,},
		{0x0C,0x01, 0x09,0x00,	0x0A,0x0F, 0x0B,0x07, 0x01,0x08, 0x02,0x18, 0x03,0x28, 0x04,0x48, 0x05,0x7e, 0x06,0x08, 0x07,0x08, 0x08,0x00,},
		{0x0C,0x01, 0x09,0x00,	0x0A,0x0F, 0x0B,0x07, 0x01,0x00, 0x02,0x7e, 0x03,0x40, 0x04,0x40, 0x05,0x7e, 0x06,0x02, 0x07,0x02, 0x08,0x7e,},
		{0x0C,0x01, 0x09,0x00,	0x0A,0x0F, 0x0B,0x07, 0x01,0x00, 0x02,0x7e, 0x03,0x40, 0x04,0x40, 0x05,0x7e, 0x06,0x42, 0x07,0x42, 0x08,0x7e,},
		{0x0C,0x01, 0x09,0x00,	0x0A,0x0F, 0x0B,0x07, 0x01,0x00, 0x02,0x7c, 0x03,0x04, 0x04,0x04, 0x05,0x3f, 0x06,0x04, 0x07,0x04, 0x08,0x04,},
		{0x0C,0x01, 0x09,0x00,	0x0A,0x0F, 0x0B,0x07, 0x01,0x00, 0x02,0x7e, 0x03,0x42, 0x04,0x42, 0x05,0x7e, 0x06,0x42, 0x07,0x42, 0x08,0x7e,},
		{0x0C,0x01, 0x09,0x00,	0x0A,0x0F, 0x0B,0x07, 0x01,0x00, 0x02,0x7e, 0x03,0x42, 0x04,0x42, 0x05,0x7e, 0x06,0x02, 0x07,0x02, 0x08,0x7e,},
};

//###############################################################################################################
//====================================
// Driving function area
//====================================
int write_pulse(int fd)
{
    int retValue=0;
    char* writeBuffer;
    
    writeBuffer = (char *)malloc(10);
    while(1)
    {
        retValue = write(fd, writeBuffer, 10);
        if(retValue < 0)
        {
            //printf("Trigger Failure\n");
        }
        else
        {
            //printf("Trigger Success\n");
            break;
        }
        usleep(100000);
    }
    free(writeBuffer);
    return retValue;
}


int read_pulse(int fd)
{
    int retValue=0;
    unsigned int writeBuffer =0;
    while(1)
    {
        retValue = read(fd, &writeBuffer, sizeof(writeBuffer));
        
        if(retValue < 0)
        {
            //printf("Read error\n");
        }
        else
        {
            //printf("Read Success\n");
            break;
        }
        usleep(100000);
    }
    return writeBuffer;
}

//=======================
// Thread function area
//=======================

// Display thread
//=================
void* displayFunction(void* parameters)
{
	int fd3,fd4;
	fd3 = open(DISPLAY, O_RDWR);
	if(fd3<0)
		printf("opening error");
	fd4=ioctl(fd3,1,array);
	if(fd4<0)
	{
		printf("error in ioctl\n");
	} 
	while(1)
	{
		pthread_mutex_lock(&disp_lock);
		fd4=write(fd3,(void*)LED_pat, sizeof(LED_pat));
		if(fd4<0)
			{
				printf("error in reading\n");
			}
		pthread_mutex_unlock(&disp_lock);
	} 
	close(fd3);
	return 0;
}

// Distance thread
//=================
void* distanceFunction(void* parameters)
{
	 int fdUltra;
	 int distance;
	 int i;

	fdUltra=open("/dev/ultrasonic",O_RDWR);

    if(fdUltra<0)
    {
        printf("error could not open Ultra driver\n");
    }

   
    while(1)
    {
        write_pulse(fdUltra);
        distance = read_pulse(fdUltra);
        pthread_mutex_lock(&dist_lock);	
		result = distance * 0.017;
		if(result<120)
		{
			if(result<20)
			{
				for(i=0;i<10;i++)
				{
					LED_pat[i] = LED_pat1[i];
				}
			}
			else if(result<50)
			{
				for(i=0;i<10;i++)
				{
					LED_pat[i] = LED_pat2[i];
				}
		
			}
			else 
			{
				for(i=0;i<10;i++)
				{
					LED_pat[i] = LED_pat3[i];
				}
			
			}
	
			printf("\n %d ", result);
		}
		pthread_mutex_unlock(&dist_lock);	  
        usleep(100000);
    }

	return 0;
}

//###############################################################################
//==================
// Main
//==================

int main() 
{

	//Thread creation

	pthread_t ultra_tid;
	pthread_create (&ultra_tid, NULL, &distanceFunction, NULL);

	pthread_t LED_tid;
	pthread_create (&LED_tid, NULL, &displayFunction, NULL);

	pthread_join(ultra_tid, NULL);
	pthread_join(LED_tid, NULL);

	return 0;
}	
