/* The scheduler, to be called by init */

/* Machine specific includes */
#include<uMPStypes.h>
#include<libumps.h>

/* includes phase_1 */
#include<pcb.e>
#include<asl.e>

/* our constants */
#include<myconst.h>

/* data structures */
#include"init.h"
#include"mutex.h"
#include"procInterface.h"
#include"scheduler.h"


HIDDEN void updateVirtualPriority(pcb_t* pcb, void *args)
{
    pcb->priority--;
}


HIDDEN U32 procPriority(U32 pty){
    U32 ret_value = DEFAULT_PRIORITY ;
    switch( pty ){
	case 19 :
	    ret_value = 0;
	    break;
	case 18:
	    ret_value = 1;
	    break;
	case 17:
	    ret_value = 2;
	    break;
	case 16:
	    ret_value = 3;
	    break;
	case 15:
	    ret_value = 4;
	    break;
	case 14 :
	    ret_value = 5;
	    break;
	case 13:
	    ret_value = 6;
	    break;
	case 12:
	    ret_value = 7;
	    break;
	case 11:
	    ret_value = 8;
	    break;
	case 10:
	    ret_value = 9;
	    break;
	case 9 :
	    ret_value = 10;
	    break;
	case 8:
	    ret_value = 11;
	    break;
	case 7:
	case 6:
	    ret_value = 12;
	    break;
	case 5:
	case 4:
	    ret_value = 13;
	    break;
	case 3:
	case 2:
	    ret_value = 14;
	    break;
	case 1:
	case 0:
	    ret_value = 15;
	    break;
	default:
	    ret_value = DEFAULT_PRIORITY;
	    break;
    }
    return ret_value;
}

void scheduler()
{
    U32 cpu_id = getPRID(); /* current cpu id executing the scheduler */
    volatile procInterface_t *procIntReg;
    procIntReg = (procInterface_t *) PROC_IF_ADDR;
    procIntReg->tpr = 0; /* clear this field out */
    if(headProcQ(Ready_Queue[cpu_id]) == NULL) /* the readyQueue of the current  (cpu_id) is empty */
    {
    /* CRITICAL SECTION */
       mutex_sched_in();
       if(process_count == 0){
          mutex_sched_out(); /* otherwise it's deadlock */
          HALT();
	}
/*	else if(process_count > 0   && soft_block_count == 0 ){
           mutex_sched_out();
            PANIC();
	}*/
	else{
	    mutex_sched_out();
            procIntReg->tpr |= 15;
	    setSTATUS( STATUS_PROC  );
	    setTIMER(SCHED_TIME_SLICE);
	    WAIT();
	}
    }
    Current_Process[cpu_id] = removeProcQ(&Ready_Queue[cpu_id]);// current process
    forallProcQ(headProcQ(Ready_Queue[cpu_id]), updateVirtualPriority, NULL);
    Current_Process[cpu_id]->p_s.status |= STATUS_TE;
    Current_Process[cpu_id]->tod_run = GET_TODLOW;
    procIntReg->tpr |= procPriority(Current_Process[cpu_id]->s_priority);
    setTIMER(SCHED_TIME_SLICE);
    LDST(&Current_Process[cpu_id]->p_s); /* leaves the control to the process */
    /* some troubles comes here... */
}
