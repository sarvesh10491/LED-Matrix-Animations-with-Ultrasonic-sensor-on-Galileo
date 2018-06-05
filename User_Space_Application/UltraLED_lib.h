//################################################################################################
//
// Program     : Library for LED matrix control with Ultrasound sensor interface
// Source file : UltraLED_lib.h
// Authors     : Sarvesh Patil & Nagarjun Chinnari
// Date        : 10 November 2017
//
//################################################################################################


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <linux/input.h>
#include <poll.h>
#include <sys/timerfd.h>
#include <sys/time.h>
#include <pthread.h>
#include <fcntl.h>
#include <inttypes.h>
#include <math.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <stdint.h>
#include <linux/spi/spidev.h>

#define DEVICE "/dev/spidev1.0" 

// GPIO pin multiplexing table.
// Each column represents corresponding IO0 to IO13 shield pins.
// Each row represents specific function select GPIO pins.
// If value is "-1", no gpio setting required for that pin.
//=====================================================================================
//                   IO : 0  1  2  3  4  5  6  7  8  9 10 11 12 13       Functions :
int gpio_ctrl[4][14] = {{32,28,34,16,36,18,20,-1,-1,22,26,24,42,30},  // Level shifter
						{-1,45,77,76,-1,66,68,-1,-1,70,74,44,-1,46},  // MUX select 1
						{-1,-1,-1,64,-1,-1,-1,-1,-1,-1,-1,72,-1,-1},  // MUX select 2
						{11,12,13,14, 6, 0, 1,38,40, 4,10, 5,15, 7}}; // Linux gpio pins


// Variable declarations area
//============================

// For Ultrasonic sensor
int fd_export;
int fd[14];
int fdechv,fdech,fdtrig;

int trig_flag=0;	
char buf[256],buf_1;
int len,i,j;


// For LED matrix patterns
struct spi_ioc_transfer tr = {0};

int ani_sel;
uint8_t Led_write[2];

typedef struct 
{
	uint8_t led [16];

}pattern;


//++++++++++++++++++++
// Animation patterns
//++++++++++++++++++++

// Dog running pattern
	pattern pat1_towards_1={
		{	
			0x01, 0x06, // Pattern bytes
			0x02, 0x06,	
			0x03, 0x06,
			0x04, 0x83,
			0x05, 0x7F,
			0x06, 0x24,
			0x07, 0x22,
			0x08, 0x63,
		}
	};

	pattern pat1_towards_2={
		{
			0x01, 0x0c, // Pattern bytes
			0x02, 0x06,	
			0x03, 0x06,
			0x04, 0x03,
			0x05, 0x7F,
			0x06, 0xA2,
			0x07, 0x32,
			0x08, 0x16,
		}
	};
	pattern pat1_away_1={
		{
			0x01, 0x60, // Pattern bytes
			0x02, 0x60,	
			0x03, 0x60,
			0x04, 0xC1,
			0x05, 0xFE,
			0x06, 0x24,
			0x07, 0x44,
			0x08, 0xC6,
		}
	};

	pattern pat1_away_2={
		{
			0x01, 0x30, // Pattern bytes
			0x02, 0x60,	
			0x03, 0x60,
			0x04, 0xC0,
			0x05, 0xFE,
			0x06, 0x45,
			0x07, 0x4C,
			0x08, 0x68,
		}
	};


