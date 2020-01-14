
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            proto.h
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

/* klib.asm */
PUBLIC void	out_byte(u16 port, u8 value);
PUBLIC u8	in_byte(u16 port);
PUBLIC void	disp_str(char * info);
PUBLIC void	disp_color_str(char * info, int color);

/* protect.c */
PUBLIC void	init_prot();
PUBLIC u32	seg2phys(u16 seg);

/* klib.c */
PUBLIC void	delay(int time);

/* kernel.asm */
void restart();

/* main.c */
void F();
void A();
void B();
void C();
void D();
void E();

/* i8259.c */
PUBLIC void put_irq_handler(int irq, irq_handler handler);
PUBLIC void spurious_irq(int irq);

/* clock.c */
PUBLIC void clock_handler(int irq);
PUBLIC void init_clock();

/* keyboard.c */
PUBLIC void init_keyboard();

/* tty.c */
PUBLIC void task_tty();
PUBLIC void in_process(TTY* p_tty, u32 key);

/* console.c */
PUBLIC void out_char(CONSOLE* p_con, char ch);
PUBLIC void out_char_color(CONSOLE* p_con, char ch,int color);
PUBLIC void scroll_screen(CONSOLE* p_con, int direction);
PUBLIC void stop_find(CONSOLE* p_con);//我新加的函数，停止查找
PUBLIC void find(CONSOLE* p_con);//我新加的函数，查找

/* 以下是系统调用相关 */

/* proc.c */
PUBLIC  int     sys_get_ticks();        /* sys_call */
PUBLIC void  sys_P(SEMAPHORE* t);
PUBLIC void  sys_V(SEMAPHORE* t);
PUBLIC void  sys_disp_str(char* str);
PUBLIC void  sys_process_sleep(int k);
PUBLIC void  initSemaphore();
PUBLIC int getColor(char* str);
PUBLIC void Read(char id);
PUBLIC void Write(char id);
PUBLIC void Read2(char id);
PUBLIC void Write2(char id);
PUBLIC void WStatusPRINT(int i,char id);
PUBLIC void RStatusPRINT(int i,char id);

PUBLIC void cur_read(int num);
PUBLIC void cur_write();


/* syscall.asm */
PUBLIC  void    sys_call();             /* int_handler */
PUBLIC  int     get_ticks();

