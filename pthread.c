#define __LIBRARY__
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>

_syscall2(int,make_thread,fn_ptr,func,int,sp);
_syscall2(void,thread_join,int,tid,int*,retval);
_syscall1(void,thread_exit,int,retval);
_syscall1(void,thread_cancel,int,tid);

int pthread_create(pthread_t* tid,fn_ptr start_routine, int arg)
{
	int *p = (int*)malloc(STACK_SIZE*sizeof(int));
	*(p+STACK_SIZE-1) = arg;
	/*   *(p+STACK_SIZE-2): Return Address
	*/
	*tid = make_thread(start_routine,p+STACK_SIZE-2);
	if(*tid > 0 && *tid <10)
		return 0;
	return -1;
}

void pthread_exit(int val)
{
	thread_exit(val);
}

void pthread_cancel(int tid)
{
	thread_cancel(tid);
}

void pthread_join(int tid,int* retval)
{
	thread_join(tid,retval);
}