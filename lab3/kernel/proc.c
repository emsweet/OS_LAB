
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                               proc.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include "type.h"
#include "const.h"
#include "protect.h"
#include "tty.h"
#include "console.h"
#include "string.h"
#include "proc.h"
#include "global.h"
#include "proto.h"

/*======================================================================*
                              schedule
 *======================================================================*/
SEMAPHORE rmutex;
SEMAPHORE wmutex;
SEMAPHORE rcnt;
SEMAPHORE x,y,z;
SEMAPHORE *Z;
SEMAPHORE *X;
SEMAPHORE *Y;
SEMAPHORE *sp_rmutex;
SEMAPHORE *sp_wmutex;
SEMAPHORE *sp_s;
SEMAPHORE *sp_rcmutex;

PUBLIC int getColor(char *str)
{
	if (str[0] == 'N' || str[0] == '=')
		return F_COLOR;
	switch (str[6])
	{
	case 'A':
		return A_COLOR;
	case 'B':
		return B_COLOR;
	case 'C':
		return C_COLOR;
	case 'D':
		return D_COLOR;
	case 'E':
		return E_COLOR;
	}
}

PUBLIC void schedule()
{
	PROCESS *p;
	int greatest_ticks = 0;
	for (p = proc_table; p < proc_table + NR_TASKS; p++)
	{
		if (p->sleep > 0)
		{
			p->sleep--; //减掉睡眠的时间
		}
	}
	while (!greatest_ticks)
	{
		for (p = proc_table; p < proc_table + NR_TASKS; p++)
		{
			if (p->wait > 0 || p->sleep > 0)
			{
				continue; //若在等待状态或有睡眠时间，就不分配时间片
			}
			if (p->ticks > greatest_ticks)
			{
				greatest_ticks = p->ticks;
				p_proc_ready = p;
			}
		}

		if (!greatest_ticks)
		{

			for (p = proc_table; p < proc_table + NR_TASKS; p++)
			{
				if (p->wait > 0 || p->sleep > 0)
				{
					continue; //若在等待状态或有睡眠时间，就不分配时间片
				}
				p->ticks = p->priority;
			}
		}
	}
}

/*======================================================================*
                           sys_get_ticks
 *======================================================================*/
PUBLIC int sys_get_ticks()
{

	return ticks;
}

PUBLIC void sys_disp_str(char *str)
{
	TTY *p_tty = tty_table;
	int i = 0;
	while (str[i] != '\0')
	{
		out_char_color(p_tty->p_console, str[i], getColor(str));
		i++;
	}
}

PUBLIC void sys_process_sleep(int k)
{
	p_proc_ready->sleep = k;
	schedule();
}

PUBLIC void sys_P(SEMAPHORE *t)
{

	t->s--;
	if (t->s < 0)
	{
		p_proc_ready->wait = 1;
		t->x[t->ptr] = p_proc_ready;
		t->ptr++; //进入等待进程队列

		schedule();
	}
}

PUBLIC void sys_V(SEMAPHORE *t)
{
	t->s++;
	if (t->s <= 0)
	{
		t->x[0]->wait = 0;
		for (int i = 0; i < t->ptr; i++)
		{
			t->x[i] = t->x[i + 1];
		}
		t->ptr--;
	}
}

PUBLIC void initSemaphore()
{
	rmutex.s = 1;
	wmutex.s = 1;
	x.s=1;
	y.s=1;
	z.s=1;
	rcnt.s = MaxReaderNum;
	Z = &z;
	Y=&y;
	X=&x;
	sp_rcmutex = &rcnt;
	sp_rmutex = &rmutex;
	sp_wmutex = &wmutex;
	readcount = 0;
	writecount = 0;
	TIMESLICE = 1000;
}

