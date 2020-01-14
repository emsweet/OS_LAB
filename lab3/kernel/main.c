


/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            main.c
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
#include "proto.h"
#include "stdio.h"

/*======================================================================*
                            kernel_main
 *======================================================================*/
PUBLIC int kernel_main()
{
    disp_str("-----\"kernel_mainworld\" begins-----\n");
    disp_str("helloworld\n");

    TASK*       p_task      = task_table;
    PROCESS*    p_proc      = proc_table;
    char*       p_task_stack    = task_stack + STACK_SIZE_TOTAL;
    u16     selector_ldt    = SELECTOR_LDT_FIRST;
    int i;
    for (i = 0; i < NR_TASKS; i++) {
        strcpy(p_proc->p_name, p_task->name);   // name of the process
        p_proc->pid = i;            // pid

        p_proc->ldt_sel = selector_ldt;

        memcpy(&p_proc->ldts[0], &gdt[SELECTOR_KERNEL_CS >> 3],
               sizeof(DESCRIPTOR));
        p_proc->ldts[0].attr1 = DA_C | PRIVILEGE_TASK << 5;
        memcpy(&p_proc->ldts[1], &gdt[SELECTOR_KERNEL_DS >> 3],
               sizeof(DESCRIPTOR));
        p_proc->ldts[1].attr1 = DA_DRW | PRIVILEGE_TASK << 5;
        p_proc->regs.cs = ((8 * 0) & SA_RPL_MASK & SA_TI_MASK)
            | SA_TIL | RPL_TASK;
        p_proc->regs.ds = ((8 * 1) & SA_RPL_MASK & SA_TI_MASK)
            | SA_TIL | RPL_TASK;
        p_proc->regs.es = ((8 * 1) & SA_RPL_MASK & SA_TI_MASK)
            | SA_TIL | RPL_TASK;
        p_proc->regs.fs = ((8 * 1) & SA_RPL_MASK & SA_TI_MASK)
            | SA_TIL | RPL_TASK;
        p_proc->regs.ss = ((8 * 1) & SA_RPL_MASK & SA_TI_MASK)
            | SA_TIL | RPL_TASK;
        p_proc->regs.gs = (SELECTOR_KERNEL_GS & SA_RPL_MASK)
            | RPL_TASK;

        p_proc->regs.eip = (u32)p_task->initial_eip;
        p_proc->regs.esp = (u32)p_task_stack;
        p_proc->regs.eflags = 0x1202; /* IF=1, IOPL=1 */

        p_task_stack -= p_task->stacksize;
        p_proc++;
        p_task++;
        selector_ldt += 1 << 3;
    }

    proc_table[0].ticks = proc_table[0].priority = 1;
    proc_table[1].ticks = proc_table[1].priority = 1;
    proc_table[2].ticks = proc_table[2].priority = 1;
    proc_table[3].ticks = proc_table[3].priority = 1;
    proc_table[4].ticks = proc_table[4].priority = 1;
    proc_table[5].ticks = proc_table[5].priority = 1;
    proc_table[6].ticks = proc_table[5].priority = 1;

    initSemaphore();

    k_reenter = 0;
    ticks = 0;

    p_proc_ready    = proc_table;

    init_clock();
    init_keyboard();

    restart();
   
    while(1){}
}

/*======================================================================*
                               普通进程
 *======================================================================*/
void F()
{
    // initSemaphore();
    system_process_sleep(500);
    int start = 1;
    while(1){
        if(start){
            system_disp_str("===== start =====\n");
            start=0;
        }
        else if(readcount!=0){
            cur_read(readcount);
        }else{
            cur_write();
        }
        system_process_sleep(1*TIMESLICE);
    }
}

/*======================================================================*
                               读者进程
 *======================================================================*/

void A(){
    system_process_sleep(500);
    while(1){
        if(type)
            Read2('A');
        else
        {
            Read('A');
        }
        
    }
}

void B(){
    system_process_sleep(500);
    while(1){
         if(type)
           Read2('B');
        else
            Read('B');
    }
}

void C(){
    system_process_sleep(500);
    while(1){
        if(type)
        {
           Read2('C');
        }
        else
          Read('C');
    }
}

/*======================================================================*
                               写者进程
 *======================================================================*/

void D(){
    system_process_sleep(500);
    while(1){
         if(type)
         {
          Write2('D');
         }
        else        
        Write('D');
    }
}
void E(){
    system_process_sleep(500);
    while(1){
        if(type)
        {
                system_process_sleep(520);
          Write2('E');
        }
        else 
          Write('E');
    }
}

