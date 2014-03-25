#include<libumps.h>
#include<base.h>
/* mutex module for all system's semaphores */

/* mutex data */

volatile U32 mutex_scheduler = 0; /* to access ready queues and process count */
volatile U32 mutex_dev =  0; /* to access devices semaphores */

/* mutex global functions */

void mutex_sched_in(){
    /* it enters iff mutex = 1. After CAS function is called, mutex is 0 until someone calls mutex_out */
    while(!CAS(&mutex_scheduler,0,1));
}

void mutex_sched_out(){
    CAS(&mutex_scheduler,1,0);
}

void mutex_dev_in(){
    while(!CAS(&mutex_dev,0,1));
}

void mutex_dev_out(){
    CAS(&mutex_dev,1,0);
}

