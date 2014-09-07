/**

Simple cooperative "on-delay" multitasking is achieved with 3 simple steps:
1. Add tasks to the switching system with cmt_setup_task.
2. Use a timer interrupt to call cmt_tick periodically.
3. Within a running task, call cmt_delay_ticks to switch to another task and return after specified ticks.

The logic of this system is to use the CPU cycles you would normally waste in a delay loop to perform other tasks.
The prerequisite for expected behaviour ofcourse is that tasks spend little time processing and more time delaying.
If a tasks requests a 5 tick delay and other tasks spend 10 ticks processing, the first task will obviously not be
returned to in 5 ticks, but 10 ticks. In general, this means a requested delay of X ticks will result in an actual
delay of X or more ticks. A task that does not wish to wait (but must still cooperate in task switching) can invoke
task switching without delay by calling cmt_delay_ticks(0).

Any single task project that uses delay loops can be converted to cooperative multi tasking by simply replacing
non critical delays with cmt_delay_ticks.

There is no mechanism for ending a task once started, so every task is basically one big while(1) loop.

Although mutexes are rarely needed in a cooperative multitasking scenario (since task switching in under
current task's control), mutex functions are implemented for convenience (and because it was fun to do).

@file		cmt.c
@brief		Simple cooperative "on-delay" multitasking for STM32 F1
@author		Matej Kogovsek (matej@hamradio.si)
@copyright	LGPL 2.1
@note		This file is part of mat-stm32f1-lib
*/

#include "stm32f10x.h"

#include "cmt.h"

volatile uint8_t cmt_curtask = 0; /**< Currently running task */
volatile struct cmt_task cmt_tasks[CMT_MAXTASKS]; /**< Array of task state structs. */
volatile uint8_t cmt_numtasks = 1; /**< Number of defined tasks */

/**
@brief Delay d ticks.

Task switching is done here. This should not be called with interrupts disabled!
@param[in]	d		Number of ticks (usually ms) to delay.
*/
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

/**
@brief Add task to switching logic.
@param[in]	task_proc		Pointer to task procedure
@param[in]	task_sp			Task stack pointer
@return Number of defined tasks. If CMT_MAX_TASKS are already running, returns 0.
*/
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

/**
@brief Call within a timer interrupt.

If you want cmt_delay_ticks to mean cmt_delay_ms, simply call this function every ms.
*/
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

/**
@brief Returns the task's minimal detected stack pointer.

Used for determining the task's required stack size. Note that the returned value is an approximation.
@param[in]	task_num	Task number (zero based).
@return Task's minimal detected stack pointer. If task_num is out of bounds, returns 0.
*/
uint32_t cmt_minsp(uint8_t task_num)
{
	if( task_num < cmt_numtasks ) {
		return cmt_tasks[task_num].minsp;
	}
	return 0;
}

/**
@brief Tries to acquire mutex.
@param[in]	m		Pointer to caller allocated cmt_mutex.
@return True on success (acquired), false otherwise.
*/
uint8_t cmt_try_acquire(struct cmt_mutex* m)
{
	if( (m->ot == cmt_curtask) || (m->ac == 0) ) {
		m->ot = cmt_curtask;
		m->ac++;
		return 1;
	}
	return 0;
}

/**
@brief Waits until mutex acquired.
@param[in]	m		Pointer to caller allocated cmt_mutex.
*/
void cmt_acquire(struct cmt_mutex* m)
{
	while( !cmt_try_acquire(m) ) {
		cmt_delay_ticks(0);
	}
}

/**
@brief Releases mutex.
@param[in]	m		Pointer to caller allocated cmt_mutex.
*/
void cmt_release(struct cmt_mutex* m)
{
	if( (m->ot == cmt_curtask) && (m->ac > 0) ) {
		m->ac--;
	}
}
