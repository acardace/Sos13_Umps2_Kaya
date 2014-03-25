#ifndef _TYPES13_H
#define _TYPES13_H

#include "uMPStypes.h"
#include<myconst.h>

/* Process Control Block (PCB) data structure*/
typedef struct pcb_t {
	/*process queue fields */
	struct pcb_t* p_next;

	/*process tree fields */
	struct pcb_t* p_parent;
	struct pcb_t* p_first_child;
	struct pcb_t* p_sib;

	/* processor state, etc */
	state_t	p_s;

	/* process priority */
	int	priority; /* this is the dynamic priority */
	int     s_priority; /* this is the static priority */

	/* key of the semaphore on which the process is eventually blocked */
	int	*p_semkey;

        /* process id */
	U32 pid;

        /* cpu_id where the process is running */
        U32 cpu_id;

       /* when its time has come... */
       U32 must_die;

       /* TOD when the process runs */
       U32 tod_run;

       /* amount of running time for the process */
       U32 exRunTime;

       /* PgmTrap/SysBp/TLB excetion handler defined after calling SYS5 */
       state_t *excpHandler[SYS5_TYPES];

       /* Address into which the old processor state is to be stored after an exception occurs, defined after calling SYS5 */
       state_t *old_state_addr[SYS5_TYPES];
} pcb_t;

/* Semaphore Descriptor (SEMD) data structure*/
typedef struct semd_t {
	struct semd_t* s_next;

	/* Semaphore value*/
	int* s_key;

	/* Queue of PCBs blocked on the semaphore*/
	pcb_t *s_procQ;
} semd_t;

#endif
