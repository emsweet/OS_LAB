
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
			      console.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
						    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

/*
	回车键: 把光标移到第一列
	换行键: 把光标前进到下一行
*/

#include "type.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "global.h"
#include "keyboard.h"
#include "proto.h"

PRIVATE void set_cursor(unsigned int position);
PRIVATE void set_video_start_addr(u32 addr);
PRIVATE void flush(CONSOLE *p_con);

//new
extern int findmode;
int tabstack[200];	
int p_tab = -1;	   //stack top pos
void push_tab(int pos){	tabstack[++p_tab] = pos;}
void pop_tab(){if (p_tab >= 0)		p_tab--;}   
void cleanTabStack(){p_tab = -1;}
//
int enterstack[200];	//enter cursor
int Dist[200];		  //pos of cusor's distance
int p_enter = -1;
int is_tab(int pos){ return (tabstack[p_tab] == pos);}
int is_enter(int pos); 
void push_enter(int pos , int dist){enterstack[++p_enter] = pos;Dist[p_enter] = dist;}
void pop_enter(){if (p_enter >= 0)		p_enter--;}

int findlen = 0;

/*======================================================================*
			   init_screen
 *======================================================================*/
PUBLIC void init_screen(TTY *p_tty)
{
	int nr_tty = p_tty - tty_table;
	p_tty->p_console = console_table + nr_tty;

	int v_mem_size = V_MEM_SIZE >> 1; /* 显存总大小 (in WORD) */

	int con_v_mem_size = v_mem_size / NR_CONSOLES;
	p_tty->p_console->original_addr = nr_tty * con_v_mem_size;
	p_tty->p_console->v_mem_limit = con_v_mem_size;
	p_tty->p_console->current_start_addr = p_tty->p_console->original_addr;

	/* 默认光标位置在最开始处 */
	p_tty->p_console->cursor = p_tty->p_console->original_addr;

	if (nr_tty == 0)
	{
		/* 第一个控制台沿用原来的光标位置 */
		p_tty->p_console->cursor = disp_pos / 2;
		disp_pos = 0;
	}
	else
	{
		out_char(p_tty->p_console, nr_tty + '0');
		out_char(p_tty->p_console, '#');
	}

	set_cursor(p_tty->p_console->cursor);
}

/*======================================================================*
			   is_current_console
*======================================================================*/
PUBLIC int is_current_console(CONSOLE *p_con)
{
	return (p_con == &console_table[nr_current_console]);
}

/*======================================================================*
			   out_char
 *======================================================================*/

PUBLIC void out_char(CONSOLE *p_con, char ch)
{
	u8 *p_vmem = (u8 *)(V_MEM_BASE + p_con->cursor * 2);
	/*modified..*/
	int flag = 0,p1=0,p2=0,dist=0;
	switch (ch)
	{
	case '\n':
		p1 = p_con->cursor; //before enter
		if (p_con->cursor < p_con->original_addr +
								p_con->v_mem_limit - SCREEN_WIDTH)
		{
			p_con->cursor = p_con->original_addr + SCREEN_WIDTH *
													   ((p_con->cursor - p_con->original_addr) /
															SCREEN_WIDTH +
														1);
		}
		p2 = p_con->cursor; //after enter
		push_enter(p2, p2 - p1);
		break;
	case '\t':
		for (int j = 0; j < 4; j++)
		{
			out_char(p_con, ' ');
		}
		push_tab(p_con->cursor);
		break;
	case '\b'://blankspace
		if (p_tab != -1 && is_tab(p_con->cursor))
		{
			pop_tab();
			flag = 1;
		}
		else if (p_enter != -1 && (dist = is_enter(p_con->cursor)) > 0)
			flag = 2;
		// error if deleted
		if (p_con->cursor > p_con->original_addr)
		{
			p_con->cursor--;
			*(p_vmem - 2) = ' ';
			*(p_vmem - 1) = DEFAULT_CHAR_COLOR;
		}
		if (flag == 1)
		{ //tab
				for (int j = 0; j < 3; j++)
				out_char(p_con, '\b');
		}
		else if (flag == 2)
		{ //enter
			for (int j = 0; j < dist-1; j++)
				out_char(p_con, '\b');
		}
		break;
	default:
		if (p_con->cursor <	p_con->original_addr + p_con->v_mem_limit - 1)
		{
			*p_vmem++ = ch;
			if (findmode == 0)
			{
				*p_vmem++ = DEFAULT_CHAR_COLOR;
			}
			else
			{
				*p_vmem++ = SPECIAL_CHAR_COLOR;//esc mode
			}
			p_con->cursor++;
		}
		break;
	}
	while (p_con->cursor >= p_con->current_start_addr + SCREEN_SIZE)
	{
		scroll_screen(p_con, SCR_DN);
	}
	flush(p_con);
}

