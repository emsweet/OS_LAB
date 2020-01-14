
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
PRIVATE void flush(CONSOLE* p_con);

extern  int isFindState;
//这里是我新加的函数和数据结构，为了实现Tab的后退
int isTab(int pos);//判断当前光标位置是不是Tab键，1 是，0 不是
int TabPos[128];//记录是Tab 位置的光标
int TabPtr=-1;//记录TabPos数组的位置
void pushTab(int pos);//Tab 位置压栈
void popTab();//Tab 位置出栈
void cleanTabStack();// Tab 栈清空   (这个方法按照当前实现是不需要使用的)

//这里是我新加的函数和数据结构，为了实现回车的后退
int isEnter(int pos);//判断当前光标位置是不是回车键，>0 是，0 不是. 返回delta
int EnterPos[128];//记录回车位置的光标
int delta[128];//记录回车位置光标与上一字符光标的差值
int EnterPtr=-1;//记录回车数组的位置
void pushEnter(int pos,int delta);
void popEnter();

//用于记录查找内容的数据结构
int findlen=0;

/*======================================================================*
			   init_screen
 *======================================================================*/
PUBLIC void init_screen(TTY* p_tty)
{
	int nr_tty = p_tty - tty_table;
	p_tty->p_console = console_table + nr_tty;

	int v_mem_size = V_MEM_SIZE >> 1;	/* 显存总大小 (in WORD) */

	int con_v_mem_size                   = v_mem_size / NR_CONSOLES;
	p_tty->p_console->original_addr      = nr_tty * con_v_mem_size;
	p_tty->p_console->v_mem_limit        = con_v_mem_size;
	p_tty->p_console->current_start_addr = p_tty->p_console->original_addr;

	/* 默认光标位置在最开始处 */
	p_tty->p_console->cursor = p_tty->p_console->original_addr;

	if (nr_tty == 0) {
		/* 第一个控制台沿用原来的光标位置 */
		p_tty->p_console->cursor = disp_pos / 2;
		disp_pos = 0;
	}
	else {
		out_char(p_tty->p_console, nr_tty + '0');
		out_char(p_tty->p_console, '#');
	}

	set_cursor(p_tty->p_console->cursor);
}


/*======================================================================*
			   is_current_console
*======================================================================*/
PUBLIC int is_current_console(CONSOLE* p_con)
{
	return (p_con == &console_table[nr_current_console]);
}


/*======================================================================*
			   out_char
 *======================================================================*/
PUBLIC void out_char(CONSOLE* p_con, char ch)
{

	u8* p_vmem = (u8*)(V_MEM_BASE + p_con->cursor * 2);
    int tempBool=0;
    int tempInt1=0;
    int tempInt2=0;
    int tempDelta=0;
	switch(ch) {
	case '\n':
	    tempInt1=p_con->cursor;
		if (p_con->cursor < p_con->original_addr +
		    p_con->v_mem_limit - SCREEN_WIDTH) {
			p_con->cursor = p_con->original_addr + SCREEN_WIDTH * 
				((p_con->cursor - p_con->original_addr) /
				 SCREEN_WIDTH + 1);
		}
		tempInt2=p_con->cursor;
		pushEnter(tempInt2,tempInt2-tempInt1);
		break;
	case '\b':
		if (p_con->cursor > p_con->original_addr) {
			p_con->cursor--;
			*(p_vmem-2) = ' ';
			*(p_vmem-1) = DEFAULT_CHAR_COLOR;
		}

		break;
	case '\t': //实现TAB
		for(int j=0;j<4;j++)
		out_char(p_con,' ');

		pushTab(p_con->cursor);
		break;
	default:
		if (p_con->cursor <
		    p_con->original_addr + p_con->v_mem_limit - 1) {
			*p_vmem++ = ch;
			if(isFindState==0){
			*p_vmem++ = DEFAULT_CHAR_COLOR;
			}else{
                *p_vmem++ =SPECIAL_CHAR_COLOR; 
			}
			p_con->cursor++;
		}
		break;
	}

	while (p_con->cursor >= p_con->current_start_addr + SCREEN_SIZE) {
		scroll_screen(p_con, SCR_DN);
	}

	flush(p_con);
}

