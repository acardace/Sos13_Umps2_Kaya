/* Constants defined by our group */

/* Init stack pointer */
#define SCHED_SP	 RAMTOP - (FRAME_SIZE*(GET_NCPU-1))
#define INIT_SP          SCHED_SP - (FRAME_SIZE*GET_NCPU)


/*Maximum number of CPUs supported by uMps2 */
#define MAXCPU_N 16

/* Interrupt line for Processor Local Timer */
#define INT_IPI	0
#define INT_LOCAL_TIMER 1

/* Device register adress given an interrupt line and a device number */
#define DEV_ADDR_BASE(intLine,devNo) ( 0x10000050 + (( intLine - 3) * 0x80) + (devNo * 0x10) )

/* Interrupting Devices Bitmap */
#define DEV_BITMAP_ADDR(intLine)  *(U32*)(PENDING_BITMAP_START + ((intLine) -INT_LOWEST ) * WORD_SIZE)

/* Status macros */
#define STATUS_EXCPHANDLER	0x10000000 /* Kernel mode ON, VM OFF, Local Timer ON ,Int disabled */
#define STATUS_SCHED		0x1000FF00 /* Kernel mode ON, VM OFF, Local Timer ON ,Int disabled */
#define STATUS_PROC		0x1800FF15 /* Kernel mode ON, VM OFF, Local Timer ON ,Int enabled and unmasked */

/* Terminal Device Bitmask  */
#define TERM_MASK               0x000000FF /* to check only the status of the device */

