#include "types13.h"
#include "aux.e"

pcb_t pcb_table[MAXPROC]; 
pcb_t *pcbfree_h; 


/*
 * Links pcb each other in the pcb_table
*/
static void _linkPcb(unsigned int i,unsigned int dim)
{
	if(i<dim-1) /* there is another element after the current "i"*/
	{
		pcb_table[i].p_next = &pcb_table[i+1];
		_linkPcb(i+1,dim);
	}
	else if (i==dim-1) /* "i" is the last element*/
	{
		pcb_table[i].p_next = NULL;
	}
		
}

void initPcbs(void)
{
	pcbfree_h = pcb_table;
	_linkPcb(0,MAXPROC); /*it takes the starting index and the dim of pcb_table */
}


void freePcb(pcb_t *p)
{
	p->p_next = (pcbfree_h);
	pcbfree_h = p;
}

pcb_t *allocPcb(void)
{
	if(pcbfree_h == NULL)
	   return NULL;
	pcb_t* tmpPtr = pcbfree_h;
	pcbfree_h = pcbfree_h->p_next;
	tmpPtr->p_next = NULL;
	tmpPtr->p_parent = NULL;
	tmpPtr->p_first_child = NULL;
	tmpPtr->p_sib = NULL;
	memset(&(tmpPtr->p_s),0,sizeof(tmpPtr->p_s));
	tmpPtr->priority = 0;
	tmpPtr->p_semkey = NULL;
	return tmpPtr;
}

static void _insertProcQ(pcb_t* head,pcb_t* p)
{
	if(head->p_next != NULL) /* look ahead by one element */
	{
		if(p->priority > head->p_next->priority)
		{
			p->p_next = head->p_next;
			head->p_next = p;
		}
		else
		{
			_insertProcQ(head->p_next,p);
		}
	}
	else
	{   
		/* p is the last element in the list */
		head->p_next = p;
		p->p_next = NULL;
	}
}

void insertProcQ(pcb_t **head,pcb_t* p)
{
	/* assume head != NULL */
	if(p != NULL)
	{
		if(*head != NULL)
		{
			if(p->priority > (*head)->priority)
			{
				/* head insertion */
				p->p_next = (*head);
				(*head) = p;
			}
			else
			{
				_insertProcQ(*head,p);
			}
		}
		else
		{
			/* list is empty and p is the first element */
			*head = p;
			p->p_next = NULL;
		}
	}
}

pcb_t* headProcQ(pcb_t* head)
{
	if(head == NULL)
	   return NULL;
	return head;
}

pcb_t* removeProcQ(pcb_t** head)
{
	/* assume head != NULL */
	
	if(*head == NULL)
	   return NULL;
	else
	{
		pcb_t* ptr = *head;
	    *head = (*head)->p_next;
	    ptr->p_next = NULL; /* unlink removed element */ 
	    return ptr;
	} 
}

static pcb_t* _outProcQ(pcb_t* head,pcb_t *p)
{
	if(head->p_next != NULL) /* look ahead by one element */
	{
		if(head->p_next == p)
		{
			pcb_t* ptr;
			ptr = head->p_next;
			head->p_next = head->p_next->p_next; /* links what is before head->p_next with what is after head->p_next */
			ptr->p_next = NULL; /* unlink removed element */
			return ptr;
		}
		else
		   return _outProcQ(head->p_next,p);
	}
	else
	   return NULL;
}

pcb_t* outProcQ(pcb_t** head,pcb_t *p)
{
	/* assume head != NULL */
	
	if(*head == NULL)
	   return NULL; /* if the queue is empty is returned NULL anyway */
	else
	{
		pcb_t* ptr;
		
		if(*head == p)
		{
			ptr = (*head);
			(*head) = (*head)->p_next;
			ptr->p_next = NULL; /* unlink removed element */
			return ptr;
		}
		else
		   return _outProcQ(*head,p);
	}
	
}


static void _insertChild(pcb_t* child,pcb_t* p)
{
	/* child != null, look at insertChild... for the first call of the function*/
	
	if(child->p_sib == NULL)
	{
		child->p_sib = p;
		p->p_sib = NULL;
	}
	else
	   _insertChild(child->p_sib,p);
}

void insertChild(pcb_t* parent,pcb_t* p)
{	
	if(parent->p_first_child == NULL)
	{
		parent->p_first_child = p;
		p->p_sib = NULL;
	}
	else
	    _insertChild(parent->p_first_child,p);
	
	p->p_parent = parent;
}

pcb_t* removeChild(pcb_t* p)
{
	if(p->p_first_child == NULL)
	   return NULL;
	else
	{
		pcb_t* ptr = p->p_first_child;
		p->p_first_child = p->p_first_child->p_sib;
                ptr->p_parent = NULL;
		ptr->p_sib = NULL; /* unlink removed element */
		return ptr;
	}
}

static void _outChild(pcb_t* child,pcb_t* p)
{
	/* child != null, look at outChild... for the first call of the function*/
	
	if(child->p_sib != NULL) 
	{
		if(child->p_sib == p)
		{
			child->p_sib = child->p_sib->p_sib; /* links what is before child->p_sib with what is after child->p_sib */
		}
		else
		{
			_outChild(child->p_sib,p);
		}
	}
}

pcb_t* outChild(pcb_t* p)
{
	if(p->p_parent == NULL)
	   return NULL;
	if(p->p_parent->p_first_child == p)
	{
		p->p_parent->p_first_child = p->p_parent->p_first_child->p_sib;
	}
	else
	{
		_outChild(p->p_parent->p_first_child,p);
	}
        p->p_parent = NULL; 
	p->p_sib = NULL; /* unlink removed element */
	return p;
}
