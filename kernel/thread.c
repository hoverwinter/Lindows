#include <linux/sched.h>
#include <linux/kernel.h>
#include <asm/segment.h>
#include <asm/system.h>
#include <errno.h>

int init_tss(int eax,long ebp,long edi,long esi,long gs,
		long ebx,long ecx,long edx,
		long fs,long es,long ds,
		long eip,long cs,long eflags,long esp,long ss)
{
	int i;
	char * p = (char*) get_free_page();
	if (!p)
		return -EAGAIN;
	current->thread_number += 1;
	i = current->thread_number;
	if(i > 9)
	{
		panic("Don't try to get more threads than 10!\n");
	}
	printk("%d\n",cs);
	printk("%d\n",current->thread_number);

	current->tss[i].back_link = 0;
	current->tss[i].esp0 = PAGE_SIZE + (long) p;
	current->tss[i].ss0 = 0x10;
	current->tss[i].eip = eip;
	current->tss[i].eflags = eflags;
	current->tss[i].eax = 0;
	current->tss[i].ecx = ecx;
	current->tss[i].edx = edx;
	current->tss[i].ebx = ebx;
	current->tss[i].esp = esp;
	current->tss[i].ebp = ebp;
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
	current->tss[i].i387 = current->tss[i].i387;
}