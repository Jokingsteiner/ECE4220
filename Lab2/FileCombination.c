/*
 * FileCombination.c
 *
 *  Created on: Feb 19, 2015
 *      Author: jcmx9
 */

#include <stdio.h>
#include <stdlib.h>
#include <rtai.h>
#include <rtai_lxrt.h>
#include <pthread.h>
#include <stdbool.h>
//#include <rtai_sched.h>
//#include <rtai_fifos.h>
//#include <stdlib.h>
//#include <unistd.h>
//#include <fcntl.h>
//#include <sys/mman.h>
//#include <sys/io.h>

//file buffer
char buffer[50];
bool writeFlag = true;

void *readFile1 (void *arg);
void *readFile2 (void *arg);
void *writeFile (void *arg);

int main (){
	//pthread related
	int *status, i;
	pthread_t tds[3];
	pthread_attr_t thread_attr;

	//set pthread attributes
	pthread_attr_init(&thread_attr);
	pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_JOINABLE);

	RTIME BaseP = start_rt_timer(nano2count(1000000));			//count per 1ms

	pthread_create(&tds[0], &thread_attr, readFile1, (void *)&BaseP);
	pthread_create(&tds[1], &thread_attr, readFile2, (void *)&BaseP);
	pthread_create(&tds[2], &thread_attr, writeFile, (void *)&BaseP);

	for (i = 0; i < 3; i++)
		pthread_join(tds[i], (void *) &status);

	stop_rt_timer();
	return 0;
}


void *readFile1 (void *arg){
	FILE *file;
	RTIME *BaseP = (RTIME *)arg;

	if (!(file = fopen ("first.txt", "r")))
		printf ("File 1 Reading Error.\n");

	RT_TASK *rtrf1 = rt_task_init(nam2num("RDF1"), 0, 512, 256);

	//tID = rt_task_init (nam2num ("rdf1"), 0, 512, 256);
	rt_task_make_periodic (rtrf1, rt_get_time(), *BaseP * 100);

	while (fgets(buffer, 50, file)){
		printf ("Reading File 1.\n");
		rt_task_wait_period ();
	}

	fclose (file);
	rt_task_delete(rtrf1);
	pthread_exit(0);
}

void *readFile2 (void *arg){
	FILE *file;
	RTIME *BaseP = (RTIME *)arg;

	if (!(file = fopen ("second.txt", "r")))
		printf ("File 2 Reading Error.\n");

	RT_TASK *rtrf2 = rt_task_init(nam2num("RDF2"), 0, 512, 256);

	rt_task_make_periodic (rtrf2, rt_get_time() + *BaseP * 50, *BaseP * 100);

	while (fgets(buffer, 50, file)){
		printf ("Reading File 2.\n");
		rt_task_wait_period ();
	}

	fclose (file);
	writeFlag = false;
	rt_task_delete(rtrf2);
	pthread_exit(0);
}

void *writeFile (void *arg){
	FILE *file;
	RTIME *BaseP = (RTIME *)arg;

	if (!(file = fopen ("output.txt", "w")))
		printf ("Output File Creating Error.\n");

	RT_TASK *wrtf = rt_task_init(nam2num("WRTF"), 0, 512, 256);

	rt_task_make_periodic (wrtf, rt_get_time() + *BaseP * 5, *BaseP * 50);

	while (writeFlag){
		printf ("%s", buffer);
		fprintf (file, "%s", buffer);
		rt_task_wait_period ();
	}

	fclose (file);
	rt_task_delete(wrtf);
	pthread_exit(0);
}