PUBLIC void out_char_color(CONSOLE* p_con, char ch,int color)
{

	u8* p_vmem = (u8*)(V_MEM_BASE + p_con->cursor * 2);
	int tempBool=0;
	int tempInt1=0;
	int tempInt2=0;
	int tempDelta=0;
	switch(ch) {
		case '\n':
			tempInt1=p_con->cursor;
			if (p_con->cursor < p_con->original_addr +
								p_con->v_mem_limit - SCREEN_WIDTH) {
				p_con->cursor = p_con->original_addr + SCREEN_WIDTH *
													   ((p_con->cursor - p_con->original_addr) /
														SCREEN_WIDTH + 1);
			}
			tempInt2=p_con->cursor;
			pushEnter(tempInt2,tempInt2-tempInt1);
			break;
		case '\b':

			if(TabPtr!=-1&&isTab(p_con->cursor)){
				tempBool=1;
			}else if(EnterPtr!=-1&&(tempDelta=isEnter(p_con->cursor))>0) {
				tempBool=2;
			}

			if (p_con->cursor > p_con->original_addr) {
				p_con->cursor--;
				*(p_vmem-2) = ' ';
				*(p_vmem-1) = color;
			}

			if(tempBool==1){//若为TAB 在退掉一格的基础上再退三个格
				for(int j=0;j<3;j++)
					out_char(p_con,'\b');
			}else if(tempBool==2){ //若为TAB 在退掉一格的基础上再退delta-1 格
				for(int j=0;j<tempDelta-1;j++)
					out_char(p_con,'\b');
			}
			break;
		case '\t': //实现TAB
			for(int j=0;j<4;j++)
				out_char(p_con,' ');

			pushTab(p_con->cursor);
			break;
		default:
			if (p_con->cursor <
				p_con->original_addr + p_con->v_mem_limit - 1) {
				*p_vmem++ = ch;
				if(isFindState==0){
					*p_vmem++ = color;
				}else{
					*p_vmem++ =color; //查找模式下，使用特殊颜色输出
				}
				p_con->cursor++;
			}
			break;
	}

	while (p_con->cursor >= p_con->current_start_addr + SCREEN_SIZE) {
		scroll_screen(p_con, SCR_DN);
	}

	flush(p_con);
}
/*======================================================================*
                           flush
*======================================================================*/
PRIVATE void flush(CONSOLE* p_con)
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
PUBLIC void select_console(int nr_console)	/* 0 ~ (NR_CONSOLES - 1) */
{
	if ((nr_console < 0) || (nr_console >= NR_CONSOLES)) {
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
PUBLIC void scroll_screen(CONSOLE* p_con, int direction)
{
	if (direction == SCR_UP) {
		if (p_con->current_start_addr > p_con->original_addr) {
			p_con->current_start_addr -= SCREEN_WIDTH;
		}
	}
	else if (direction == SCR_DN) {
		if (p_con->current_start_addr + SCREEN_SIZE <
		    p_con->original_addr + p_con->v_mem_limit) {
			p_con->current_start_addr += SCREEN_WIDTH;
		}
	}
	else{
	}

	set_video_start_addr(p_con->current_start_addr);
	set_cursor(p_con->cursor);
}


//下面的代码和方法是我自己加的，为了实现Tab和回车的整体后退

void pushTab(int pos){
    TabPtr++;
    TabPos[TabPtr]=pos;

}
void popTab(){
    if(TabPtr>=0)
        TabPtr--;
}
void cleanTabStack(){
    TabPtr=-1;
}
int isTab(int pos){
  if(TabPos[TabPtr]==pos){
      popTab();
      return 1;
  }else{
      return 0;
  }
}

void pushEnter(int pos,int delta0){
    EnterPtr++;
    EnterPos[EnterPtr]=pos;
    delta[EnterPtr]=delta0;
}
void popEnter(){
    if(EnterPtr>=0)
        EnterPtr--;
}
int isEnter(int pos){
    int ret=0;
    if(EnterPos[EnterPtr]==pos){
        ret=delta[EnterPtr];
        popEnter();
    }
        return ret;
}

PUBLIC void stop_find(CONSOLE* p_con){
    //恢复字体颜色并输入对应数量的退格
    u8* p_vmem = (u8*)(V_MEM_BASE + p_con->cursor * 2);
    for(int i=0;i<p_con->cursor-p_con->original_addr;i++){
        if(*(p_vmem-2*i-1)==SPECIAL_CHAR_COLOR){
            *(p_vmem-2*i-1)=DEFAULT_CHAR_COLOR;
        }
    }

    for(int i=0;i<findlen;i++)
    out_char(p_con,'\b');
}
PUBLIC void find(CONSOLE* p_con){
    char findsth[128];
int  findptr=0;
	u8* p_vmem = (u8*)(V_MEM_BASE + p_con->cursor * 2);
    for(int i=0;i<p_con->cursor-p_con->original_addr;i++){
        if(*(p_vmem-2*i-1)==SPECIAL_CHAR_COLOR){
            findsth[findptr]=*(p_vmem-2*i-2);
            findptr++;
        }
    }
    findlen=findptr;
    for(int i=0;i<p_con->cursor-p_con->original_addr;i++){
    	int temp=1;
         for(int j=0;j<findptr;j++){
         	if(*(p_vmem-2*j-2*i-2)!=findsth[j]){
         		temp=0;
         		break;
         	}
         }
         if(temp==1){
         	for(int j=0;j<findptr;j++){
         		*(p_vmem-2*j-2*i-1)=SPECIAL_CHAR_COLOR;
         	}
         }
    }

}