// Man dancing pattern
	pattern pat2_towards_1={
		{	
			0x01, 0x1C, // Pattern bytes
			0x02, 0x1C,	
			0x03, 0x48,
			0x04, 0x3E,
			0x05, 0x09,
			0x06, 0x10,
			0x07, 0x28,
			0x08, 0x46,
		}
	};

	pattern pat2_towards_2={
		{
			0x01, 0x1C, // Pattern bytes
			0x02, 0x1C,	
			0x03, 0x09,
			0x04, 0x3E,
			0x05, 0x48,
			0x06, 0x04,
			0x07, 0x0A,
			0x08, 0x31,

		}
	};
	pattern pat2_away_1={
		{
			0x01, 0x38, // Pattern bytes
			0x02, 0x38,	
			0x03, 0x10,
			0x04, 0x7C,
			0x05, 0x92,
			0x06, 0x10,
			0x07, 0x7C,
			0x08, 0x82,
		}
	};

	pattern pat2_away_2={
		{
			0x01, 0x38, // Pattern bytes
			0x02, 0x38,	
			0x03, 0x92,
			0x04, 0x7C,
			0x05, 0x10,
			0x06, 0x92,
			0x07, 0x7C,
			0x08, 0x00,
		}
	};

// Emoji face pattern
	pattern pat3_towards_1={
		{	
			0x01, 0x00, // Pattern bytes
			0x02, 0x66,	
			0x03, 0x66,
			0x04, 0x00,
			0x05, 0x18,
			0x06, 0x42,
			0x07, 0x3C,
			0x08, 0x00,
		}
	};

	pattern pat3_towards_2={
		{
			0x01, 0x00, // Pattern bytes
			0x02, 0x00,	
			0x03, 0x66,
			0x04, 0x00,
			0x05, 0x18,
			0x06, 0x42,
			0x07, 0x7E,
			0x08, 0x3C,

		}
	};
	pattern pat3_away_1={
		{
			0x01, 0x00, // Pattern bytes
			0x02, 0x66,	
			0x03, 0x66,
			0x04, 0x00,
			0x05, 0x18,
			0x06, 0x00,
			0x07, 0x3C,
			0x08, 0x00,
		}
	};

	pattern pat3_away_2=
	{
		{
			0x01, 0x00, // Pattern bytes
			0x02, 0x00,	
			0x03, 0x66,
			0x04, 0x00,
			0x05, 0x18,
			0x06, 0x00,
			0x07, 0x3C,
			0x08, 0x42,
		}
	};


// LED matrix initialization pattern
uint8_t init [] =
{
		0x0F, 0x00, // Init bytes	
		0x0C, 0x01,
		0x09, 0x00,
		0x0A, 0x01,
		0x0B, 0x07,
		
};


int fd_spi;
int fd5,fd15,fd7,fd24,fd42,fd30,fd25,fd43,fd31,fd44,fd46;  // LED matrix file descriptors
long double dist,diff,prev;
int s=1;
int init_flag=0;

int patt;  // To manage directions of pattern
int smooth1=0,smooth0=0;  // Flags to manage smooth transitions between patterns



// For timer
long double start_time,end_time,t;


// RDTSC
static __inline__ unsigned long long rdtsc(void)
{
  unsigned hi, lo;
  __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
  return ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 );
}

int PWM_On=10;

// Poll function settings
//========================
struct pollfd fdset;
int poll_ret,timeout,nfds;


// End of declarations

// #################################################################################################


//+++++++++++++++++++++++++++++
//
// GPIO setup code
//
//+++++++++++++++++++++++++++++

//=============================================
// Function to setup GPIO pins for Ultrasonic
//=============================================

