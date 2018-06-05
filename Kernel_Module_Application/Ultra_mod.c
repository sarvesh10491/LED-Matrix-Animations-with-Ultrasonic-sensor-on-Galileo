//################################################################################################
//
// Program     : Ultrasonic sensor control Kernel module
// Header file : Ultra_mod.c
// Authors     : Sarvesh Patil & Nagarjun Chinnari
// Date        : 19 November 2017
//
//################################################################################################


#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/proc_fs.h>
#include <linux/fcntl.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <asm/uaccess.h>
#include <linux/string.h>
#include <linux/cdev.h>
#include <linux/ioctl.h>
#include <linux/version.h>
#include <linux/gpio.h>
#include <linux/jiffies.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/interrupt.h>	
#include <linux/workqueue.h>

// Driver information
//======================
#define  MODULE_NAME   "ultrasonic"
#define  DEVICE_NAME   "ultrasonic"
#define  CLASS_NAME    "ultrasonicClass"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sarvesh Patil & Nagarjun Chinnari");
MODULE_DESCRIPTION("Linux Ultrasonic sensor Driver");
MODULE_VERSION("0.1");


// Character device initializations
//===================================
static dev_t ultrasonic_dev_number;
           
struct ultra_dev 
{
struct cdev ultrasonic_cdev;   
char name[20];                  			
unsigned int write_flag;
unsigned long long int trise;
unsigned long long int tfall;
int  irq;
} *ultra_ptr;



static struct class*  ultrasonicClass  = NULL;
static struct device* ultrasonicDevice = NULL;


static int     ultrasonic_open(struct inode *, struct file *);
static int     ultrasonic_release(struct inode *, struct file *);
static ssize_t ultrasonic_write(struct file *, const char *, size_t, loff_t *);
static ssize_t ultrasonic_read(struct file *,  char *, size_t, loff_t *);

static __inline__ unsigned long long rdtsc(void)
{
  unsigned hi, lo;
  __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
  return ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 );
}


// Variable declaration area
//============================
	
char buf[256];
int i,j;
int irq_pin;
static int edge_type=0; // 0 -> Rising

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


// File operations definition
//=============================
static struct file_operations fops =
{
   .owner = THIS_MODULE,
   .open = ultrasonic_open,
   .write = ultrasonic_write,
   .read=ultrasonic_read,
   .release = ultrasonic_release,
   
};



//##########################
// Driver function area
//##########################


// Open function
int ultrasonic_open(struct inode *inode, struct file *file)
{
  return 0;
}


// Release function
int ultrasonic_release(struct inode *inode, struct file *file)
{
	 return 0;
}

// Write function
ssize_t ultrasonic_write(struct file *file, const char *buf,size_t count, loff_t *ppos)
{
	int retValue = 0;
	
	if(ultra_ptr->write_flag == 1)
	{
		retValue = -EBUSY;
		return -EBUSY;
	}
	
	//Generate a trigger pulse
	gpio_set_value_cansleep(13, 1);
	//printk("pulse rising\n");
	udelay(18);
	gpio_set_value_cansleep(13, 0);
	//printk("pulse falling\n");
	ultra_ptr->write_flag = 1;
	
	return retValue;
}



ssize_t ultrasonic_read(struct file *file,  char *buf, size_t count, loff_t *ppos)
{
	int retValue=0;
	unsigned int c;
	unsigned long long tempBuffer;
	//printk("pulse_read() Start hua hai mc : %d\n", ultra_ptr->write_flag);
	if(ultra_ptr->write_flag == 1)
	{
		//printk("read nahi ho raha chutiye, chala jaa\n");
		return -EBUSY;
	}
	else
	{
		if(ultra_ptr->trise == 0 && ultra_ptr->tfall == 0)
		{
			//printk("Trigger the measure first\n");
		}
		else
		{
			//printk("yesss distance fuck yeah\n");
			tempBuffer = ultra_ptr->tfall - ultra_ptr->trise;
			c = div_u64(tempBuffer,400);
			retValue = copy_to_user((void *)buf, (const void *)&c, sizeof(c));
		}
	}
	//printk("pulse_read() End\n");
	return retValue;
}



static irqreturn_t intr_chg_func(int irq, void *dev_id)
{
	//printk("Change_state_interrupt Start\n");
	if(edge_type==0)
	{
		//printk("Rising edge interrupt\n");
		ultra_ptr->trise = rdtsc();
	    irq_set_irq_type(irq, IRQF_TRIGGER_FALLING);
	    edge_type=1;
	}
	else
	{
		//printk("Falling edge interrupt\n");
		ultra_ptr->tfall = rdtsc();
	    irq_set_irq_type(irq, IRQF_TRIGGER_RISING);
	    edge_type=0;
		ultra_ptr->write_flag = 0;
	}
	//printk("Change_state_interrupt End\n");
	return IRQ_HANDLED;
}

