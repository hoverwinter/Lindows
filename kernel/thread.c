#include <linux/sched.h>
#include <linux/kernel.h>
#include <asm/segment.h>
#include <asm/system.h>
#include <errno.h>
int get_task_nr(struct task_struct *p)
{
	int i;
	for(i=0 ; i<NR_TASKS ; i++)
			if (task[i] && task[i]->pid == p->pid) return i;
	return -1;
}
/*参数none: sys_make_thread返回地址*/
int init_tss(int eax,long ebp,long edi,long esi,long gs,long none,
		long ebx,long ecx,long edx,
		long fs,long es,long ds,
		long eip,long cs,long eflags,long esp,long ss)
{
	int i,nr;
	printk("eax=%p\n",ebx);
	printk("ebx=%p\n",ebx);
	printk("ecx=%p\n",ecx);
	printk("edx=%p\n",edx);
	printk("eip=%p\n",eip);
	printk("ss=%p\n",ss);
	printk("cs=%p\n",cs);
	printk("esp=%p\n",esp);
	char * p = (char*) get_free_page();
	if (!p)
		return -EAGAIN;
	for(i=0;i<10;i++)
	{
		if(current->thread_state[i] == 0)
		{
			current->thread_number += 1;
			current->thread_state[i] = 1;
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
	current->tss[i].ebp = edx;
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
	// printk("Task struct:%d\n",get_task_nr(current));
	// nr = get_task_nr(current);
	// copy_process(10,edx,edi,esi,gs,none,
	// 	ebx,ecx,edx,
	// 	fs,es,ds,
	// 	ebx,cs,eflags,ecx,ss);
	current->thread_inuse = 1;
	schedule();
	return i;
}

void thread_cancel(int tid)
{
	if(tid==0)
	{
		panic("Main thread can't be canceled!\n");
	}else if(tid < 0 || tid > 9)
	{
		panic("Invaliable thread!\n");
	}else if(current->thread_state[tid] == 0)
	{
		printk("BAD BAD: try to cancel useless thread!\n");
	}
	current->thread_state[tid] = 0;
	current->thread_number -= 1;
	schedule();
}

void thread_exit(long value_ptr)
{
	
}

void thread_join(int tid, void** value_ptr)
{

}