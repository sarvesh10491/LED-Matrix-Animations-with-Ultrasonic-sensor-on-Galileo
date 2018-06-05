//################################################################################################
//
// Program     : Led matrix control Kernel module
// Header file : LED_mod.c
// Authors     : Sarvesh Patil & Nagarjun Chinnari
// Date        : 19 November 2017
//
//################################################################################################

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <linux/string.h>
#include <linux/device.h>
#include <linux/jiffies.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/param.h>
#include <linux/list.h>
#include <linux/semaphore.h>
#include <linux/errno.h>
#include <linux/gpio.h>
#include <linux/delay.h>	
#include <linux/workqueue.h>
#include <linux/interrupt.h>
#include <linux/spi/spi.h>
#include <linux/kthread.h>
#include <linux/delay.h>


// Driver information
//======================
#define DRIVER_NAME  "ledmat"
#define DEVICE_NAME  "ledmat"
#define CLASS_NAME	 "ledmatclass"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sarvesh Patil & Nagarjun Chinnari");
MODULE_DESCRIPTION("Linux LED matrix sensor Driver");
MODULE_VERSION("0.1");


#define CONFIG 1
#define MINOR_NUMBER  0
#define MAJOR_NUMBER  154


// SPI data struct
//=================
struct spidev_data 
{
	dev_t  ledmat_devt;
	struct spi_device  *spi;
};


static struct spidev_data *spi_msg;
static struct class *class1;


// Pattern arrays init
//======================
uint8_t LEDmat_arr[12][24];
uint8_t LEDmat_pat[10];

static struct spi_message mes;

uint8_t LEDmat_arrtx[2];

struct spi_transfer tx = 
{
		.tx_buf = LEDmat_arrtx,
		.rx_buf = 0,
		.len = 2,
		.delay_usecs = 1,
		.speed_hz = 10000000,
		.bits_per_word = 8,
		.cs_change = 1,
};
int i;
int j;
int k;
int z;


// Kernel thread function 
//========================
int LEDspi_ktfunc(void *data)
{
	for(i=0;i<10;i=i+2)
	{
	
	z=LEDmat_pat[i];
	j=i+1;
	
	for(k=0;k<24;k=k+2)
	{
		LEDmat_arrtx[0] = LEDmat_arr[z][k];
	    LEDmat_arrtx[1] = LEDmat_arr[z][k+1];

		// Send data over SPI
		spi_message_init(&mes);
		spi_message_add_tail((void *)&tx, &mes);
		gpio_set_value(15,0);
		spi_sync(spi_msg->spi, &mes);
		gpio_set_value(15,1);
	}
	
	msleep(j*20);
	
	}
	
	return 0;	
}

// Function to set the directions and values of GPIO
//=====================================================
int LEDmat_open(struct inode *i, struct file *f)
{

	gpio_direction_output(46,1);
	gpio_direction_output(30,0);
	
	gpio_set_value(72,0);

	gpio_direction_output(24,0);

	gpio_direction_output(44,1);

	gpio_direction_output(42,0);
	
	gpio_direction_output(15,1);
	gpio_set_value(15,0);

	printk("GPIO opening\n");
	return 0;
}

// Function to free all the gpio
//================================
int LEDmat_close(struct inode *i, struct file *f)
{

	gpio_unexport(24);
	gpio_free(24);   
	   
	gpio_unexport(30);
	gpio_free(30);   

	gpio_unexport(42);
	gpio_free(42);   
	   
	gpio_unexport(44);
	gpio_free(44);   

	gpio_unexport(72);
	gpio_free(72);   

	gpio_unexport(15);
	gpio_free(15);  

	printk("\nGPIO closing");

	return 0;
}

// Write function
//================
ssize_t LEDmat_write(struct file *f,const char *m1, size_t count, loff_t *offp)
{
	
	uint8_t LEDmat_seq[10];
	struct task_struct *task;
	copy_from_user((void *)&LEDmat_seq, (void * __user)m1, sizeof(LEDmat_seq));
	for(j=0;j<10;j++)
	{
			LEDmat_pat[j] = LEDmat_seq[j];
	}
	task = kthread_run(&LEDspi_ktfunc, (void *)LEDmat_pat,"kthread_spi_led");
	msleep(1500);

return 0;
}

