#include <linux/sched.h>
#include <linux/kernel.h>
#include <asm/segment.h>
#include <asm/system.h>
#include <errno.h>

#define THREAD_NOUSING 0
#define THREAD_RUNNING 1
#define THREAD_WAITING 2

/*参数none: sys_make_thread返回地址*/
int init_tss(int eax,long ebp,long edi,long esi,long gs,long none,
		long ebx,long ecx,long edx,
		long fs,long es,long ds,
		long eip,long cs,long eflags,long esp,long ss)
{
	int i;
	// printk("\n\tREGS OF CPU\nSS   :%d",ss);
	// printk("\nESP  :%d",esp);
	// printk("\nCS   :%d",cs);
	// printk("\nEIP  :%d",eip);
	// printk("\nEAX  :%d",eax);
	// printk("\nEBX  :%d",ebx);
	// printk("\nECX  :%d",ecx);
	// printk("\nEDX  :%d\n",edx);
	char * p = (char*) get_free_page();
	if (!p)
		return -EAGAIN;
	for(i=0;i<10;i++)
	{
		if(current->thread_state[i] == THREAD_NOUSING)
		{
			current->thread_number += 1;
			current->thread_state[i] = THREAD_RUNNING;
			current->thread_counter[i] = current->priority;
			break;
		}
	}
	if(i > 9)
	{
		panic("Don't try to get more threads than 10!\n");
	}
	// printk("Thread number:%d\n",current->thread_number);
	// printk("Thread in use:%d\n",current->thread_inuse);
	// printk("Thread finded:%d\n",i);
	current->tss[i].back_link = 0;
	current->tss[i].esp0 = PAGE_SIZE + (long) p;
	current->tss[i].ss0 = 0x10;
	current->tss[i].eip = ebx;
	current->tss[i].eflags = eflags;
	current->tss[i].eax = eax;
	current->tss[i].ecx = ecx;
	current->tss[i].edx = edx;
	current->tss[i].ebx = ebx;
	current->tss[i].esp = ecx;
	current->tss[i].ebp = ecx;
	current->tss[i].esi = esi;
	current->tss[i].edi = edi;
	current->tss[i].es = es & 0xffff;
	current->tss[i].cs = cs & 0xffff;
	current->tss[i].ss = ss & 0xffff;
	current->tss[i].ds = ds & 0xffff;
	current->tss[i].fs = fs & 0xffff;
	current->tss[i].gs = gs & 0xffff;
	current->tss[i].ldt = current->tss[0].ldt;
	current->tss[i].trace_bitmap = 0x80000000;
	current->tss[i].i387 = current->tss[0].i387;
	return i;
}

void thread_schedule(struct task_struct *p)
{
	// int i;
	// if(p != NULL)
	// {
	// 	if(p->thread_number == 0) return;
	// 	i = (p->thread_inuse + 1)%10;
	// 	while(1)
	// 	{
	// 		if(p->thread_state[i] == THREAD_RUNNING && p->thread_inuse!= i)
	// 		{
	// 			p->thread_inuse = i;
	// 			printk("Thread schedule result: %d\n",i);
	// 			return i;
	// 		}
	// 		i = (i+1) % 10;
	// 	}
	// }
	int i,next,c;
	if(p != NULL)
	{
		if(p->thread_number == 0) return;
		i = 10;
		while (1) {
			c = -1;
			next = 0;
			i = 10;
			while ((--i)>-1) {
				if(p->thread_state[i] == THREAD_NOUSING)
					continue;
				if(p->thread_state[i] == THREAD_RUNNING && p->thread_counter[i] > c)
				{	
					c = p->thread_counter[i];
					// printk("c=%d\ti=%d\n",c,i);
					next = i;
				}
			}
			// printk("outer c = %d\n",c);
			if (c) break;
			for(i =0 ; i< 10 ; i++)
				if(p->thread_state[i] != 0)
				{
					p->thread_counter[i] = p->priority + (p->thread_counter[i] >> 1);
				}
		}
		p->thread_inuse = next;
		// printk("Thread schedule result: %d\n",next);
	}
}

void sys_thread_cancel(int tid)
{
	cli();
	if(tid==0)
	{
		panic("Main thread can't be canceled!\n");
	}else if(tid < 0 || tid > 9)
	{
		panic("Invaliable thread!\n");
	}else if(current->thread_state[tid] == THREAD_NOUSING)
	{
		printk("BAD BAD: try to cancel useless thread!\n");
	}
	if(current->thread_number == 0)
	{
		printk("BAD BAD: no more thread to cancel!");
	}
	current->thread_state[tid] = THREAD_NOUSING;
	current->thread_number -= 1;
	sti();
	schedule();
}

void sys_thread_exit(int value)
{
	cli();
	/*唤醒所有等待此线程的其他线程*/
	printk("Thread %d exit!\n",current->thread_inuse);
	int i;
	for(i=0;i<10;i++)
	{
		if(current->thread_state[i] == current->thread_inuse*10 + THREAD_WAITING)
			current->thread_state[i] = THREAD_RUNNING;
	}
	// printk("Return value from syscall:%d\n",value);
	current->thread_retval[current->thread_inuse] = value;
	current->thread_state[current->thread_inuse] = THREAD_NOUSING;
	current->thread_number -= 1;
	sti();
	schedule();
}

/*  I'm indeed prond of my design of the following function  
*	保留最初始版本，对比即可知道这样写的好处：充分利用 调度
*/
void sys_thread_join(int tid, int* value_ptr)
{
	// printk("Join thread state:%d\n",current->thread_state[tid]);
	// if(current->thread_state[tid] == 0)
	// {
	// 	printk("Join thread retval:%d\n",current->thread_retval[tid]);
	// 	put_fs_long(current->thread_retval[tid],(unsigned long*)value_ptr);
	// 	return;
	// }else
	// {
	// 	current->thread_state[current->thread_inuse] == 2;
	// 	schedule();
	// }
	if(current->thread_state[tid] == THREAD_RUNNING)
	{
		/*等待状态中加入等待的thread标志*/
		current->thread_state[current->thread_inuse] == tid*10 + THREAD_WAITING;
		schedule();
	}
	// printk("Join thread retval:%d\n",current->thread_retval[tid]);
	put_fs_long(current->thread_retval[tid],(unsigned long*)value_ptr);
}

int sys_thread_status(int tid)
{
	return current->thread_state[current->thread_inuse]%10;
}

int sys_thread_gettid()
{
	return current->thread_inuse;
}