#include<base.h>

/* Processor Interface Data Structure */
typedef struct procInterface_t{
    U32 inbox;
    U32 outbox;
    U32 tpr;
} procInterface_t;

/* IRT */
typedef struct irt_t{
    U32 irt_entry;
} irt_t;

/* Function prototypes for IPI */

extern void sendIPI(U32 cpu_no,U32 message);
extern void receiveIPI();
extern void populateIRT(); /*to redirect interrupt to all the available CPUs */


/*Utility defitions */

/* Interrupt Routing utility definitions */
#define	 RP_DYNAMIC     (0x1<<28)

#define  INT_RTG_TBL_BASE_ADDR	0x10000300
#define  INT_RTG_LAST_ADDR      0x100003BC
#define  INT_RTG_DEVS		5
#define  ENTRY_PER_DEV	        8
#define  TOT_IRT_ENTRIES        ( (INT_RTG_DEVS * ENTRY_PER_DEV) -1)
#define  IRT_TBL_ENTRY(intLine,device)  INT_RTG_TBL_BASE_ADDR+(WORD_SIZE*((intLine-2)*8+device)))

/* Interrupt Delivery Controller Processor Interface Registers */

#define  PROC_IF_ADDR	0x10000400
#define	 INBOX		0x10000400
#define	 OUTBOX		0x10000404
#define	 TPR		0x10000408

/* Bitmasks and messages definitions */

#define	 MSG_MASK	0x000000ff
#define  ACK_IPI	0x000000ff

#define	 KILL_PROCESS	1

/* Recipients */

#define  ALLCPUS	0x00ffff00

#define	 TO_PROC(i)	(0x1<<((i)+8))