void IO_Setup(int IO_pin,char * IO_mode)
{
	// Opening gpio/export
    //======================
    fd_export = open("/sys/class/gpio/export", O_WRONLY);
	if(fd_export < 0) 
	{
		printf("gpio export open failed \n");
		return;
	}

	printf("%s\n", IO_mode);


		for(j=0; j<4; j++) // To loop over all required gpio pins for selected IO (row in gpio_ctrl table)
		{
			//printf("Setting up GPIO pin %d for IO%d \n", gpio_ctrl[j][IO_pin], IO_pin);

			if(gpio_ctrl[j][IO_pin] != -1)
			{
				printf("Setting up GPIO pin %d for IO%d \n", gpio_ctrl[j][IO_pin], IO_pin);

				// Writing to /gpio/export
				//=========================
				len = snprintf(buf, sizeof(buf), "%d",gpio_ctrl[j][IO_pin]);
				if(len < 0)
				{
					printf("sprintf error\n");
				}
				else
					write(fd_export,buf,2); 
			 

				// Writing to /gpio/direction
				//=============================
				if(gpio_ctrl[j][IO_pin] < 64)  // GPIO 64-79 can be output only. Hence direction setting not required.
				{

					
					len = snprintf(buf, sizeof(buf),"/sys/class/gpio/gpio%d/direction",gpio_ctrl[j][IO_pin]);
					if(len < 0)
					{
						printf("sprintf error\n");
					}
					else
			        	fd[IO_pin] = open(buf, O_WRONLY);

						
					if (fd[IO_pin] < 0)
					{
						perror("gpio/direction");
					}
					else
						write(fd[IO_pin], IO_mode, 3);
				}

				
				// Writing to /gpio/value
				//========================
				len = snprintf(buf, sizeof(buf),"/sys/class/gpio/gpio%d/value",gpio_ctrl[j][IO_pin]);
				if(len < 0)
				{
				  	printf("sprintf error\n");
				}
				else
					fd[IO_pin] = open(buf, O_WRONLY);

				if (fd[IO_pin] < 0)
				{
					perror("gpio/value");
				}
				else
					write(fd[IO_pin], "0", 1);
				
		    }
		}
}

//=============================================
// Function to setup GPIO pins for LED matrix
//=============================================

// Fixed GPIO pins are used for LED matrix setup as it is connected to SPI pins.
// IO11 -> Data in
// IO12 -> Chip select (Active low)
// IO13 -> SPI clock

