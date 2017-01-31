/*
 * MusicalKeyboardKerne.c
 *
 *  Created on: Apr 23, 2015
 *      Author: jcmx9
 */

#ifndef MODULE
#define MODULE
#endif

#ifndef __KERNEL__
#define __KERNEL__
#endif

#include <linux/module.h>
#include <linux/kernel.h>
#include <asm/io.h>
#include <rtai.h>
#include <rtai_sched.h>
#include <rtai_hal.h>
#include <rtai_fifos.h>

MODULE_LICENSE("GPL");

static RT_TASK musicalKeyboard;
RTIME period;
RTIME music_period[5];

int hrd_irq_num = 59;									//use hardware interrupt Line 59
int sft_irq_num = 63;									//use software interrupt Line 63

//registers pointers
unsigned long *PBDR;
unsigned long *PBDDR;
unsigned long *PFDR;
unsigned long *PFDDR;
unsigned long *GPIOBIntEn;
unsigned long *GPIOBIntType1;
unsigned long *GPIOBIntType2;
unsigned long *GPIOBIntEOI;
unsigned long *GPIOBDB;
unsigned long *IntStsB;
unsigned long *VIC2IntEnable;
unsigned long* VIC2SoftIntClear;

static void hwdHandler(unsigned irq_num, void *cookie){
	rt_disable_irq (irq_num);
	printk ("Hardware Handler is called.\n");
	int button = 3;
	if (*IntStsB & 1 << 0)
		button = 0;
	else if (*IntStsB & 1 << 1)
		button = 1;
	else if (*IntStsB & 1 << 2)
		button = 2;
	else if (*IntStsB & 1 << 3)
		button = 3;
	else if (*IntStsB & 1 << 4)
		button = 4;
	rtf_put (1, &button, sizeof (button));
	rt_task_make_periodic (&musicalKeyboard, rt_get_time(), music_period[button]);
	*GPIOBIntEOI |= 0x1F;				//clear hardware interrupt
	rt_enable_irq (irq_num);
}

static void sftHandler(unsigned irq_num, void *cookie){
	rt_disable_irq (irq_num);
	printk ("Software Handler is called.\n");
	int tone;
	rtf_get (0, &tone, sizeof (tone));
	rt_task_make_periodic (&musicalKeyboard, rt_get_time(), music_period[tone]);
	*VIC2SoftIntClear |= 1 << 31;		//clear software interrupt
	rt_enable_irq (irq_num);
}

static void rt_process(int t){
	while (1){
		*PFDR ^= 1 << 1;
		rt_task_wait_period();
	}
}


int init_module(void){
	//map address
	int i;
	unsigned long *GPOI_base;		//GPIO Control Registers
	unsigned long *VIC2_base;		//Vectored Interrupt Controller 2 Registers
	GPOI_base = (unsigned long *)__ioremap(0x80840000, 4096, 0);		//4*sizeof(unsigned long)
	VIC2_base = (unsigned long *)__ioremap(0x800C0000, 4096, 0);		//4*sizeof(unsigned long)

	PBDR = (unsigned long *)((char *)GPOI_base + 0x04);				//char is 1 byte long
	PBDDR = (unsigned long *)((char *)GPOI_base + 0x14);
	PFDR = (unsigned long *)((char *)GPOI_base + 0x30);
	PFDDR = (unsigned long *)((char *)GPOI_base + 0x34);
	GPIOBIntEn = (unsigned long *)((char *)GPOI_base + 0xB8);		//Interrupt Enable Register B
	GPIOBIntType1 = (unsigned long *)((char *)GPOI_base + 0xAC);	//Interrupt Type 1 Register B
	GPIOBIntType2 = (unsigned long *)((char *)GPOI_base + 0xB0);	//Interrupt Type 2 Register B
	GPIOBIntEOI = (unsigned long *)((char *)GPOI_base + 0xB4);		//End-Of_Interrupt Register B
	GPIOBDB = (unsigned long *)((char *)GPOI_base + 0xC4);
	IntStsB = (unsigned long *)((char *)GPOI_base + 0xBC);			//Interrupt Status Register B
	VIC2IntEnable = (unsigned long *)((char *)VIC2_base + 0x10);	//VIC2 Interrupt Enable Register
	VIC2SoftIntClear = (unsigned long *)((char *)VIC2_base + 0x1C);	//VIC2 Software Interrupt Clear Register

	//modify registers
	*PBDDR = 0xE0;			//set portB 0~4 (five button bits) to input
	*PFDDR |= 1 << 1;		//set the buzzer bit to output, bit masking method
	*PBDR |= 1 << 7;		//indicator LED
	*GPIOBIntEOI |= 0x1F;	//clear interrupt
	*GPIOBIntEn = 0x1F;		//enable interrupt on portB 0~4 (5 buttons)
	*GPIOBIntType1 = 0x1F;	//portB 0~4 edge sensitive
	*GPIOBIntType2 = 0x1F;	//portB 0~4 rising edge sensitive
	*GPIOBDB = 0x1F;		//enable portB 0~4 debouncing
	*VIC2IntEnable |= 1 << 31;//MFS bit is corresponding to 63 interrupt line

	//initiate interrupt
	rt_request_irq (hrd_irq_num, hwdHandler, 0, 1);
	rt_request_irq(sft_irq_num, sftHandler, 0, 1);
	rt_enable_irq (hrd_irq_num);
	rt_enable_irq (sft_irq_num);

	rt_set_periodic_mode();
	period = start_rt_timer(nano2count(200000));			//internal period is 1 ms
	for (i = 0; i < 5; i++)
		music_period[i] = period * (i + 1);
	rt_task_init(&musicalKeyboard, rt_process, 0, 512, 0, 0, 0);
	rt_task_make_periodic (&musicalKeyboard, rt_get_time(), music_period[3]);
	rtf_create(0, sizeof(int)); 	//fifo 0 is used to receive tone set by network
	rtf_create(1, sizeof(int)); 	//fifo 1 is used to send hardware irq to main process and forward to network
	printk ("MusicalKeyBoardKernel MODULE INSTALLED\n");
	return 0;
}

void cleanup_module(void){
	*PBDR &= ~(1 << 7);		//indicator LED
	rt_release_irq (hrd_irq_num);
	rt_release_irq (sft_irq_num);
	rtf_destroy(0);
	rtf_destroy(1);
	rt_task_delete(&musicalKeyboard);
	stop_rt_timer();
	printk ("MusicalKeyBoardKernel MODULE REMOVED\n");
}
