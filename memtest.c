#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MEM_SIZE 0x40000

int memtest(int times)
{
	int i,j,flag=1,tmp,counter=0;
	unsigned char * p = (unsigned char*) malloc(MEM_SIZE*sizeof(unsigned char));
	printf("memtest times=%d\n",times);
	fflush(stdout);
	for(i=0;i<times;i++)
	{
		for(j=0;j<MEM_SIZE;j++)
		{
			*(p+j) = 0;
			if(*(p+j) != 0) flag = 0;
		
			*(p+j) = 0xFF;
			if(*(p+j) != 0xFF) flag = 0;
	
			*(p+j) = 0x55;
			if(*(p+j) != 0x55) flag = 0;
	
			*(p+j) = 0xAA;
			if(*(p+j) != 0xAA) flag = 0;

			srand(time(NULL));
			tmp = rand() % 0xff;
			*(p+j) = tmp;
			if(*(p+j) != tmp) flag = 0;
			if(flag)
			{
				counter ++;
			}else
			{
				flag = 1;
			}
		}
	}
	pthread_exit(counter);
	return 0;
}

int main()
{
	int times,num,i =0;
	char tmp[10];
	int tid[10];
	int result[10];
	times = num = 1;
	printf("Usage:\n");
	while(1)
	{
		printf(">>>");
		fflush(stdout);
		scanf(" %s",tmp);
		if(strcmp(tmp,"times") == 0)
		{
			scanf(" %d",&times);
			printf("Times:%d\n",times);
			continue;
		}
		if(strcmp(tmp,"thread") == 0)
		{
			scanf(" %d",&num);
			printf("Thread:%d\n",num);
			continue;
		}
		if(strcmp(tmp,"exit") == 0)
		{
			exit(0);
		}
		if(strcmp(tmp,"go") == 0)
		{
			/* for(i=0;i<num;i++)*/
				pthread_create(&tid[i],memtest,times);
			/* for(i=0;i<num;i++)*/
				printf("tid[i]=%d",tid[i]);
				/*pthread_join(tid[i],&result[i]);*/
			for(i=0;i<num;i++)
				printf("Result:%d\n",result[i]);
			continue;
		}
		if(strcmp(tmp,"status") == 0)
		{
			continue;
		}
		if(strcmp(tmp,"abort") == 0)
		{
			continue;
		}
	}
	
}