void LEDmat_setup()
{
	// Unexport any of the LED GPIO pins if they are already set
	int fd_unexport;
	fd_unexport = open("/sys/class/gpio/unexport", O_WRONLY);
	if (fd_unexport < 0)
	{
		printf("\n gpio unexport open failed");
	}

		if(0 > write(fd_unexport,"5",2))
			printf("error fdunexport 5 \n");
		if(0 > write(fd_unexport,"15",2))
			printf("error fdunexport 15 \n");
		if(0 > write(fd_unexport,"7",2))
			printf("error fdunexport 7 \n");
		if(0 > write(fd_unexport,"24",2))
			printf("error fdunexport 24 \n");
		if(0 > write(fd_unexport,"42",2))
			printf("error fdunexport 42 \n");
		if(0 > write(fd_unexport,"30",2))
			printf("error fdunexport 30 \n");
		if(0 > write(fd_unexport,"25",2))
			printf("error fdunexport 25 \n");
		if(0 > write(fd_unexport,"43",2))
			printf("error fdunexport 43 \n");
		if(0 > write(fd_unexport,"31",2))
			printf("error fdunexport 31 \n");
		if(0 > write(fd_unexport,"44",2))
			printf("error fdunexport 44 \n");
		if(0 > write(fd_unexport,"46",2))
			printf("error fdunexport 46 \n");
		

	// Export required GPIO pins for LED matrix 
	fd_export = open("/sys/class/gpio/export", O_WRONLY);
	if (fd_export < 0)
	{
		printf("\n gpio export open failed");
	}

		if(0 > write(fd_export,"5",2))
			printf("error fdexport 5 \n");
		if(0 > write(fd_export,"15",2))
			printf("error fdexport 15 \n");
		if(0 > write(fd_export,"7",2))
			printf("error fdexport 7 \n");
		if(0 > write(fd_export,"24",2))
			printf("error fdexport 24 \n");
		if(0 > write(fd_export,"42",2))
			printf("error fdexport 42 \n");
		if(0 > write(fd_export,"30",2))
			printf("error fdexport 30 \n");
		if(0 > write(fd_export,"25",2))
			printf("error fdexport 25 \n");
		if(0 > write(fd_export,"43",2))
			printf("error fdexport 43 \n");
		if(0 > write(fd_export,"31",2))
			printf("error fdexport 31 \n");
		if(0 > write(fd_export,"44",2))
			printf("error fdexport 44 \n");
		if(0 > write(fd_export,"46",2))
			printf("error fdexport 46 \n");
		

		// Set direction for LED matrix GPIO pins
		
        fd5 = open("/sys/class/gpio/gpio5/direction", O_WRONLY);
		if (fd5 < 0)
		{
			printf("\n gpio5 direction open failed");
		}
		
		fd15 = open("/sys/class/gpio/gpio15/direction", O_WRONLY);
		if (fd15 < 0)
		{
			printf("\n gpio15 direction open failed");
		}
		fd7 = open("/sys/class/gpio/gpio7/direction", O_WRONLY);
		if (fd7 < 0)
		{
			printf("\n gpio7 direction open failed");
		}
		
		fd24 = open("/sys/class/gpio/gpio24/direction", O_WRONLY);
		if (fd24 < 0)
		{
			printf("\n gpio24 direction open failed");
		}
		fd42 = open("/sys/class/gpio/gpio42/direction", O_WRONLY);
		if (fd42 < 0)
		{
			printf("\n gpio42 direction open failed");
		}
		fd30 = open("/sys/class/gpio/gpio30/direction", O_WRONLY);
		if (fd30 < 0)
		{
			printf("\n gpio24 direction open failed");
		}
		fd25 = open("/sys/class/gpio/gpio25/direction", O_WRONLY);
		if (fd25 < 0)
		{
			printf("\n gpio25 direction open failed");
		}
		fd43 = open("/sys/class/gpio/gpio43/direction", O_WRONLY);
		if (fd43 < 0)
		{
			printf("\n gpio43 direction open failed");
		}
		fd31 = open("/sys/class/gpio/gpio31/direction", O_WRONLY);
		if (fd31 < 0)
		{
			printf("\n gpio31 direction open failed");
		}
		fd44 = open("/sys/class/gpio/gpio44/direction", O_WRONLY);
		if (fd44 < 0)
		{
			printf("\n gpio44 direction open failed");
		}
		fd46 = open("/sys/class/gpio/gpio46/direction", O_WRONLY);
		if (fd46 < 0)
		{
			printf("\n gpio46 direction open failed");
		}
	 
		
		// Make all LED matrix GPIO pins as output
		if(0> write(fd5,"out",3))
			printf("\nerror Fd5");

		if(0> write(fd15,"out",3))
			printf("\nerror Fd15");

		if(0> write(fd7,"out",3))
			printf("\nerror Fd7");

		if(0> write(fd24,"out",3))
			printf("\nerror Fd24");

		if(0> write(fd42,"out",3))
			printf("\nerror Fd42");

		if(0> write(fd30,"out",3))
			printf("\nerror Fd30");

		if(0> write(fd25,"out",3))
			printf("\nerror Fd25");

		if(0> write(fd43,"out",3))
			printf("\nerror Fd43");
		
		if(0> write(fd31,"out",3))
			printf("\nerror Fd31");

		if(0> write(fd44,"out",3))
			printf("\nerror Fd44");

		if(0> write(fd46,"out",3))
			printf("\nerror Fd46");


		// Close all above file descriptors opened for LED matrix GPIO
 		close(fd5);
        close(fd15);
		close(fd7);
		close(fd24);
		close(fd42);
		close(fd30);
		close(fd25);
        close(fd43);
		close(fd31);
		close(fd44);
		close(fd46);
		
              
		// Open /gpio*/value file for each LED matrix GPIO pin
	    fd5 = open("/sys/class/gpio/gpio5/value", O_WRONLY);
		if (fd5 < 0)
				{
					printf("\n gpio5 value open failed");
				}
		fd15 = open("/sys/class/gpio/gpio15/value", O_WRONLY);
                if (fd15 < 0)
				{
					printf("\n gpio15 value open failed");
				}
		fd7 = open("/sys/class/gpio/gpio7/value", O_WRONLY);
		if (fd7 < 0)
				{
					printf("\n gpio7 value open failed");
				}
		fd24 = open("/sys/class/gpio/gpio24/value", O_WRONLY);
                if (fd24 < 0)
				{
					printf("\n gpio24 value open failed");
				}
		fd42 = open("/sys/class/gpio/gpio42/value", O_WRONLY);
        	if (fd42<0)
				{
					printf("\n gpio42 value open failed");
        		}		
		
		fd30 = open("/sys/class/gpio/gpio30/value", O_WRONLY);  		 
		if (fd30< 0)
				{
					printf("\n gpio30 value open failed");       
				}	
		fd25 = open("/sys/class/gpio/gpio25/value", O_WRONLY);
		if (fd25 < 0)
				{
					printf("\n gpio25 value open failed");
				}
		fd43 = open("/sys/class/gpio/gpio43/value", O_WRONLY);
                if (fd43 < 0)
				{
					printf("\n gpio43 value open failed");
				}
		fd31 = open("/sys/class/gpio/gpio31/value", O_WRONLY);
		if (fd31 < 0)
				{
					printf("\n gpio31 value open failed");
				}
		fd44 = open("/sys/class/gpio/gpio44/value", O_WRONLY);
                if (fd44 < 0)
				{
					printf("\n gpio44 value open failed");
				}
		fd46 = open("/sys/class/gpio/gpio46/value", O_WRONLY);
        	if (fd46<0)
				{
					printf("\n gpio46 value open failed");
        			}		
		
	

		if(0> write(fd5,"1",1))
					printf("\nerror Fd5 value");

		if(0> write(fd15,"0",1))
					printf("\nerror Fd15 value");

		if(0> write(fd7,"0",1))
					printf("\nerror Fd7 value");

		if(0> write(fd24,"0",1))
					printf("\nerror Fd24 value");

		if(0> write(fd42,"0",1))
					printf("\nerror Fd42 value");

		if(0> write(fd30,"0",1))
					printf("\nerror Fd30 value");

		if(0> write(fd25,"0",1))
					printf("\nerror Fd25 value");

		if(0> write(fd43,"0",1))
					printf("\nerror Fd43 value");

		if(0> write(fd31,"0",1))
					printf("\nerror Fd31 value");

		if(0> write(fd44,"1",1))
					printf("\nerror Fd44 value");

		if(0> write(fd46,"1",1))
					printf("\nerror Fd46 value");


		close(fd_export);
		close(fd_unexport);

}

