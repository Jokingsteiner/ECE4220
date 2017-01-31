/*
 * TrafficLightKernel2.c
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
#include <rtai_fifos.h>
#include <ece4220lab4.h>
#include <rtai_sem.h>

MODULE_LICENSE("GPL");

unsigned long *ptr_base, *PBDR_PTR, *PBDDR_PTR;
static RT_TASK  tLight1, tLight2, pLight;
RTIME period;
SEM sem;

static void trafficLight1 (int t){				//red light
	while (1){
		rt_sem_wait (&sem);
		//turn on Traffic Light 1
		*PBDR_PTR |= (1 << 5);
		rt_sleep(period * 1000);	//sleep for 1 sec
		*PBDR_PTR &= ~(1 << 5);
		rt_sem_signal (&sem);
	}
}

static void trafficLight2 (int t){				//green light
	while (1){
		rt_sem_wait (&sem);
		//turn on Traffic Light 1
		*PBDR_PTR |= (1 << 7);
		rt_sleep(period * 1000);	//sleep for 1 sec
		*PBDR_PTR &= ~(1 << 7);
		rt_sem_signal (&sem);
	}
}

static void pedestrianLight (int t){			//yellow light
	while (1){
		rt_sem_wait(&sem);
		//check button for pedestrian input
		if (check_button()){
			*PBDR_PTR |= (1 << 6);
			rt_sleep(period * 1000);	//sleep for 1 sec
			*PBDR_PTR &= ~(1 << 6);
			clear_button();
		}
		rt_sem_signal (&sem);
	}
}

int init_module(void){
	ptr_base = (unsigned long *)__ioremap(0x80840000, 4096, 0);
	PBDR_PTR = ptr_base + 1;	//4*sizeof(unsigned long)
	PBDDR_PTR = ptr_base + 5;	//4*sizeof(unsigned long)
	*PBDDR_PTR = 0xE0;
	*PBDR_PTR &= 0x17;	//turn off all lights first

	rt_set_periodic_mode();
	period = start_rt_timer(nano2count(1000000));			//internal period is 1 ms
	//task with priority 0 is the highest
	rt_task_init(&tLight1, trafficLight1, 0, 512, 2, 0, 0);
	rt_task_init(&tLight2, trafficLight2, 0, 512, 0, 0, 0);
	rt_task_init(&pLight, pedestrianLight, 0, 512, 1, 0, 0);
	rt_sem_init (&sem, 1);

	rt_task_resume (&tLight1);
	rt_task_resume (&tLight2);
	rt_task_resume (&pLight);

	printk ("MODULE INSTALLED\n");
	return 0;
}

void cleanup_module(void){
	*PBDR_PTR &= 0x17;	//turn off all lights
	rt_task_delete(&tLight1);
	rt_task_delete(&tLight2);
	rt_task_delete(&pLight);
	stop_rt_timer();
	rt_sem_delete(&sem);
}
