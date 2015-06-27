#ifndef _TIMER_H
#define _TIMER_H

timer_t timerInit(void (*callback)(int signal_num), uint64_t timer_period_ns);
void timerEnd(timer_t timerid);
void enableTimerSignal(void);
void disableTimerSignal(void);
#endif