// #################################################################################################

//+++++++++++++++++++++++++++++
//
// Ultrasonic sensor code
//
//+++++++++++++++++++++++++++++

//=====================================
// Function to do time calculations
//=====================================

void calc(long double new_t)
{
	
	// Updates time difference globally & records current time for calculatinf difference on next reading
	dist=(new_t)*(7.5/20000);  
	diff=dist-prev;
	prev=dist;

	if(diff > 15)
		{
			smooth1 = smooth1 + 1;			
			if (smooth1 == 1)
			{	
				smooth0 = 0;
			}
			else if (smooth1 == 3)
			{
				patt = 1;
				smooth1 = 0;
			}
			patt = 1;
		
		}

		else if(diff < -15)
		{
				smooth0 = smooth0 + 1;			
			if (smooth0 == 1)
			{	
				smooth1 = 0;
			}						
			else if (smooth0 == 3)
			{
				patt = 0;
				smooth0 = 0;
			}
			patt = 0;
		}
}



//================================================
// Function to write to control Ultrasonic sensor
//================================================

void Ultrasonic_ctrl(int Out_pin, int In_pin)
{
	
	// opens from /gpio/edge & /gpio/value
	// to set & read respectively later
	//====================================
	len = snprintf(buf, sizeof(buf),"/sys/class/gpio/gpio%d/edge",gpio_ctrl[3][In_pin]);
	fdech = open(buf, O_WRONLY);
	
	len = snprintf(buf, sizeof(buf),"/sys/class/gpio/gpio%d/value",gpio_ctrl[3][In_pin]);
	fdechv = open(buf, O_RDWR);

	len = snprintf(buf, sizeof(buf),"/sys/class/gpio/gpio%d/value",gpio_ctrl[3][Out_pin]);
	fdtrig = open(buf, O_WRONLY);

	

	// Poll structure members initialization
	//=======================================
	timeout = 1000;
	nfds = 1;
	fdset.fd = fdechv;
	fdset.events = POLLPRI|POLLERR;
	fdset.revents = 0;


	// Loop to send trigger pulse & read echo from sensor
	while(1)
	{		
		
		// Set trigger pin edge to Rising
		lseek(fdset.fd, 0, SEEK_SET);
		write(fdech,"rising",sizeof("rising"));

		write(fdtrig,"0",1 );
		usleep(1);
			
		// Send trigger pulse of specified width		
		write(fdtrig,"1",1);
		usleep(PWM_On);
		write(fdtrig,"0",1);  

		// Begin polling for the rising edge detection
		poll_ret=poll(&fdset, nfds, timeout);

		if(poll_ret<0)
		{
			printf("Polling error\n");
		}
		if(poll_ret==0)
		{
			printf("Polling timed out\n");
		}
		if(poll_ret>0)   // Poll returned successfully
		{
			if (fdset.revents & POLLPRI)
			{	
				// Start timer for echo pulse width calculation
				start_time=rdtsc(); 			
				read(fdset.fd,&buf_1,1);
				fdset.revents=0;       
			}
		}

		// Set trigger pin edge to Falling
		lseek(fdset.fd, 0, SEEK_SET);
		write(fdech,"falling",sizeof("falling"));
		

		// Begin polling for the falling edge detection
		poll_ret=poll(&fdset, nfds, timeout);

		if(poll_ret<0)
		{
			printf("Polling error\n");
		}
		if(poll_ret==0)
		{
			printf("Polling timed out\n");
		}
					
		if(poll_ret>0)   // Poll returned successfully
		{			
			if (fdset.revents & POLLPRI)
			{		
				// Stop timer for echo pulse width calculation
				end_time=rdtsc();
				read(fdset.fd,&buf_1,1);
				fdset.revents=0;
				
				// Calculate ticks difference from rdtsc() & 
				// call distance calculations function
				t=end_time-start_time;
				calc(t);	
			}	
		}
		usleep(300000);
	// End of polling loop. Repeat new iteration after 60ms 
	}	
}

