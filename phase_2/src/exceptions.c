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

#include"mutex.h"
#include"init.h"
#include"procInterface.h"

/* Functions Prototypes */
HIDDEN int CreateProcess(state_t *initialState, U32 priority, U32 cpuNumber);
void Terminate_Process(pcb_t* procToKill);
HIDDEN void Verhogen(int *semaddr);
HIDDEN void Passeren(int *semaddr);
HIDDEN void SpecTrapVec(U32 ExcType, U32 oldProcState, U32 newProcState);
HIDDEN U32 Get_CPU_Time();
HIDDEN void Wait_For_Clock();
HIDDEN U32 Wait_for_IO_Device(U32 intLineNumber, U32 deviceNumber, U32 waitForReadOp);
HIDDEN void devicePasseren(int *semaddr);


/* Save a state_t* in a defined physical address */
void saveState(state_t *addr,state_t *ps){
        U32 i;
        addr->entry_hi = ps->entry_hi;
        addr->cause = ps->cause;
        addr->status = ps->status;
        addr->pc_epc = ps->pc_epc;
        addr->hi = ps->hi;
        addr->lo = ps->lo;
        for( i=0 ; i<29 ; i++)
                addr->gpr[i] = ps->gpr[i];
}

/**************************************************** SYSCALL SUPPORT FUNCTIONS ***********************************************************/

/* SYS1 */
HIDDEN int CreateProcess(state_t *initialState, U32 priority, U32 cpuNumber)
{
        int i;
        cpuNumber = cpuNumber % GET_NCPU; /* cpu number where the process will execute */
        if(initialState == NULL || priority > 19)
        {
                pcb_t* procToKill = Current_Process[getPRID()];
                Current_Process[getPRID()] = NULL;
                Terminate_Process(procToKill);
        }

        /* CRITICAL SECTION */
        mutex_sched_in();
        pcb_t* newPcb = allocPcb(); /* trying to alloc a new pcb  for the new process */
        mutex_sched_out();
        /* END CRITICAl SECTION */

        if(newPcb == NULL)
        {
                return (S32)-1; /* TYPE MISMATCH! it should be -1 but reg_v0 is U32! */
        }
        else
        {
                saveState(&newPcb->p_s,initialState);
                newPcb->priority = newPcb->s_priority = priority;
                newPcb->must_die = 0;
                newPcb->tod_run = 0;
                newPcb->exRunTime = 0;
                newPcb->cpu_id = cpuNumber;
                for( i=0 ; i< SYS5_TYPES ; i++){
                        newPcb->excpHandler[i] =  NULL;
                        newPcb->old_state_addr[i] = NULL;
                }
                /* CRITICAL SECTION */
                mutex_sched_in();
                newPcb->pid = ++process_count;
                insertChild(Current_Process[getPRID()], newPcb); /* I can request to insert another child from another cpu at the same time */
                insertProcQ(&Ready_Queue[cpuNumber],newPcb); /* insert the new process in the ready queue of the cpu #cpuNumber */
                mutex_sched_out();
                /* END CRITICAL SECTION */

                return 0; /* CREATEPROCESS SUCCEEDED! */
        }
}

/* SYS2 */
void Terminate_Process(pcb_t* procToKill) /* MUST set Current_Process[i]=NULL for obvious reasons */
{
        pcb_t *p;
        Current_Process[getPRID()] = NULL;
        while( ( p = removeChild(procToKill) )!=NULL )
                Terminate_Process(p);

        mutex_dev_in();
        if( procToKill->p_semkey != NULL ){
                if( procToKill->p_semkey != &(sem_pseudotimer) )
                        (*(procToKill->p_semkey))++;
                outBlocked(procToKill);
        }
        mutex_dev_out();
        mutex_sched_in();
        process_count--;
        outChild(procToKill);
        outProcQ(&Ready_Queue[getPRID()],procToKill);
        mutex_sched_out();
        freePcb(procToKill);
}

/* SYS3 */
HIDDEN void Verhogen(S32 *semaddr)
{
        pcb_t* unblocked_proc;
        mutex_dev_in();
        /* MUTEX IN */
        if( (*semaddr)++ < 0)
        {
                if((unblocked_proc = removeBlocked(semaddr)) == NULL) /* it must be != NULL because semaddr->s_key was < 0.. so some processes must be blocked on this semaphore */
                        PANIC();
                mutex_dev_out();
                /* MUTEX OUT */
                unblocked_proc->priority = unblocked_proc->s_priority; /* restore dynamic priority to the static priority */
                mutex_sched_in();
                insertProcQ(&Ready_Queue[getPRID()],unblocked_proc);
                mutex_sched_out();
        }
        else
                mutex_dev_out();
        /* MUTEX OUT */
}

