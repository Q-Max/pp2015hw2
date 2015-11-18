#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <pthread.h>

void *passenger(void* arg);
pthread_mutex_t mutex;
pthread_cond_t cond;
int np, n, C, T, N, headOfArray, count;
long long ln;
int *array;
int *line;
int main(int argc, char** argv){
	if(argc<5){
		puts("execution error, ./hw2_SRCC n C T N");
		exit(0);
	}
	struct timeval tvalBefore, tvalAfter;
	int i, j;
	count = 0;
	n = atoi(argv[1]);
	C = atoi(argv[2]);
	T = atoi(argv[3]);
	N = atoi(argv[4]);
	if(n<C){
		puts("n<C");
		exit(0);
	}
	array = (int*)malloc(sizeof(int)*n);
	line = (int*)calloc(0, sizeof(int)*n);
	pthread_t threads[n];
	pthread_mutex_init(&mutex, NULL);
	pthread_cond_init(&cond, NULL);
	np = 0;
	ln = 0;
	headOfArray = 0;
	gettimeofday (&tvalBefore, NULL);
	for(i=0;i<n;i++){
		pthread_create(&threads[i], NULL, passenger, (void *) i);
	}
	while(count<N){
		while(1){
			//pthread_mutex_lock(&mutex);
			gettimeofday (&tvalAfter, NULL);
			printf("car departures at %ld millisec. ", (tvalAfter.tv_usec-tvalBefore.tv_usec)/1000+1000*(tvalAfter.tv_sec-tvalBefore.tv_sec));
			if(ln - headOfArray >= C){
				for(i=headOfArray%n,j=0;j<C;j++,i++){
					printf("%d ",array[i%n]);
					//line[array[i%n]]=0;
				}
				printf("passengers are in the car\n");
				usleep(T*1000);
				gettimeofday (&tvalAfter, NULL);
				printf("car arrives at %ld millisec. ", (tvalAfter.tv_usec-tvalBefore.tv_usec)/1000+1000*(tvalAfter.tv_sec-tvalBefore.tv_sec));
				pthread_mutex_lock(&mutex);
				for(i=headOfArray%n,j=0;j<C;j++,i++){
					printf("%d ",array[i%n]);
					line[array[i%n]]=0;
				}
				printf("passengers get off\n");
				//pthread_cond_broadcast(&cond);
				headOfArray+=C;
				pthread_mutex_unlock(&mutex);
				break;
			}
			else
				usleep(100);
			//pthread_mutex_unlock(&mutex);
		}
		// control
		count++;
	}
	return 0;
}

void *passenger(void* arg){
	int id = (int) arg;
	char wonder[30];
	char returnline[30];
	char temp[30];
	/*if(id == 0)
		strcpy(temp, "1st");
	else if(id == 1)
		strcpy(temp, "2sec");
	else if(id == 2)
		strcpy(temp, "3rd");
	else
		sprintf(temp, "%dth", id+1);*/
	/*sprintf(temp, "%d", id);
	strcpy(wonder, temp);
	strcpy(returnline, temp);
	strcat(wonder," passenger wanders around the park.");
	strcat(returnline, " passenger returns for another ride.");
	printf("%s %s\n",wonder,returnline);*/
	while(count<N){
		pthread_mutex_lock(&mutex);
			array[ln%n] = id;
			line[id]=1;
			//printf("line id : %d %d\n",line[id],id);
			np++;
			ln++;
			/*pthread_cond_wait(&cond, &mutex);
			while(line[id]==1)
				pthread_cond_wait(&cond, &mutex);
		pthread_mutex_unlock(&mutex);*/
		pthread_mutex_unlock(&mutex);
		while(line[id]==1)
			usleep(100);
		printf("%d passenger wanders around the park.\n",id);
		usleep((rand()%100+1));
		printf("%d passenger returns for another ride.\n",id);
	}
}
