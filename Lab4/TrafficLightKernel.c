/*
 * TrafficLightKernel.c
 *
 *  Created on: Mar 19, 2015
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
#include <ece4220lab4.h>

MODULE_LICENSE("GPL");

static RT_TASK trafficLight;
RTIME period;


static void rt_process(int t){
	unsigned long *ptr_base, *PBDR_PTR, *PBDDR_PTR;
	ptr_base = (unsigned long *)__ioremap(0x80840000, 4096, 0);
	PBDR_PTR = ptr_base + 1;	//4*sizeof(unsigned long)
	PBDDR_PTR = ptr_base + 5;	//4*sizeof(unsigned long)
	*PBDDR_PTR = 0xE0;
	*PBDR_PTR &= 0x17;

	while (1){
		//turn on Traffic Light 1, red light
		*PBDR_PTR |= (1 << 5);
		rt_sleep(period * 1000);	//sleep for 1 sec
		*PBDR_PTR &= ~(1 << 5);

		//turn on Traffic Light 2, green light
		*PBDR_PTR |= (1 << 7);
		rt_sleep(period * 1000);	//sleep for 1 sec
		*PBDR_PTR &= ~(1 << 7);
		//check button for pedestrian input, yellow light
		if (check_button()){
			*PBDR_PTR |= (1 << 6);
			rt_sleep(period * 1000);	//sleep for 1 sec
			*PBDR_PTR &= ~(1 << 6);
			clear_button();
		}
	}
}

int init_module(void){
	rt_set_periodic_mode();
	period = start_rt_timer(nano2count(1000000));			//internal period is 1 ms
	rt_task_init(&trafficLight, rt_process, 0, 512, 0, 0, 0);
	rt_task_resume (&trafficLight);
	printk ("MODULE INSTALLED\n");
	return 0;
}

void cleanup_module(void){
	rt_task_delete(&trafficLight);
	stop_rt_timer();
}
