/*
 * LEDModule.c
 *
 *  Created on: Feb 5, 2015
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
#include <unistd.h>				//for getpagesize()
#include <asm/io.h>				//for __ioremap

MODULE_LICENSE("GPL");

int init_module(void){
	unsigned long *ptr_base, *PBDR_PTR, *PBDDR_PTR;
	ptr_base = (unsigned long *)__ioremap(0x80840000, 4096, 0);
	PBDR_PTR = ptr_base + 1;	//4*sizeof(unsigned long)
	PBDDR_PTR = ptr_base + 5;	//4*sizeof(unsigned long)
	*PBDDR_PTR = 0xE0;
	*PBDR_PTR = 0x17;
	*(PBDR_PTR) |=  (1 << 5);
	*(PBDR_PTR) |=  (1 << 6);
	printk ("MODULE INSTALLED\n");
	return 0;
}


void cleanup_module(void){
	unsigned long *ptr_base, *PBDR_PTR, *PBDDR_PTR;
	ptr_base = (unsigned long *)__ioremap(0x80840000, 4096, 0);
	PBDR_PTR = ptr_base + 1;	//4*sizeof(unsigned long)
	PBDDR_PTR = ptr_base + 5;	//4*sizeof(unsigned long)
	*PBDDR_PTR = 0xE0;
	*(PBDR_PTR) &=  (0 << 5);
	*(PBDR_PTR) &=  (0 << 6);
	printk ("MODULE REMOVED\n");
}
