/* Kernel data Structures definitions */

/* semaphore struct for each external (sub)device */
typedef struct{
    S32 sem_disk[DEV_PER_INT];
    S32 sem_tape[DEV_PER_INT];
    S32 sem_network[DEV_PER_INT];
    S32 sem_printer[DEV_PER_INT];
    S32 sem_terminalTX[DEV_PER_INT];
    S32 sem_terminalRX[DEV_PER_INT];
} sem_devices_t;


typedef struct{
    state_t int_old_area;
    state_t int_new_area;
    state_t tlb_old_area;
    state_t tlb_new_area;
    state_t pgmtrap_old_area;
    state_t pgmtrap_new_area;
    state_t sysbk_old_area;
    state_t sysbk_new_area;
} areas_t;


/*************************************** Kernel Data *********************************************************/

extern U32 process_count; /* number of processes in the system */
extern U32 soft_block_count; /* number of  blocked processes waiting for an interrupt */
extern pcb_t *Ready_Queue[MAXCPU_N]; /*queue of processes ready for execution */
extern pcb_t *Current_Process[MAXCPU_N]; /* the current executing process */
extern pcb_t *init; /* INIT process */
extern sem_devices_t sem_devices; /* semaphores for each device */
extern S32 sem_pseudotimer; /* semaphore for the pseudo clock timer */
extern areas_t newoldareas[MAXCPU_N];
extern U32 deviceStatus;

