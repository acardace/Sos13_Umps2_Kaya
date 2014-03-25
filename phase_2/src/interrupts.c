/* Device and timer interrupts management */

/* Machine specific includes */
#include<uMPStypes.h>
#include<libumps.h>
/* includes phase_1 */
#include<pcb.e>
#include<asl.e>

#include"init.h"
#include<myconst.h>
#include"scheduler.h"
#include"exceptions.h"

/*mutex*/
#include"mutex.h"

/*Interprocess Communication */
#include"procInterface.h"

HIDDEN void intTimer(){
   pcb_t *awake;
   /* CRITICAL SECTION */
   mutex_dev_in();
   if( sem_pseudotimer < 0 ){
      awake = removeBlocked(&sem_pseudotimer);
      sem_pseudotimer++;
      soft_block_count--;
      mutex_dev_out();
      /* END OF CRITICAL SECTION */
      awake->priority = awake->s_priority;
      /*CRITICAL SECTION*/
      mutex_sched_in();
      insertProcQ(&Ready_Queue[getPRID()],awake);
      mutex_sched_out();
      /* END OF CRITICAL SECTION */
      intTimer(); /* unblock all of the waiting procs*/
   }else{
      sem_pseudotimer = 0;
      mutex_dev_out();
      /* END OF CRITICAL SECTION */
   }
   SET_IT(SCHED_PSEUDO_CLOCK);
}

HIDDEN void deviceVerhogen(S32 *key){
   pcb_t *awake;
   /* CRITICAL SECTION */
   mutex_dev_in();
   if( (*key)++ < 0  ){
      awake = removeBlocked(key);
      soft_block_count--;
      mutex_dev_out();
      /* END OF CRITICAL SECTION */
      awake->p_s.reg_v0 = deviceStatus;
      awake->priority = awake->s_priority;
      /*CRITICAL SECTION*/
      mutex_sched_in();
      insertProcQ(&Ready_Queue[getPRID()],awake);
      mutex_sched_out();
      /* END OF CRITICAL SECTION */
   }else
      mutex_dev_out();
      /* END OF CRITICAL SECTION */
}

HIDDEN void procLocalTimer(){ // the scheduling slice has ended, let's get our hands dirty!
   U32 cpu_id = getPRID();
   if( Current_Process[cpu_id]!=NULL ){
      Current_Process[cpu_id]->exRunTime+= GET_TODLOW - (Current_Process[cpu_id]->tod_run);
      Current_Process[cpu_id]->tod_run = GET_TODLOW;
      Current_Process[cpu_id]->priority = Current_Process[cpu_id]->s_priority; /* get dynamic priority back to its static value, for niceness purposes  */
      /* CRITICAL SECTION */
      mutex_sched_in();
      insertProcQ(&Ready_Queue[cpu_id],Current_Process[cpu_id]);
      mutex_sched_out();
      /* END OF CRITICAL SECTION */
   }
   Current_Process[getPRID()] = NULL;
}

HIDDEN void deviceInterrupt(U32 intLine){
   U32 intBitmap = DEV_BITMAP_ADDR(intLine); /* interrupting devices bitmap word for disk devices */
   U32 devNo;
   S32 *semaphore;
   U32 termDev; /* 0 if character received, 1 if char transmitted */
   dtpreg_t *device;
   termreg_t *term;
   for( devNo=0 ; devNo< DEV_PER_INT ; devNo++){ /* who did it?? */
      if( intBitmap & 0x1 )
         break;
      intBitmap = intBitmap>>1;
   }
   /* ACK the outstanding interrupt */
   if( intLine == INT_TERMINAL ){
      term = (termreg_t *) DEV_ADDR_BASE(intLine,devNo);
      if( (term->transm_status & TERM_MASK ) == DEV_TTRS_S_CHARTRSM ){
	 deviceStatus = term->transm_status;
         term->transm_command = DEV_C_ACK;
         termDev = 1;
      }
      else if( (term->recv_status & TERM_MASK ) == DEV_TRCV_S_CHARRECV ){
	 deviceStatus = term->recv_status;
         term->recv_command = DEV_C_ACK;
         termDev = 0;
      }
   }
   else{
      device = (dtpreg_t *) DEV_ADDR_BASE(intLine,devNo);
      deviceStatus = device->status;
      device->command = DEV_C_ACK;
   }
   switch (intLine){
      case INT_DISK:
         semaphore = (&sem_devices.sem_disk[devNo]);
         break;
      case INT_TAPE:
         semaphore = (&sem_devices.sem_tape[devNo]);
         break;
      case INT_UNUSED:
         semaphore = (&sem_devices.sem_network[devNo]);
         break;
      case INT_PRINTER:
         semaphore = (&sem_devices.sem_printer[devNo]);
         break;
      case INT_TERMINAL:
         if( termDev == 1 )
            semaphore = (&sem_devices.sem_terminalTX[devNo]);
         else
            semaphore = (&sem_devices.sem_terminalRX[devNo]);
         break;
   }
   deviceVerhogen(semaphore);
}

void intExcpHandler(){
   state_t *old_area;
   if( getPRID() == 0 )
       old_area = (state_t *) INT_OLDAREA;
   else
       old_area = &(newoldareas[getPRID()].int_old_area);
   U32 cause = old_area->cause;
   if( Current_Process[getPRID()] != NULL ){
           pcb_t* proc = Current_Process[getPRID()];
           saveState(&proc->p_s,old_area);
   }

   if(CAUSE_IP_GET( cause,INT_IPI) ){ /* Interprocess Communication */
      receiveIPI();
      LDST(old_area);
   }
   else if( CAUSE_IP_GET(cause,INT_LOCAL_TIMER) )
      procLocalTimer();

   else if( CAUSE_IP_GET(cause,INT_TIMER) )
      intTimer();

   else if( CAUSE_IP_GET(cause,INT_DISK) )
      deviceInterrupt(INT_DISK);

   else if( CAUSE_IP_GET(cause,INT_TAPE) )
      deviceInterrupt(INT_TAPE);

   else if( CAUSE_IP_GET(cause,INT_UNUSED) )
      deviceInterrupt(INT_UNUSED);

   else if( CAUSE_IP_GET(cause,INT_PRINTER) )
      deviceInterrupt(INT_PRINTER);

   else if( CAUSE_IP_GET(cause,INT_TERMINAL) )
      deviceInterrupt(INT_TERMINAL);

   /* once the interrupt is happy and fully managed */
   if( Current_Process[getPRID()] != NULL )
      LDST(old_area);
   else{
      scheduler();
   }
}
