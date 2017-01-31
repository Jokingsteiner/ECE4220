/*
 * MatrixOpt.c
 *
 *  Created on: Feb 12, 2015
 *      Author: jcmx9
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define MAX_NUM_THREADS 64

typedef struct {
	int row;
	int col;
	float *elements;
} Matrix;

typedef struct {
	int tid;
   	//int starting_element;
    int num_elements;
   	Matrix *A, *B, *C;
} thread_data_t;

//global variables
int add_thread_num, mul_thread_num;

//function prototype
int readMatrix (Matrix *matrixA, Matrix *matrixB, Matrix *matrixC, FILE *file);
void *addMatrix(void *arg);
void *mulMatrix(void *arg);
void printMatrix (Matrix *matrix);

int main (int arg, char *argc[]){
	int i;
	Matrix addMatrixA, addMatrixB, addMatrixC;
	Matrix mulMatrixA, mulMatrixB, mulMatrixC;

	FILE *file;

	//ask for running threads
	if (arg==3){
		add_thread_num = atoi(argc[1]);
		mul_thread_num = atoi(argc[2]);
	}
	else{
		printf("Usage: command [addition threads number] [multiplication threads number].\n");
		exit(1);
	}

	printf ("%d Threads for addition.\n", add_thread_num);
	printf ("%d Threads for multiplication.\n", mul_thread_num);

	//pthread related
	int *status;
	pthread_t add_thread[add_thread_num];
	pthread_t mul_thread[mul_thread_num];
	thread_data_t add_thread_data[add_thread_num];
	thread_data_t mul_thread_data[mul_thread_num];
	pthread_attr_t thread_attr;

	//read matrixes for addition
	file=fopen("addition.txt", "r");
	if (readMatrix (&addMatrixA, &addMatrixB, &addMatrixC, file) != 0){
		return 0;
	}
	fclose(file);

	//read matrixes for multiplication
	file=fopen("multiplication.txt", "r");
	if (readMatrix (&mulMatrixA, &mulMatrixB, &mulMatrixC, file) != 0){
		return 0;
	}
	fclose(file);

	//printf ("MatrixA:\n");
	//printMatrix (&addMatrixA);
	//printf ("MatrixB:\n");
	//printMatrix (&addMatrixB);

	//set pthread attributes
	pthread_attr_init(&thread_attr);
	pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_JOINABLE);

	//set threads data
   	for (i = 0; i < add_thread_num;i++) {
    	add_thread_data[i].tid = 2 * i;
      	add_thread_data[i].A = &addMatrixA;
      	add_thread_data[i].B = &addMatrixB;
      	add_thread_data[i].C = &addMatrixC;
    }
   	for (i = 0; i < mul_thread_num;i++) {
   	 	mul_thread_data[i].tid = 2 * i + 1;
   	 	mul_thread_data[i].A = &mulMatrixA;
   	 	mul_thread_data[i].B = &mulMatrixB;
   	    mul_thread_data[i].C = &mulMatrixC;
   	}

   	//create threads
   	for (i = 0;i < add_thread_num; i++)
   		pthread_create(&add_thread[i], &thread_attr, addMatrix, (void *)&add_thread_data[i]);
   	for (i = 0;i < mul_thread_num; i++)
   	   	pthread_create(&mul_thread[i], &thread_attr, mulMatrix, (void *)&mul_thread_data[i]);

   	//join threads
   	for (i = 0; i < add_thread_num; i++) {
   		pthread_join(add_thread[i], (void *) &status);
   	}
   	for (i = 0; i < mul_thread_num; i++) {
   		pthread_join(mul_thread[i], (void *) &status);
   		}

   	printf ("Addition MatrixC:\n");
   	printMatrix (&addMatrixC);
   	printf ("Multiplication MatrixC:\n");
   	printMatrix (&mulMatrixC);

   	free (addMatrixA.elements);
   	free (addMatrixB.elements);
   	free (addMatrixC.elements);
   	free (mulMatrixA.elements);
   	free (mulMatrixB.elements);
   	free (mulMatrixC.elements);
	return 0;
}

void *addMatrix(void *arg){
	int op_ele, real_ele_loc, real_ele_i, real_ele_j, num_elements;
    thread_data_t *td = (thread_data_t *) arg;

    int tid = td->tid / 2;
    num_elements = td->C->row * td->C->col;

    if (tid < num_elements){
		for (op_ele = 0; op_ele < num_elements; op_ele += mul_thread_num){
			real_ele_loc = tid + op_ele;
			if (real_ele_loc < num_elements){
				real_ele_i = real_ele_loc / td->C->col;
				real_ele_j = real_ele_loc % td->C->col;
			}
			//printf ("Addition Thread #%d is taking over element #%d:[%d][%d]\n", tid, real_ele_loc, real_ele_i, real_ele_j);
			*(td->C->elements + real_ele_i * td->C->col + real_ele_j) = *(td->A->elements + real_ele_i * td->A->col + real_ele_j) + (*(td->B->elements + real_ele_i * td->B->col + real_ele_j));
		}
    }
	pthread_exit(0);
}

void *mulMatrix(void *arg)
{
	int k;
	int op_ele, real_ele_loc, real_ele_i, real_ele_j, num_elements;
    thread_data_t *td = (thread_data_t *) arg;

    int tid = (td->tid - 1)/2;
    num_elements = td->C->row * td->C->col;

    if (tid < num_elements){
		for (op_ele = 0; op_ele < num_elements; op_ele += mul_thread_num){
			real_ele_loc = tid + op_ele;
			if (real_ele_loc < num_elements){
				real_ele_i = real_ele_loc / td->C->col;
				real_ele_j = real_ele_loc % td->C->col;
			}
			printf ("Multiplication Thread #%d is taking over element #%d:[%d][%d]\n", tid, real_ele_loc, real_ele_i, real_ele_j);
			for (k = 0; k < td->A->col; ++k){
				*(td->C->elements + real_ele_i * td->C->col + real_ele_j) += *(td->A->elements + real_ele_i * td->A->col + k) * (*(td->B->elements + k * td->B->col + real_ele_j));
			}
		}
    }
	pthread_exit(0);
}

int readMatrix (Matrix *matrixA, Matrix *matrixB, Matrix *matrixC, FILE *file){
	int i, j;

	//read row and col of matrixA
	if (!fscanf (file, "%d", &(matrixA->row))){
		printf ("Error: Read matrixA row failed.\n");
		return 1;
	}
	if (!fscanf (file, "%d", &(matrixA->col))){
		printf ("Error: Read matrixA col failed.\n");
		return 1;
	}
	matrixA->elements = malloc (sizeof(float) * matrixA->row * matrixA->col);

	//read elements in matrixA
	for(i = 0; i < matrixA->row; i++){
		for (j = 0; j < matrixA->col; j++)
			if (!fscanf (file, "%f", matrixA->elements + matrixA->row * i + j)){
				printf ("Error: Read matrixA failed.\n");
				return 1;
			}
	}

	printf ("MatrixA is a %d*%d matrix.\n", matrixA->row, matrixA->col);

	//read row and col of matrixB
	if (!fscanf (file, "%d", &(matrixB->row))){
		printf ("Error: Read matrixB row failed.\n");
		return 1;
	}
	if (!fscanf (file, "%d", &(matrixB->col))){
		printf ("Error: Read matrixB col failed.\n");
		return 1;
	}
	matrixB->elements = malloc (sizeof(float) * matrixB->row * matrixB->col);

	//read elements in matrixB
	for(i = 0; i < matrixB->row; i++){
		for (j = 0; j < matrixB->col; j++)
			if (!fscanf (file, "%f", matrixB->elements + matrixB->row * i + j)){
				printf ("Error: Read matrixB failed.\n");
				return 1;
			}
	}

	printf ("MatrixB is a %d*%d matrix.\n", matrixB->row, matrixB->col);

	if ((matrixA->row == matrixB->row) && (matrixA->col == matrixB->col)){
		matrixC->row = matrixA->row;
		matrixC->col = matrixA->col;
		matrixC->elements = malloc (sizeof(float) * matrixA->row * matrixA->col);
		for (i = 0; i < matrixC->row; i++)
			for (j = 0; j < matrixC->col; j++)
				*matrixC->elements = 0;
		printf ("Matrixes dimension match. Ready for addition or multiplication.\n");
	}
	else if (matrixA->col == matrixB->row){
		matrixC->row = matrixA->row;
		matrixC->col = matrixB->col;
		matrixC->elements = malloc (sizeof(float) * matrixA->row * matrixB->col);
		for (i = 0; i < matrixC->row; i++)
			for (j = 0; j < matrixC->col; j++)
				*matrixC->elements = 0;
		printf ("Matrixes are ready for multiplication.\n\n");
	}
	else{
		printf ("These two matrixes are not available for further calculation.\n\n");
		return -1;
	}

	return 0;
}

void printMatrix (Matrix *matrix){
	int i, j;
	for(i = 0; i < matrix->row; i++){
		for (j = 0; j < matrix->col; j++){
			printf ("%.0f", *(matrix->elements + matrix->row * i + j));
			if (j != matrix->col - 1)
				printf (" ");
		}
		printf ("\n");
	}
}
