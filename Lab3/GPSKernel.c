/*
 * GPSKernel.c
 *
 *  Created on: Mar 5, 2015
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
#include <rtai_fifos.h>
#include <linux/time.h>
#include <stdbool.h>

MODULE_LICENSE("GPL");

static RT_TASK btnDetect;
RTIME period;

static void rt_process(int t){
	unsigned long *ptr_base, *PBDR_PTR, *PBDDR_PTR;
	struct timeval tv;

	ptr_base = (unsigned long *)__ioremap(0x80840000, 4096, 0);
	PBDR_PTR = ptr_base + 1;	//4*sizeof(unsigned long)
	PBDDR_PTR = ptr_base + 5;	//4*sizeof(unsigned long)
	*PBDDR_PTR = 0xE0;
	*(PBDR_PTR) |=  (1 << 5);	//mod indicator
	//*(PBDDR_PTR) &=  ~(1 << 0);
	//*PBDR_PTR = 0x37;

	while(1){
		bool BTN0 = !((*PBDR_PTR) & (1 << 0));
		if (BTN0){
			rt_sleep(35 * period);
			BTN0 = !((*PBDR_PTR) & (1 << 0));
			if (BTN0){
				do_gettimeofday(&tv);
				rtf_put(0, &tv, sizeof(struct timeval));
			}
		}
		rt_task_wait_period();
	}
}

int init_module(void){
	rt_set_periodic_mode();
	period = start_rt_timer(nano2count(1000000));			//internal period is 1 ms
	//btnDetect = rt_task_init(nam2num("BTND"), 0, 512, 256);
	rt_task_init(&btnDetect, rt_process, 0, 512, 0, 0, 0);
	rt_task_make_periodic (&btnDetect, rt_get_time() + 5 * period, period * 75);
	rtf_create (0, sizeof(int));

	printk ("MODULE INSTALLED\n");
	return 0;
}

void cleanup_module(void){
	rt_task_delete(&btnDetect);
	stop_rt_timer();
	rtf_destroy(0);
}
