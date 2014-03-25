#include"procInterface.h"
#include<const13.h>
#include<libumps.h>
#include"mutex.h"
#include<pcb.e>
#include"init.h"

extern void scheduler();

void sendIPI(U32 cpu_no,U32 message){
    procInterface_t *pInterface;
    U32 cpu_dest;
    pInterface = (procInterface_t *) PROC_IF_ADDR;
    /* clear out field */
    pInterface->outbox = 0;
    if( cpu_no>15 )
	cpu_dest = ALLCPUS;
    else
	cpu_dest = TO_PROC(cpu_no);
    pInterface->outbox |= cpu_dest; /* set recipients */
    pInterface->outbox |= message; /*set message to send */
}

void receiveIPI(){
    procInterface_t *pInterface;
    pInterface = (procInterface_t *) PROC_IF_ADDR;
    if( (pInterface->inbox & MSG_MASK) == KILL_PROCESS ){
	pInterface->inbox |= ACK_IPI; /* ACK */
    }
}

void populateIRT(){
    U32 i;
    U32 procBit = 0x0; /* containing the processor bits to be turned on*/
    irt_t *intRoutingEntry = (irt_t *) INT_RTG_TBL_BASE_ADDR;
    for(i=0 ; i<GET_NCPU ; i++){ /* build bitmask, according to how many CPUs in the system */
	procBit<<=1;
	procBit+=1;
    }
    for(i=INT_RTG_TBL_BASE_ADDR ; i<= INT_RTG_LAST_ADDR ; i+=WORD_SIZE){ /*populate all the entries */
	intRoutingEntry = (irt_t *) i;
	intRoutingEntry->irt_entry |= procBit;
	intRoutingEntry->irt_entry |= RP_DYNAMIC;
    }
}