/* SYS4 */
HIDDEN void Passeren(S32 *semaddr)
{
        pcb_t* procToBlock = Current_Process[getPRID()];
        /* MUTEX IN */
        mutex_dev_in();
        if( --(*semaddr) < 0)
        {
                insertBlocked(semaddr,procToBlock);
                mutex_dev_out(); /* MUTEX OUT */
                Current_Process[getPRID()] = NULL;
        }
        else
                mutex_dev_out();
        /* MUTEX OUT */
}


HIDDEN void ClockPasseren(){
        pcb_t *procToBlock = Current_Process[getPRID()];
        /* MUTEX IN */
        mutex_dev_in();
        sem_pseudotimer--;
        soft_block_count++;
        insertBlocked(&sem_pseudotimer,procToBlock);
        mutex_dev_out();
        /* MUTEX OUT */
        Current_Process[getPRID()] = NULL;
}

/* SYS5 */
HIDDEN void SpecTrapVec(U32 ExcType, U32 oldProcState, U32 newProcState)
{
        if( Current_Process[getPRID()]->excpHandler[ExcType] != NULL ) /* This SYSCALL has been already requested*/
        {
                pcb_t* procToKill = Current_Process[getPRID()];
                Terminate_Process(procToKill);
        }
        else{
                Current_Process[getPRID()]->old_state_addr[ExcType] = (state_t *) oldProcState;
                Current_Process[getPRID()]->excpHandler[ExcType] = (state_t *) newProcState;
        }
}

/* SYS6 */
HIDDEN U32 Get_CPU_Time()
{
        Current_Process[getPRID()]->exRunTime += GET_TODLOW - (Current_Process[getPRID()]->tod_run);
        Current_Process[getPRID()]->tod_run = GET_TODLOW;
        return Current_Process[getPRID()]->exRunTime;
}

/* SYS7 */
HIDDEN void Wait_For_Clock() /* P() on the pseudo clock semaphore  */
{
        ClockPasseren();
}

/* SYS8 */
HIDDEN U32 Wait_for_IO_Device(U32 intLineNumber, U32 deviceNumber, U32 waitForReadOp)
{
        switch(intLineNumber)
        {
                case INT_DISK:
                        devicePasseren(&sem_devices.sem_disk[deviceNumber]);
                        break;
                case INT_TAPE:
                        devicePasseren(&sem_devices.sem_tape[deviceNumber]);
                        break;
                case INT_UNUSED:
                        devicePasseren(&sem_devices.sem_network[deviceNumber]);
                        break;
                case INT_PRINTER:
                        devicePasseren(&sem_devices.sem_printer[deviceNumber]);
                        break;
                case INT_TERMINAL:
                        if(waitForReadOp)
                                devicePasseren(&sem_devices.sem_terminalRX[deviceNumber]);
                        else
                                devicePasseren(&sem_devices.sem_terminalTX[deviceNumber]);
                        break;
                default:
                        PANIC();
        }
        return deviceStatus;
}
/*************************************************END SYSCALL SUPPORT FUNCTIONS ***********************************************************/


HIDDEN void devicePasseren(S32 *semaddr)   /* differs from passeren because modifies soft_blockcount */
{
        pcb_t* procToBlock = Current_Process[getPRID()];
        /* MUTEX IN */
        mutex_dev_in();
        if( --(*semaddr) < 0)
        {
                soft_block_count++;
                insertBlocked(semaddr,procToBlock);
                mutex_dev_out(); /* MUTEX OUT */
                Current_Process[getPRID()] = NULL;
        }
        else
                mutex_dev_out();
        /* MUTEX OUT */
}

void pgmExcpHandler(){
        state_t *old_state;
        if( getPRID()==0 )
                old_state = (state_t *) PGMTRAP_OLDAREA;
        else
                old_state = &(newoldareas[getPRID()].pgmtrap_old_area);
        pcb_t *proc = Current_Process[getPRID()];
        if( proc->excpHandler[SYS5_PGMTRAP]==NULL ){
                Terminate_Process(proc);
                scheduler();
        }else{ /* SYSCALL5 has been previously called */
                saveState(proc->old_state_addr[SYS5_PGMTRAP],old_state);
                LDST((STATE_PTR) proc->excpHandler[SYS5_PGMTRAP] );
        }
}

