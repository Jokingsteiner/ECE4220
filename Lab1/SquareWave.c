/*
 * SquareWave.c
 *
 *  Created on: Feb 2, 2015
 *      Author: jcmx9
 */

#include <stdio.h>
#include <err.h>	//for open() error test
#include <fcntl.h>	//for open() flags
#include <sys/mman.h>	//for MMAP flags
#include <unistd.h>	//for getpagesize()
#include <sys/mman.h>	//for msync()
#include <stdbool.h>

int main (){
	//open files and check
	int i, j;
	int fd = -1;
	int select;
	if ((fd = open("/dev/mem", O_RDWR | O_SYNC, 0)) == -1)
		err(1, "open");
	unsigned long *ptr_base;
	unsigned long *PBDR_PTR;
	unsigned long *PBDDR_PTR;
	unsigned long *PFDR_PTR;
	unsigned long *PFDDR_PTR;

	bool BTN0, BTN1, BTN2, BTN3, BTN4;

	//set ptrs
	ptr_base = (unsigned long*)mmap(0, getpagesize(), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x80840000);
	PBDR_PTR = ptr_base + 1;	//4*sizeof(unsigned long)
	PBDDR_PTR = ptr_base + 5;	//4*sizeof(unsigned long)
	PFDR_PTR = ptr_base + 12;	//4*sizeof(unsigned long)
	PFDDR_PTR = ptr_base + 13;	//4*sizeof(unsigned long)

	//set direction register
	*PBDDR_PTR = 0xE0;
	*PFDDR_PTR = 0x02;
	printf ("%p\n", ptr_base);
	*(PBDR_PTR) |=  (1 << 5);
	*(PBDR_PTR) |=  (1 << 6);

	while (1){
		printf ("Please input which button you wanna push.\n");
		scanf ("%d", &select);
		while (1){
			if (select < 0 || select > 4){
				printf ("Please input button number 0~4.\n");
				break;
			}

			BTN0 = !((*PBDR_PTR) & (1 << 0));
			BTN1 = !((*PBDR_PTR) & (1 << 1));
			BTN2 = !((*PBDR_PTR) & (1 << 2));
			BTN3 = !((*PBDR_PTR) & (1 << 3));
			BTN4 = !((*PBDR_PTR) & (1 << 4));
			//msync(PFDR_PTR, getpagesize(), MS_SYNC);
			//printf ("%d\n", BTN0);
			if (BTN0 && (select == 0)){
				printf ("Button 0 is pushed down.\n");
				while(1){
						*(PFDR_PTR) ^= 1 << 1;
						msync(PFDR_PTR, getpagesize(), MS_SYNC);
						usleep (200);		//works fine with msync
						//but the frequency doesn't change much when parameter changes
						
						//for loop is another way to make pause between toggling
						//for (i = 0 ; i<600; i++){
						//	for (j = 0 ; j<i; j++){
						//	}
						//}
						BTN0 = !((*PBDR_PTR) & (1 << 0));
						if (!BTN0)
							break;
					}
				break;
			}
			if (BTN1 && (select == 1)){
				printf ("Button 1 is pushed down.\n");
				while(1){
						*(PFDR_PTR) ^= 1 << 1;
						for (i = 0 ; i<500; i++){
							for (j = 0 ; j<i; j++){
							}
						}
						BTN1 = !((*PBDR_PTR) & (1 << 1));
						if (!BTN1)
							break;
					}
				break;
			}
			if (BTN2 && (select == 2)){
				printf ("Button 2 is pushed down.\n");
				while(1){
						*(PFDR_PTR) ^= 1 << 1;
						for (i = 0 ; i<400; i++){
							for (j = 0 ; j<i; j++){
							}
						}
						BTN2 = !((*PBDR_PTR) & (1 << 2));
						if (!BTN2)
							break;
					}
				break;
			}
			if (BTN3 && (select == 3)){
				printf ("Button 3 is pushed down.\n");
				while(1){
						*(PFDR_PTR) ^= 1 << 1;
						for (i = 0 ; i<300; i++){
							for (j = 0 ; j<i; j++){
							}
						}
						BTN3 = !((*PBDR_PTR) & (1 << 3));
						if (!BTN3)
							break;
					}
				break;
			}
			if (BTN4 && (select == 4)){
				printf ("Button 4 is pushed down.\n");
				while(1){
						*(PFDR_PTR) ^= 1 << 1;
						for (i = 0 ; i<200; i++){
							for (j = 0 ; j<i; j++){
							}
						}
						BTN4 = !((*PBDR_PTR) & (1 << 4));
						if (!BTN4)
							break;
					}
				break;
			}

		}
	}

	return 0;
}