/*======================================================================*
                           flush
*======================================================================*/
PRIVATE void flush(CONSOLE *p_con)
{
	set_cursor(p_con->cursor);
	set_video_start_addr(p_con->current_start_addr);
}

/*======================================================================*
			    set_cursor
 *======================================================================*/
PRIVATE void set_cursor(unsigned int position)
{
	disable_int();
	out_byte(CRTC_ADDR_REG, CURSOR_H);
	out_byte(CRTC_DATA_REG, (position >> 8) & 0xFF);
	out_byte(CRTC_ADDR_REG, CURSOR_L);
	out_byte(CRTC_DATA_REG, position & 0xFF);
	enable_int();
}

/*======================================================================*
			  set_video_start_addr
 *======================================================================*/
PRIVATE void set_video_start_addr(u32 addr)
{
	disable_int();
	out_byte(CRTC_ADDR_REG, START_ADDR_H);
	out_byte(CRTC_DATA_REG, (addr >> 8) & 0xFF);
	out_byte(CRTC_ADDR_REG, START_ADDR_L);
	out_byte(CRTC_DATA_REG, addr & 0xFF);
	enable_int();
}

/*======================================================================*
			   select_console
 *======================================================================*/
PUBLIC void select_console(int nr_console) /* 0 ~ (NR_CONSOLES - 1) */
{
	if ((nr_console < 0) || (nr_console >= NR_CONSOLES))
	{
		return;
	}

	nr_current_console = nr_console;

	set_cursor(console_table[nr_console].cursor);
	set_video_start_addr(console_table[nr_console].current_start_addr);
}

/*======================================================================*
			   scroll_screen
 *----------------------------------------------------------------------*
 滚屏.
 *----------------------------------------------------------------------*
 direction:
	SCR_UP	: 向上滚屏
	SCR_DN	: 向下滚屏
	其它	: 不做处理
 *======================================================================*/
PUBLIC void scroll_screen(CONSOLE *p_con, int direction)
{
	if (direction == SCR_UP)
	{
		if (p_con->current_start_addr > p_con->original_addr)
		{
			p_con->current_start_addr -= SCREEN_WIDTH;
		}
	}
	else if (direction == SCR_DN)
	{
		if (p_con->current_start_addr + SCREEN_SIZE <
			p_con->original_addr + p_con->v_mem_limit)
		{
			p_con->current_start_addr += SCREEN_WIDTH;
		}
	}
	else
	{
	}
	set_video_start_addr(p_con->current_start_addr);
	set_cursor(p_con->cursor);
}

//new
int is_enter(int pos)
{
	int ret = 0;
	if (enterstack[p_enter] == pos)
	{
		ret = Dist[p_enter];
		pop_enter();
	}
	return ret;
}

PUBLIC void stop_find(CONSOLE *p_con)
{
	u8 *p_vmem = (u8 *)(V_MEM_BASE + p_con->cursor * 2);
	for (int i = 0; i < p_con->cursor - p_con->original_addr; i++)
	{
		if (*(p_vmem - 2 * i - 1) == SPECIAL_CHAR_COLOR)
		{
			*(p_vmem - 2 * i - 1) = DEFAULT_CHAR_COLOR;
		}
	}
	for (int i = 0; i < findlen; i++)
		out_char(p_con, '\b');
}
PUBLIC void find(CONSOLE *p_con)
{
	char find_str[128];
	int findptr = 0;
	u8 *p_vmem = (u8 *)(V_MEM_BASE + p_con->cursor * 2);

	//
	for (int i = 0; i < p_con->cursor - p_con->original_addr; i++)
	{
		if (*(p_vmem - 2 * i - 1) == SPECIAL_CHAR_COLOR)
		{
			find_str[findptr] = *(p_vmem - 2 * i - 2);
			findptr++;
		}
	}
	findlen = findptr;
	for (int i = 0; i < p_con->cursor - p_con->original_addr; i++)
	{
		int temp = 1;
		for (int j = 0; j < findptr; j++)
		{
			if (*(p_vmem - 2 * j - 2 * i - 2) != find_str[j])
			{
				temp = 0;
				break;
			}
		}
		if (temp == 1)
		{
			for (int j = 0; j < findptr; j++)
			{
				*(p_vmem - 2 * j - 2 * i - 1) = SPECIAL_CHAR_COLOR;
			}
		}
	}
}