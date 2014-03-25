/* Machine specific includes */
#include<uMPStypes.h>
#include<libumps.h>

/* includes phase_1 */
#include<pcb.e>
#include<asl.e>


/* scheduler */
#include"scheduler.h"

/* our constants */
#include<myconst.h>

/*interrupts and exceptions */
#include"exceptions.h"
#include"interrupts.h"
#include"init.h"
#include"procInterface.h"

/*************************************** Kernel Data *********************************************************/

U32 process_count; /* number of processes in the system */
U32 soft_block_count; /* number of  blocked processes waiting for an interrupt */

pcb_t *Ready_Queue[MAXCPU_N]; /*queue of processes ready for execution */
pcb_t *Current_Process[MAXCPU_N]; /* the current executing process */
pcb_t *init; /* INIT process */
sem_devices_t sem_devices; /* semaphores for each device */
S32 sem_pseudotimer; /* semaphore for the pseudo clock timer */
areas_t newoldareas[MAXCPU_N];
U32 deviceStatus;


extern void test();
/*************************************** Kernel Data (END)****************************************************/

/* Functions to populate new process state areas
 * It takes the memory address to populate and the exception handler function
 */
HIDDEN void populateNewArea(memaddr newArea, memaddr excepHandler , U32 index){
    state_t *newState = (state_t*) newArea;
    STST(newState);
    newState->pc_epc = newState->reg_t9 = excepHandler;
    newState->reg_sp = RAMTOP - (FRAME_SIZE * index );
    newState->status = STATUS_EXCPHANDLER;
}


/* MAIN */

U32 main(){
    U32 i;
    /*                populate SYSCALL/BREAK                         */
    populateNewArea(SYSBK_NEWAREA, (memaddr) sysBkExcepHandler , 0);
    /*                populate Program New Area                      */
    populateNewArea(PGMTRAP_NEWAREA,(memaddr) pgmExcpHandler , 0);
    /*                populate TLB Management                        */
    populateNewArea(TLB_NEWAREA,(memaddr) tlbExcpHandler , 0);
    /*                populate Interrupt New Area                    */
    populateNewArea(INT_NEWAREA,(memaddr) intExcpHandler , 0);

    for( U32 index = 1 ; index < GET_NCPU ; index++ )
    {
	populateNewArea( (memaddr) &(newoldareas[index].int_new_area) ,(memaddr) intExcpHandler, index );
	populateNewArea( (memaddr) &(newoldareas[index].tlb_new_area) ,(memaddr) tlbExcpHandler, index );
	populateNewArea( (memaddr) &(newoldareas[index].pgmtrap_new_area) ,(memaddr) pgmExcpHandler, index );
	populateNewArea( (memaddr) &(newoldareas[index].sysbk_new_area) , (memaddr) sysBkExcepHandler, index );

    }
    /* populate Interrupt Routing Table */
    populateIRT();

    /* Initialize Level 2 Structures */
    initPcbs();
    initASL();
    /*initialize all nucleus maintained variables */
    for(i=0 ; i<GET_NCPU ; i++){
	Ready_Queue[i] = NULL;
	Current_Process[i] = NULL;
    }
    process_count = 0;
    soft_block_count = 0;

    /*initialize all nucleus maintained semaphores */
    for(i=0; i<DEV_PER_INT ; i++){
	sem_devices.sem_disk[i] = 0;
	sem_devices.sem_network[i] = 0;
	sem_devices.sem_printer[i] = 0;
	sem_devices.sem_tape[i] = 0;
	sem_devices.sem_terminalTX[i] = 0;
	sem_devices.sem_terminalRX[i] = 0;
    }
    sem_pseudotimer = 0;

    /* Start INIT and place it in the ready Queue */
    init = allocPcb();
    init->pid = (++process_count);
    init->must_die = 0; /* hasn't come its hour yet*/
    init->cpu_id = 0; /* init is assigned to cpu 0 */
    init->tod_run = 0;
    init->exRunTime = 0; /* total amount of cpu time consumed by the process */
    init->priority = DEFAULT_PRIORITY;
    init->s_priority = DEFAULT_PRIORITY;
    init->p_s.status = STATUS_PROC;
    init->p_s.reg_sp = INIT_SP;
    init->p_s.pc_epc = init->p_s.reg_t9 = (memaddr) test;
    insertProcQ(&Ready_Queue[0],init);

    /* turning on all the processors */
    for(i=1 ; i< GET_NCPU ; i++){
	state_t cpustate;
	STST(&cpustate);
	cpustate.reg_sp = SCHED_SP  - (i*FRAME_SIZE);
	cpustate.pc_epc = cpustate.reg_t9  = (memaddr) scheduler;
	cpustate.status = STATUS_SCHED;
	INITCPU(i,&cpustate, &(newoldareas[i].int_old_area) );
    }

    SET_IT(SCHED_PSEUDO_CLOCK);
    /* call the scheduler */
    scheduler();
    return 0;
}