// End of Ultrasonic sensor code

// #################################################################################################




//+++++++++++++++++++++++++++++
//
// LED matrix code
//
//+++++++++++++++++++++++++++++

//===================================================
// Function to write animation pattern on LED matrix
//===================================================

void LEDPrint(uint8_t *a,int l)
{
	
	int  i = 0;
	int ret;
   	i=0;
   	
	while (i < l)													
	{ 
		Led_write[0] = a [i];
		Led_write[1] = a [i+1];
		
		if(0>write(fd15,"0",1))
			printf("\nerror Fd15 value");

		ret = ioctl(fd_spi, SPI_IOC_MESSAGE (1), &tr);
		if(ret<0)
			printf("\n error");
		
		usleep(1000);
		
		if(0> write(fd15,"1",1))
			printf("\nerror Fd15 value");

		i = i + 2; 	
	}
	
		

}

void LEDmatrix_ctrl()
{
	int x;

	// SPI IOC structure member definition
	
		tr.tx_buf = (unsigned long)Led_write,
		tr.rx_buf = 0,
		tr.len = 2,
		tr.delay_usecs = 1,
		tr.speed_hz = 10000000,
		tr.bits_per_word = 8,
		tr.cs_change = 1,
	

	// Open SPI device file
	fd_spi= open(DEVICE,O_WRONLY);
	if(fd_spi==-1)
	{
     	printf("Device file %s Busy/Not found.\n", DEVICE);
     	exit(-1);
	}
	

	// LED matrix init
	if(init_flag==0)
	{
		x=sizeof(init)/sizeof(uint8_t);
		LEDPrint(init,x);
		init_flag++;
	}		

	// Loop to continuously print patterns on LED matrix
	while(1)
	{ 
		// Object moving towards from Ultrasonic sensor
	    while(patt==0)
	    {
	    	// Sending pattern 1 of selected animation to LED matrix
	    	if(ani_sel==1)
	    	{
				x=sizeof(pat1_towards_1.led)/sizeof(uint8_t);	
		    	LEDPrint(pat1_towards_1.led,x);
		    }
		    if(ani_sel==2)
	    	{
				x=sizeof(pat2_towards_1.led)/sizeof(uint8_t);	
		    	LEDPrint(pat2_towards_1.led,x);
		    }
		    if(ani_sel==3)
	    	{
				x=sizeof(pat3_towards_1.led)/sizeof(uint8_t);	
		    	LEDPrint(pat3_towards_1.led,x);
		    }
			  	

			usleep(750*dist);
	

			// Sending pattern 2 of selected animation to LED matrix
			if(ani_sel==1)
			{
				x=sizeof(pat1_towards_2.led)/sizeof(uint8_t);
				LEDPrint(pat1_towards_2.led,x);
			}
			if(ani_sel==2)
			{
				x=sizeof(pat2_towards_2.led)/sizeof(uint8_t);
				LEDPrint(pat2_towards_2.led,x);
			}
			if(ani_sel==3)
			{
				x=sizeof(pat3_towards_2.led)/sizeof(uint8_t);
				LEDPrint(pat3_towards_2.led,x);
			}
		

			usleep(750*dist);
	    }



		// Object moving away Ultrasonic sensor
	    while(patt==1)
	    {
		    // Sending pattern 3 of selected animation to LED matrix
	    	if(ani_sel==1)
	    	{
		    	x=sizeof(pat1_away_1.led)/sizeof(uint8_t);
			    LEDPrint(pat1_away_1.led,x);
			}
			if(ani_sel==2)
	    	{
		    	x=sizeof(pat2_away_1.led)/sizeof(uint8_t);
			    LEDPrint(pat2_away_1.led,x);
			}
			if(ani_sel==3)
	    	{
		    	x=sizeof(pat3_away_1.led)/sizeof(uint8_t);
			    LEDPrint(pat3_away_1.led,x);
			}
			
			usleep(750*dist);
			
	
			// Sending pattern 4 of selected animation to LED matrix
			if(ani_sel==1)
			{
				x=sizeof(pat1_away_2.led)/sizeof(uint8_t);
				LEDPrint(pat1_away_2.led,x);
			}
			if(ani_sel==2)
			{
				x=sizeof(pat2_away_2.led)/sizeof(uint8_t);
				LEDPrint(pat2_away_2.led,x);
			}
			if(ani_sel==3)
			{
				x=sizeof(pat3_away_2.led)/sizeof(uint8_t);
				LEDPrint(pat3_away_2.led,x);
			}

			usleep(750*dist);
		}		
	}
	close (fd_spi);
//End of LED matrix pattern printing
}



//================================================
// Function to release to IO pins file descritors
//================================================

void IO_Release()
{
	printf("Closing opened file descriptors.\n");

	for(i=0;i<14;i++)
		close(fd[i]);

	close(fd_export);
}



//##################################################
// End of UltraLED header file
//##################################################