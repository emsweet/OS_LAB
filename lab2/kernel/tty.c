
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                               tty.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

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

#define TTY_FIRST (tty_table)
#define TTY_END (tty_table + NR_CONSOLES)

PRIVATE void init_tty(TTY *p_tty);
PRIVATE void tty_do_read(TTY *p_tty);
PRIVATE void tty_do_write(TTY *p_tty);
PRIVATE void put_key(TTY *p_tty, u32 key);
PRIVATE void Handle_ctrl_z(TTY *p_tty, u32 key); //new


int findmode; //esc numbers
int LOG_num;  //
u32 key_logs[1000];

/*======================================================================*
                           task_tty
 *======================================================================*/
PUBLIC void task_tty()
{
    TTY *p_tty;

    init_keyboard();

    for (p_tty = TTY_FIRST; p_tty < TTY_END; p_tty++)
    {
        init_tty(p_tty);
    }
    select_console(0);

    findmode = 0;
    LOG_num = 0;
    int count = 0;
    while (1)
    {
        for (p_tty = TTY_FIRST; p_tty < TTY_END; p_tty++)
        {
            tty_do_read(p_tty);
            tty_do_write(p_tty);
            if ((count > 20 * 60000 || count == 0) && findmode == 0)
            {
                for (int j = 0; j < 4000; j++) //(80*25)*2bt
                    out_char(p_tty->p_console, '\b');
                count = 1;
            }
        }
        count++;
    }
}

/*======================================================================*
			   init_tty
 *======================================================================*/
PRIVATE void init_tty(TTY *p_tty)
{
    p_tty->inbuf_count = 0;
    p_tty->p_inbuf_head = p_tty->p_inbuf_tail = p_tty->in_buf;
    init_screen(p_tty);
}
/*======================================================================*
				in_process
 *======================================================================*/
PUBLIC void in_process(TTY *p_tty, u32 key)
{
    char output[2] = {'\0', '\0'};
    if ((key & MASK_RAW) == ESC)    //key=esc?
    {
        if (findmode)
        {
            stop_find(p_tty->p_console);
            findmode = 0;
        }
        else
            findmode = 1;
    }
    if (findmode != 2)
    {
        if (!(key & FLAG_EXT))//flag_ext=Normal function keys
        { 
            if ((key & FLAG_CTRL_L) || (key & FLAG_CTRL_R))
            { 
                int raw_code = key & MASK_RAW;//MASK_RAW = 0x01FF
                if (raw_code == 'z' || raw_code == 'Z') //现在又按下了z或者Z
                    Handle_ctrl_z(p_tty, key);          //处理control + z
            }
            else
            {
                put_key(p_tty, key);
            }
        }
        else
        {                                  
            int raw_code = key & MASK_RAW; 
            switch (raw_code)
            {
			case ENTER:
                if (findmode == 0)//正常输入
                {
                    put_key(p_tty, '\n');
                }
                else
                {
                    findmode = 2;           //屏蔽输入状态
                    find(p_tty->p_console); //调用查找函数
                }
                break;
            case BACKSPACE:
                put_key(p_tty, '\b');
                break;
            case TAB:
                put_key(p_tty, '\t');
                break;
            case UP:
                if ((key & FLAG_SHIFT_L) || (key & FLAG_SHIFT_R))
                {
                    scroll_screen(p_tty->p_console, SCR_DN);
                }
                break;
            case DOWN:
                if ((key & FLAG_SHIFT_L) || (key & FLAG_SHIFT_R))
                {
                    scroll_screen(p_tty->p_console, SCR_UP);
                }
                break;
            case F1:
            case F2:
            case F3:
            case F4:
            case F5:
            case F6:
            case F7:
            case F8:
            case F9:
            case F10:
            case F11:
            case F12:
                /* Alt + F1~F12 */
                if ((key & FLAG_ALT_L) || (key & FLAG_ALT_R))
                {
                    select_console(raw_code - F1);
                }
                break;
            default:
                break;
            }
        }
    }
}

/*======================================================================*
			      put_key
*======================================================================*/
PRIVATE void put_key(TTY *p_tty, u32 key)
{
    if (p_tty->inbuf_count < TTY_IN_BYTES)
    {
        key_logs[LOG_num++] = key;//log key
        *(p_tty->p_inbuf_head) = key;
        p_tty->p_inbuf_head++;
        if (p_tty->p_inbuf_head == p_tty->in_buf + TTY_IN_BYTES)
        {
            p_tty->p_inbuf_head = p_tty->in_buf;
        }
        p_tty->inbuf_count++;
    }
}

/*======================================================================*
			      tty_do_read
 *======================================================================*/
PRIVATE void tty_do_read(TTY *p_tty)
{
    if (is_current_console(p_tty->p_console))
    {
        keyboard_read(p_tty);
    }
}

/*======================================================================*
			      tty_do_write
 *======================================================================*/
PRIVATE void tty_do_write(TTY *p_tty)
{
    if (p_tty->inbuf_count)
    {
        char ch = *(p_tty->p_inbuf_tail);
        p_tty->p_inbuf_tail++;
        if (p_tty->p_inbuf_tail == p_tty->in_buf + TTY_IN_BYTES)
        {
            p_tty->p_inbuf_tail = p_tty->in_buf;
        }
        p_tty->inbuf_count--;
        out_char(p_tty->p_console, ch);
    }
}
//ctrl+z
void Handle_ctrl_z(TTY *p_tty, u32 key)
{
    if (LOG_num >= 1)
    {
        u32 temp_key = key_logs[LOG_num - 1];
        if (temp_key == '\b')
        {
            if (LOG_num >= 2)
            {
                temp_key = key_logs[LOG_num - 2];
                put_key(p_tty, temp_key);
            }
        }
        else
        {
            put_key(p_tty, '\b');
        }
        LOG_num--;
    }
}
