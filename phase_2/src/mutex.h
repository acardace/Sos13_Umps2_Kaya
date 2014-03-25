/* mutex data */

extern volatile U32 mutex_scheduler; /* to access ready queues and process count */
extern volatile U32 mutex_dev; /* to access devices semaphores */

/* funtion prototypes */

extern void mutex_sched_in(); /* to access ready queues and process_counter */
extern void mutex_sched_out();
extern void mutex_dev_in(); /* to access devices sempahores */
extern void mutex_dev_out();


