// ------------------------------------------------------------------
// --- cmt.c                                                      ---
// --- simple cooperative "on-delay" multitasking                 ---
// ---                                15.dec.2013, Matej Kogovsek ---
// ------------------------------------------------------------------

#include "stm32f10x.h"

#include "cmt.h"

volatile uint8_t cmt_curtask = 0;
volatile struct cmt_task cmt_tasks[CMT_MAXTASKS];
volatile uint8_t cmt_numtasks = 1;

// ------------------------------------------------------------------
// task switching is done here
// this should not be called with interrupts disabled
void cmt_delay_ticks(uint32_t d)
{
	asm("push {r4-r11}\n");	// push all registers not saved by the caller
	
	__disable_irq();
	cmt_tasks[cmt_curtask].sp = __get_MSP();	// remember current task's SP
	cmt_tasks[cmt_curtask].d = d;	// and how long it wishes to sleep
	__enable_irq();
	
	uint8_t i = cmt_curtask;
	while( 1 ) {
		//wdt_reset();
		i =	(i + 1) % cmt_numtasks;
		if( 0 == cmt_tasks[i].d ) { break; }	// found ready to run task
	}

	__disable_irq();
	cmt_curtask = i;
	__set_MSP(cmt_tasks[i].sp);	// restore task's stack pointer
	__enable_irq();
	
	uint32_t tp = cmt_tasks[i].tp;	
	if( tp ) {						// if this is the first time the task
		cmt_tasks[i].tp = 0;		// is run, jump to task proc directly
		asm("bx %0\n"::"r" (tp):);
	}
	
	asm("pop {r4-r11}\n");
}

// ------------------------------------------------------------------
// setup task, call this to setup all tasks
uint8_t cmt_setup_task(void (*task_proc)(void), uint32_t task_sp)
{
	cmt_tasks[0].minsp = -1;	// should be in cmt_init, but can as well be here
	cmt_tasks[0].tp = 0;

	if( cmt_numtasks >= CMT_MAXTASKS ) return 0;
	
	cmt_tasks[cmt_numtasks].sp = task_sp;
	cmt_tasks[cmt_numtasks].tp = 1|(uint32_t)task_proc;
	cmt_tasks[cmt_numtasks].d = 0;	// ready to run
	cmt_tasks[cmt_numtasks].minsp = -1;
	
	return ++cmt_numtasks;
}

// ------------------------------------------------------------------
// should be called by a timer interrupt
void cmt_tick(void)
{
	// keep track of current task's min SP
	if( __get_MSP() < cmt_tasks[cmt_curtask].minsp ) {
		cmt_tasks[cmt_curtask].minsp = __get_MSP();
	}

	// decrease all tasks' delay count
	uint8_t i;
	for( i = 0; i < cmt_numtasks; i++ ) {
		if( cmt_tasks[i].d ) {
			cmt_tasks[i].d--;
		}
	}
}

// ------------------------------------------------------------------
// returns the task's minimal detected stack pointer
uint32_t cmt_minsp(uint8_t task_num)
{
	if( task_num < cmt_numtasks ) {
		return cmt_tasks[task_num].minsp;
	}
	return 0;
}

// ------------------------------------------------------------------
// tries to acquire mutex
uint8_t cmt_try_acquire(struct cmt_mutex* m)
{
	if( (m->ot == cmt_curtask) || (m->ac == 0) ) {
		m->ot = cmt_curtask;
		m->ac++;
		return 1;
	} 
	return 0;
}

// ------------------------------------------------------------------
// waits until mutex acquired
void cmt_acquire(struct cmt_mutex* m)
{
	while( !cmt_try_acquire(m) ) {
		cmt_delay_ticks(0);
	}
}

// ------------------------------------------------------------------
// releases mutex
void cmt_release(struct cmt_mutex* m)
{
	if( (m->ot == cmt_curtask) && (m->ac > 0) ) {
		m->ac--;
	}
}