//########################
// Driver module init
//########################
static int __init ultrasonic_init(void)
{		
		int ret,irq_req_rising;
		i=2;

		for(j=0; j<4; j++) // To loop over all required gpio pins for selected IO (row in gpio_ctrl table)
		{

			if(gpio_ctrl[j][i] != -1)
			{
				// Writing to /gpio/export
				//=========================
					gpio_request(gpio_ctrl[j][i], "sysfs");

					gpio_export(gpio_ctrl[j][i], false);            

				// Writing to /gpio/direction
				//=============================
				if(gpio_ctrl[j][i] < 64)  // GPIO 64-79 can be output only. Hence direction setting not required.
				{
					
						gpio_direction_output(gpio_ctrl[j][i], 0);   
								
				}

	        }					
		}

		i=3;

    for(j=0; j<4; j++) // To loop over all required gpio pins for selected IO (row in gpio_ctrl table)
		{

			if(gpio_ctrl[j][i] != -1)
			{
				// Writing to /gpio/export
				//=========================
					gpio_request(gpio_ctrl[j][i], "sysfs");

					gpio_export(gpio_ctrl[j][i], false);            

				// Writing to /gpio/direction
				//=============================
				if(gpio_ctrl[j][i] < 64)  // GPIO 64-79 can be output only. Hence direction setting not required.
				{
					
						gpio_direction_input(gpio_ctrl[j][i]);   
								
				}

	        }					
		}
    
  printk(KERN_INFO "Initializing the ultrasonic sensor LKM\n");  
 
  // Request dynamic allocation of a device major number
  if (alloc_chrdev_region(&ultrasonic_dev_number, 0, 1, DEVICE_NAME) < 0) {
      printk(KERN_DEBUG "Can't register device\n"); return -1;
  }

  // Populate sysfs entries 
  ultrasonicClass = class_create(THIS_MODULE, DEVICE_NAME);


  // Allocate memory for the per-device structure
  ultra_ptr = kmalloc(sizeof(struct ultra_dev), GFP_KERNEL);


  // Request I/O region
  sprintf(ultra_ptr->name, DEVICE_NAME);
	
  
  // Connect the file operations with the cdev 
  cdev_init(&ultra_ptr->ultrasonic_cdev, &fops);
  ultra_ptr->ultrasonic_cdev.owner = THIS_MODULE;

  // Connect the major/minor number to the cdev 
  ret = cdev_add(&ultra_ptr->ultrasonic_cdev, (ultrasonic_dev_number), 1);

  if (ret) {
    printk("Bad cdev\n");
    return ret;
  }

  // Send uevents to udev
  ultrasonicDevice = device_create(ultrasonicClass, NULL, MKDEV(MAJOR(ultrasonic_dev_number), 0), NULL, DEVICE_NAME);   


  irq_pin = gpio_to_irq(14);
	
	if(irq_pin < 0)
	{
		printk("Here Gpio %d cannot be used as interrupt",14);

		ret=-EINVAL;
	}
	ultra_ptr->irq = irq_pin;

	ultra_ptr->trise=0;
	ultra_ptr->tfall=0;
	
	irq_req_rising = request_irq(irq_pin, intr_chg_func, IRQF_TRIGGER_RISING, "intr_chg", ultra_ptr);
	if(irq_req_rising)
	{
		printk("Unable to claim irq %d; error %d\n ", irq_pin, irq_req_rising);
		return 0;
	}

	printk("\n irq request no. %d ",irq_req_rising);	

	printk("\n irq no. %d ",ultra_ptr->irq);


        printk("The pin is mapped to IRQ: %d\n", irq_pin);

 	

  return 0;
}

//########################
// Driver module exit
//########################
static void __exit ultrasonic_exit(void)
{
  cdev_del(&ultra_ptr->ultrasonic_cdev);

  /* Destroy device */
  device_destroy (ultrasonicClass, MKDEV(MAJOR(ultrasonic_dev_number), 0));

  /* Destroy driver_class */
  class_destroy(ultrasonicClass);

  /* Release the major number */
  unregister_chrdev_region((ultrasonic_dev_number), 1);

   kfree(ultra_ptr);
   free_irq(irq_pin,(void *)ultra_ptr);


   gpio_unexport(13);               // Unexport the Button GPIO
   gpio_free(13);                      // Free the LED GPIO
   gpio_unexport(14);               // Unexport the Button GPIO
   gpio_free(14);                      // Free the LED GPIO
   gpio_unexport(34);               // Unexport the Button GPIO
   gpio_free(34);                      // Free the LED GPIO
   gpio_unexport(16);               // Unexport the Button GPIO
   gpio_free(16);                      // Free the LED GPIO
   gpio_unexport(17);               // Unexport the Button GPIO
   gpio_free(17);                      // Free the LED GPIO
   gpio_unexport(37);               // Unexport the Button GPIO
   gpio_free(37);                      // Free the LED GPIO
   gpio_unexport(77);               // Unexport the Button GPIO
   gpio_free(77);                      // Free the LED GPIO
   gpio_unexport(76);               // Unexport the Button GPIO
   gpio_free(76);                      // Free the LED GPIO
   gpio_unexport(64);               // Unexport the Button GPIO
   gpio_free(64);                      // Free the LED GPIO

  printk("Ultrasonic sensor driver removed.\n");
  
}

module_init(ultrasonic_init);
module_exit(ultrasonic_exit);


//==========================
// End of module library
//==========================