PUBLIC void Read2(char id)
{

	system_P(sp_rcmutex);
	//z rmutex x
	system_P(Z);
	system_P(sp_rmutex);
    system_P(X);
	readcount++;
	if (readcount == 1)
		system_P(sp_wmutex);
	RStatusPRINT(1, id);
	// v~~
    system_V(X);
	system_V(sp_rmutex);
    system_V(Z);

	RStatusPRINT(2, id);
	if (id == 'A')
		system_process_sleep(2 * TIMESLICE);
	else
		system_process_sleep(3 * TIMESLICE);

	system_P(X);
	RStatusPRINT(3, id);
	readcount--;
	if (readcount == 0)
	{
		system_V(sp_wmutex);
	}
	system_V(X);
	system_V(sp_rcmutex);

	//增加延迟
	if (id == 'A')
		system_process_sleep(2 * TIMESLICE);
	else
		system_process_sleep(3 * TIMESLICE);
}
PUBLIC void Write2(char id)
{
    system_P(Y);
	writecount++;
	if(writecount==1)
	   system_P(sp_rmutex);
	system_V(Y);
	WStatusPRINT(1, id);

    system_P(sp_wmutex);
	WStatusPRINT(2, id);
	if (id == 'D')
	{ //D消耗3个时间片
		system_process_sleep(3 * TIMESLICE);
	}
	else
	{ //E消耗4个时间片
		system_process_sleep(4 * TIMESLICE);
	}
	system_V(sp_wmutex);

	system_P(Y);
	writecount--;
	if(writecount==0)
	  system_V(sp_rmutex);
	system_V(Y);
	WStatusPRINT(3, id);

	//增加延迟
	if (id == 'D')
	{ //D消耗3个时间片
		system_process_sleep(3 * TIMESLICE);
	}
	else
	{ //E消耗4个时间片
		system_process_sleep(4 * TIMESLICE);
	}
}

PUBLIC void Read(char id)
{

	system_P(sp_rcmutex);
	system_P(sp_rmutex);

	if (readcount == 0)
	{
		system_P(sp_wmutex);
	}
	readcount++;
	RStatusPRINT(1, id);
	system_V(sp_rmutex);
	RStatusPRINT(2, id);
	if (id == 'A')
		system_process_sleep(2 * TIMESLICE);
	else
		system_process_sleep(3 * TIMESLICE);
	system_P(sp_rmutex);
	RStatusPRINT(3, id);
	readcount--;

	if (readcount == 0)
	{
		system_V(sp_wmutex);
	}
	system_V(sp_rmutex);
	system_V(sp_rcmutex);
	//增加延迟
	if (id == 'A')
		system_process_sleep(2 * TIMESLICE);
	else
		system_process_sleep(3 * TIMESLICE);
}

PUBLIC void Write(char id)
{
	system_P(sp_wmutex);
	WStatusPRINT(1, id);
	WStatusPRINT(2, id);
	if (id == 'D')
	{ //D消耗3个时间片
		system_process_sleep(3 * TIMESLICE);
	}
	else
	{ //E消耗4个时间片
		system_process_sleep(4 * TIMESLICE);
	}
	WStatusPRINT(3, id);
	system_V(sp_wmutex);
	//增加延迟
	if (id == 'D')
	{ //D消耗3个时间片
		system_process_sleep(3 * TIMESLICE);
	}
	else
	{ //E消耗4个时间片
		system_process_sleep(4 * TIMESLICE);
	}
}

PUBLIC void RStatusPRINT(int i, char id)
{
	if (i == 1)
	{
		char ch[] = "readeri starts reading.\n";
		ch[6] = id;
		system_disp_str(ch);
	}
	else if (i == 2)
	{
		char ch[] = "readeri is reading.\n";
		ch[6] = id;
		system_disp_str(ch);
	}
	else if (i == 3)
	{
		char ch[] = "readeri ends reading.\n";
		ch[6] = id;
		system_disp_str(ch);
	}
}
PUBLIC void WStatusPRINT(int i, char id)
{
	if (i == 1)
	{
		char ch[] = "writeri starts writing.\n";
		ch[6] = id;
		system_disp_str(ch);
	}
	else if (i == 2)
	{
		char ch[] = "writeri is writing.\n";
		ch[6] = id;
		system_disp_str(ch);
	}
	else if (i == 3)
	{
		char ch[] = "writeri ends writing.\n";
		ch[6] = id;
		system_disp_str(ch);
	}
}

PUBLIC void cur_read(int num)
{
	if (num == 1)
	{
		system_disp_str("Now 1 reader is reading.\n");
	}
	else if (num == 2)
	{
		system_disp_str("Now 2 readers are reading.\n");
	}
	else if (num == 3)
	{
		system_disp_str("Now 3 readers are reading.\n");
	}
}

PUBLIC void cur_write()
{
	system_disp_str("Now 1 writer is writing.\n");
}