//IOCTL function
//===============
static long LEDmat_ioctl(struct file *f,unsigned int cmd, unsigned long count)  
{
	uint8_t write[12][24];

	int retValue;
   	 printk("ioctl Start\n");
	retValue = copy_from_user((void *)&write,(void *)count, sizeof(write));
	if(retValue != 0)
	{
		printk("Failure : %d number of bytes that could not be copied.\n",retValue);
	}
	for(i=0;i<12;i++)
	{
	for(j=0;j<24;j++)
	{
			LEDmat_arr[i][j] = write[i][j];
			printk(" %d ",LEDmat_arr[i][j]);
	}
	}
	printk("ioctl End\n");
	return 0;
}


// File operations
//=================
static const struct file_operations spi_operations = 
{
	.owner = THIS_MODULE,
	.open = LEDmat_open,
	.release = LEDmat_close,
	.write	= LEDmat_write,
	.unlocked_ioctl = LEDmat_ioctl,
	
};

// SPI probe function
//=====================
static int spi_probe(struct spi_device *spi)
{
	//struct spidev_data *spidev;
	int status = 0;
	struct device *dev;

	/* Allocate driver data */
	spi_msg = kzalloc(sizeof(*spi_msg), GFP_KERNEL);
	if(!spi_msg)
	{
		return -ENOMEM;
	}

	/// Driver data init
	spi_msg->spi = spi;

	spi_msg->ledmat_devt = MKDEV(MAJOR_NUMBER, MINOR_NUMBER);

    	dev = device_create(class1, &spi->dev, spi_msg->ledmat_devt, spi_msg, DEVICE_NAME);

    if(dev == NULL)
    {
		printk("Device Creation Failed\n");
		kfree(spi_msg);
		return -1;
	}
	printk("SPI LED Driver Probed.\n");
	return status;
}

static int spi_remove(struct spi_device *spi)
{
	device_destroy(class1, spi_msg->ledmat_devt);
	kfree(spi_msg);
	printk("SPI LED Driver Removed.\n");
	return 0;
}

struct spi_device_id leddeviceid[] = {{"spidev",0},{}};

static struct spi_driver spi_led_driver = 
{
    .driver = 
    {
    .name = "spidev",
    .owner = THIS_MODULE,
    },
	.id_table =   leddeviceid,
    .probe =        spi_probe,
    .remove =       spi_remove,
};


//###################################################

// Driver Init function
//=======================
int __init spidriver_init(void)
{
	
	int retValue;
	
	//Register the Device
	retValue = register_chrdev(MAJOR_NUMBER, DEVICE_NAME, &spi_operations);
	if(retValue < 0)
	{
		printk("Device Registration Failed\n");
		return -1;
	}
	
	//Create the class
	class1 = class_create(THIS_MODULE, CLASS_NAME);
	if(class1 == NULL)
	{
		printk("Class Creation Failed\n");
		unregister_chrdev(MAJOR_NUMBER, spi_led_driver.driver.name);
		return -1;
	}
	
	//Register the Driver
	retValue = spi_register_driver(&spi_led_driver);
	if(retValue < 0)
	{
		printk("Driver Registraion Failed\n");
		class_destroy(class1);
		unregister_chrdev(MAJOR_NUMBER, spi_led_driver.driver.name);
		return -1;
	}
	
	printk("SPI LED Driver Initialized.\n");
	return 0;
}


// Driver exit function
void __exit spidriver_exit(void)
{
	spi_unregister_driver(&spi_led_driver);
	class_destroy(class1);
	unregister_chrdev(MAJOR_NUMBER,spi_led_driver.driver.name);
	printk("SPI LED Driver Uninitialized.\n");
}



module_init(spidriver_init);
module_exit(spidriver_exit);
MODULE_LICENSE("GPL v2");

//==========================
// End of module library
//==========================