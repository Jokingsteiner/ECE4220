/*
 * GPSsimulator.c
 *
 *  Created on: Feb 27, 2015
 *      Author: jcmx9
 */


#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "serial_ece4220.h"
#include <stdbool.h>
#include <semaphore.h>

struct timeval ts_local, ts_rt;
bool flag[4] = {true, true, true, true};
sem_t sem;

typedef struct{
	int tid;
	unsigned char position;
	struct timeval time;
} thread_data_t;

typedef struct{ 											//for printData()
	//float partial_time;
	//float time_interval;
	float gpsValue;
	unsigned int prevPos;
	unsigned int postPos;
	//unsigned long rtTime;
	//unsigned long prevTime;
	//unsigned long postTime;
	struct timeval rtTime;
	struct timeval prevTime;
	struct timeval postTime;
}print_data_t;

unsigned int ReadBuffer;
int fifo_in;

void *readKernel(void *arg);
void *calThread (void *arg);
void *printThread (void *arg);
void timeConvert (struct timeval *time, char* timestr);

int main (){
	int i;
	unsigned char x;	//received data

	int port_id = serial_open (0, 5, 5);
	fifo_in = open("/dev/rtf/0", O_RDWR);
	system("mkfifo named_pipe");
	sem_init(&sem, 0, 1);

	//pthread related
	int *status;
	pthread_t kernel_thread;
	pthread_t print_thread;
	//thread_data_t
	pthread_attr_t thread_attr;

	//set pthread attributes
	pthread_attr_init(&thread_attr);
	pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_JOINABLE);

	pthread_create(&kernel_thread, &thread_attr, readKernel, NULL);
	pthread_create(&print_thread, &thread_attr, printThread, NULL);

	while(1){
		if (read(port_id, &x, 1) != 1)
			printf ("Error: Read port failed.\n");

		ReadBuffer = (unsigned int)x;
		gettimeofday(&ts_local, NULL);
		fflush(stdout);
		for (i = 0; i < 4; i++)
			flag[i] = false;
		printf ("%d\n", ReadBuffer);
	}

	pthread_join (kernel_thread, (void *) &status);
	pthread_join (print_thread, (void *) &status);
	close(fifo_in);
	sem_destroy(&sem);

	return 0;
}

void *readKernel(void *arg){
	int i = 0;
	pthread_t td[4];
	pthread_attr_t thread_attr;
	thread_data_t td_t[4];

	pthread_attr_init(&thread_attr);
	pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_JOINABLE);

	while (1){
		read(fifo_in, &ts_rt, sizeof(ts_rt));
		td_t[i%4].tid = i%4;
		td_t[i%4].time = ts_rt;
		pthread_create(&td[i%4], &thread_attr, calThread, (void *)(&td_t[i%4]));
		i++;
	}
	pthread_exit(0);
}

void *calThread (void *arg){
	int namedPipe;
	thread_data_t td = *((thread_data_t*)arg);
	float timeInterval, diffTime, gpsValue;
	struct timeval prevTime, postTime;
	unsigned char prevPos, postPos;
	print_data_t data;

	//time and GPS before time stamp
	prevTime = ts_local;
	data.prevTime = prevTime;
	prevPos = ReadBuffer;
	data.prevPos = prevPos;

	//wait till next GPS arrive
	flag[td.tid] = true;
	while (flag[td.tid]);
	flag[td.tid] = true;
	//time and GPS after time stamp
	postTime = ts_local;
	data.postTime = postTime;
	postPos = ReadBuffer;
	data.postPos = postPos;

	data.rtTime = td.time;

	//calculate right gpsValue for the time stamp
	diffTime = abs(td.time.tv_sec * 1000000 + td.time.tv_usec - prevTime.tv_sec * 1000000 - prevTime.tv_usec);
	timeInterval = abs(postTime.tv_sec * 1000000 + postTime.tv_usec - prevTime.tv_sec * 1000000 - prevTime.tv_usec);
	gpsValue = (postPos - prevPos) * diffTime / timeInterval + prevPos;
	data.gpsValue = gpsValue;

	if ((namedPipe = open("named_pipe", O_WRONLY)) < 0)
		printf("calThread failed to open named pipe\n");

	sem_wait(&sem);
	if (write(namedPipe, &data, sizeof(data)) != sizeof(print_data_t))
		printf("calThread failed to write into named pipe");
	sem_post(&sem);

	pthread_exit(0);
}

void *printThread (void *arg){
	int namedPipe;
	char prevTimestr[9], rtTimestr[9], postTimestr[9];
	print_data_t data;

	if ((namedPipe = open("named_pipe", O_RDONLY)) < 0)
			printf("Error: Cannot open named pipe.\n");

	while (1){
		if (read(namedPipe, &data, sizeof(data)) == sizeof(print_data_t)){
			timeConvert (&data.prevTime, prevTimestr);
			timeConvert (&data.rtTime, rtTimestr);
			timeConvert (&data.postTime, postTimestr);

			printf ("The previous GPS location is: %u @ time %s.%.6ld.\n", data.prevPos, prevTimestr, data.prevTime.tv_usec);
			printf ("The GPS location is: %.2f @ time %s.%.6ld.\n", data.gpsValue, rtTimestr, data.rtTime.tv_usec);
			printf ("The post GPS location is: %u @ time %s.%.6ld.\n", data.postPos, postTimestr, data.postTime.tv_usec);
			printf ("\n");
		}
	}

	pthread_exit(0);
}

void timeConvert (struct timeval *time, char* timestr){
	int i;
	struct tm *tmp;
	char timestr_temp[9];
	tmp = localtime(&(time->tv_sec));

	if (tmp->tm_hour == 23)
		tmp->tm_hour = 0;
	else
		tmp->tm_hour ++;

	strftime(timestr_temp, sizeof(timestr_temp), "%H:%M:%S", tmp);

	for (i = 0; i < 10; i++)
		if (i == 10)
			timestr[i] = '\0';
		else
			timestr[i] = timestr_temp[i];
}
