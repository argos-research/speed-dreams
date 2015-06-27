#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <inttypes.h>
#include <time.h>
#include "timer.h"

#define errExit(msg) perror(msg), exit(EXIT_FAILURE);

#define CLOCKID CLOCK_REALTIME
#define SIG SIGRTMIN /* The first of the available real-time signals */

timer_t timerInit(void (*callback)(int signal_num), uint64_t timer_period_ns)
{
    timer_t timerid;
    struct sigevent sev;
    struct itimerspec its;

    /* Establish handler for timer signal */
    signal(SIG, callback);

    /* Block the timer signal (needs to be explicitly enabled) */
    disableTimerSignal();

    /* Create the timer */
    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = SIG;
    sev.sigev_value.sival_ptr = &timerid;
    if (timer_create(CLOCKID, &sev, &timerid) == -1)
        errExit("timer_create");

    /* Start the timer */
    its.it_value.tv_sec = timer_period_ns / 1000000000;
    its.it_value.tv_nsec = timer_period_ns % 1000000000;
    its.it_interval.tv_sec = its.it_value.tv_sec;
    its.it_interval.tv_nsec = its.it_value.tv_nsec;

    if (timer_settime(timerid, 0, &its, NULL) == -1)
        errExit("timer_settime");

    return timerid;
}

void timerEnd(timer_t timerid)
{
    signal(SIG, SIG_DFL);
    timer_delete(timerid);
}

/* Unlock the timer signal, so that timer notification can be delivered */
void enableTimerSignal(void)
{
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIG);
    if (sigprocmask(SIG_UNBLOCK, &mask, NULL) == -1)
        errExit("sigprocmask");
    return;
}

/* Block the timer signal */
void disableTimerSignal(void)
{
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIG);
    if (sigprocmask(SIG_SETMASK, &mask, NULL) == -1)
        errExit("sigprocmask");
    return;
}