void tlbExcpHandler(){
        state_t *old_state;
        if( getPRID()==0 )
                old_state = (state_t *) TLB_OLDAREA;
        else
                old_state = &(newoldareas[getPRID()].tlb_old_area);
        pcb_t *proc = Current_Process[getPRID()];
        if( proc->excpHandler[SYS5_TLB]==NULL ){
                Terminate_Process(proc);
                scheduler();
        }else{ /* SYSCALL5 has been previously called */
                saveState(proc->old_state_addr[SYS5_TLB],old_state);
                LDST((STATE_PTR) proc->excpHandler[SYS5_TLB] );
        }
}

void sysBkExcepHandler()
{
        state_t* oldState;
        state_t* oldPgmState;
        pcb_t *proc;
        proc = Current_Process[getPRID()];
        if( getPRID()==0 ){
                oldState = (state_t *) SYSBK_OLDAREA;
                oldPgmState = (state_t *) PGMTRAP_OLDAREA;
        }else{
                oldState = &(newoldareas[getPRID()].sysbk_old_area);
                oldPgmState = &(newoldareas[getPRID()].pgmtrap_old_area);
        }
        oldState->pc_epc += WORD_SIZE; /* set PC to next istruction */
        saveState(&proc->p_s,oldState);
        proc->exRunTime += GET_TODLOW - (proc->tod_run);
        proc->tod_run = GET_TODLOW;
        if( CAUSE_EXCCODE_GET(oldState->cause) == EXC_SYSCALL )
        {
                if( oldState->reg_a0 > 8 ){
                        if( proc->excpHandler[SYS5_SYSBK]==NULL ){
                                Terminate_Process(proc);
                                scheduler();
                        }else{
                                saveState(proc->old_state_addr[SYS5_SYSBK],oldState);
                                LDST((STATE_PTR) proc->excpHandler[SYS5_SYSBK] );
                        }
                }

                if( (oldState->status & STATUS_KUp) == STATUS_KUp ){ /* if it wasn't in kernelMode, it pops the stack*/
                        saveState(oldPgmState,oldState);
                        oldPgmState->cause = CAUSE_EXCCODE_SET(oldPgmState->cause,EXC_RESERVEDINSTR);
                        pgmExcpHandler(); /* generate a Program Trap */
                }
                else
                {
                        switch(oldState->reg_a0) /* service requested */
                        {
                                case CREATEPROCESS:
                                        oldState->reg_v0 = CreateProcess((state_t *)oldState->reg_a1,oldState->reg_a2,oldState->reg_a3);
                                        break;
                                case TERMINATEPROCESS:
                                        Terminate_Process(proc);
                                        scheduler();
                                        break;
                                case VERHOGEN:
                                        Verhogen((S32 *)oldState->reg_a1);
                                        break;
                                case PASSEREN:
                                        Passeren((S32 *)oldState->reg_a1);
                                        break;
                                case SPECTRAPVEC:
                                        SpecTrapVec(oldState->reg_a1 , oldState->reg_a2, oldState->reg_a3);
                                        break;
                                case GETCPUTIME:
                                        oldState->reg_v0 = Get_CPU_Time();
                                        break;
                                case WAITCLOCK:
                                        Wait_For_Clock();
                                        break;
                                case WAITIO:
                                        oldState->reg_v0 = Wait_for_IO_Device(oldState->reg_a1, oldState->reg_a2, oldState->reg_a3);
                                        break;
                        }
                        if( Current_Process[getPRID()] !=NULL ){
                                LDST((STATE_PTR)oldState); /* return execution to the caller */
                        }
                        else{
                                scheduler();
                        }
                }
        }
        else if(  CAUSE_EXCCODE_GET(oldState->cause) == EXC_BREAKPOINT )
        {
                if( proc->excpHandler[SYS5_SYSBK]==NULL ){
                        Terminate_Process(proc);
                        scheduler();
                }else{ /* SYSCALL5 has been previously called */
                        saveState(proc->old_state_addr[SYS5_SYSBK],oldState);
                        LDST((STATE_PTR) proc->excpHandler[SYS5_SYSBK] );
                }
        }
        else
                PANIC(); /* something must have gone wrong */
}

