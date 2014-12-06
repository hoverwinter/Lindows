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
			break;
		}
	}
	if(i > 9)
	{
		panic("Don't try to get more threads than 10!\n");
	}
	printk("Thread number:%d\n",current->thread_number);
	printk("Thread in use:%d\n",current->thread_inuse);
	printk("Thread finded:%d\n",i);
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
	// 			// printk("Thread schedule result: %d\n",i);
	// 			return i;
	// 		}
	// 		i = (i+1) % 10;
	// 	}
	// }
	int i,next,c;
	if(p != NULL)
	{
		if(p->thread_number == 0) return;
		while (1) {
			c = -1;
			next = 0;
			i = 10;
			while (--i) {
				if(p->thread_state[i] == THREAD_NOUSING)
					continue;
				if(p->thread_state[i] == THREAD_RUNNING && p->thread_counter[i] > c)
					c = p->thread_counter[i], next = i;
			}
			if (c) break;
			for(i =0 ; i< 10 ; i++)
				if(p->thread_state[i] != 0)
				{
					p->thread_counter[i] = p->priority + (p->thread_counter[i] >> 1);
				}
		}
	}
}

void sys_thread_cancel(int tid)
{
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
	schedule();
}

void sys_thread_exit(int value)
{
	cli();
	/*唤醒所有等待此线程的其他线程*/
	int i;
	for(i=0;i<10;i++)
	{
		if(current->thread_state[i] == current->thread_inuse*10 + THREAD_WAITING)
			current->thread_state[i] = THREAD_RUNNING;
	}
	// printk("Return value from syscall:%d\n",value);
	current->thread_retval[current->thread_inuse] = value;
	sys_thread_cancel(current->thread_inuse);
	sti();
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

/*
 *  Ok, this is the main fork-routine. It copies the system process
 * information (task[nr]) and sets up the necessary registers. It
 * also copies the data segment in it's entirety.
 */
// int thread_runnable(int nr,long ebp,long edi,long esi,long gs,long none,
// 		long ebx,long ecx,long edx,
// 		long fs,long es,long ds,
// 		long eip,long cs,long eflags,long esp,long ss)
// {
// 	struct task_struct *p;
// 	int i;
// 	struct file *f;

// 	p = (struct task_struct *) get_free_page();
// 	if (!p)
// 		return -EAGAIN;
// 	task[nr] = p;
// 	*p = *current;	/* NOTE! this doesn't copy the supervisor stack */
// 	p->state = TASK_UNINTERRUPTIBLE;
// 	p->pid = current->pid;
// 	/*  线程初始化 */
// 	p->thread_number = current->pid*10 + current->thread_inuse;
// 	p->thread_inuse = 0;
// 	for(i=0;i<10;i++)
// 	{
// 		p->thread_state[i] = 0;
// 		p->thread_retval[i] = 0;
// 	}
// 	p->thread_state[0] = 1;
// 	/*  ******* */
// 	p->father = current->pid;
// 	p->counter = p->priority;
// 	p->signal = 0;
// 	p->alarm = 0;
// 	p->leader = 0;		/* process leadership doesn't inherit */
// 	p->utime = p->stime = 0;
// 	p->cutime = p->cstime = 0;
// 	p->start_time = jiffies;
// 	p->tss[0].back_link = 0;
// 	p->tss[0].esp0 = PAGE_SIZE + (long) p;
// 	p->tss[0].ss0 = 0x10;
// 	p->tss[0].eip = eip;
// 	p->tss[0].eflags = eflags;
// 	p->tss[0].eax = 0;
// 	p->tss[0].ecx = ecx;
// 	p->tss[0].edx = edx;
// 	p->tss[0].ebx = ebx;
// 	p->tss[0].esp = esp;
// 	p->tss[0].ebp = ebp;
// 	p->tss[0].esi = esi;
// 	p->tss[0].edi = edi;
// 	p->tss[0].es = es & 0xffff;
// 	p->tss[0].cs = cs & 0xffff;
// 	p->tss[0].ss = ss & 0xffff;
// 	p->tss[0].ds = ds & 0xffff;
// 	p->tss[0].fs = fs & 0xffff;
// 	p->tss[0].gs = gs & 0xffff;
// 	p->tss[0].ldt = _LDT(nr);
// 	p->tss[0].trace_bitmap = 0x80000000;
// 	if (last_task_used_math == current)
// 		__asm__("clts ; fnsave %0"::"m" (p->tss[0].i387));
// 	if (copy_mem(nr,p)) {
// 		task[nr] = NULL;
// 		free_page((long) p);
// 		return -EAGAIN;
// 	}
// 	for (i=0; i<NR_OPEN;i++)
// 		if ((f=p->filp[i]))
// 			f->f_count++;
// 	if (current->pwd)
// 		current->pwd->i_count++;
// 	if (current->root)
// 		current->root->i_count++;
// 	if (current->executable)
// 		current->executable->i_count++;
// 	set_tss_desc(gdt+(nr<<1)+FIRST_TSS_ENTRY,&(p->tss));
// 	set_ldt_desc(gdt+(nr<<1)+FIRST_LDT_ENTRY,&(p->ldt));
// 	p->state = TASK_RUNNING;	/* do this last, just in case */
// 	return 0;
// }