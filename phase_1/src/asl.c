#include "types13.h"
#include "pcb.e"
#include "const13.h"

semd_t semd_table[MAXPROC];
semd_t* semdFree_h;
semd_t* semd_h=NULL;

static semd_t* _getSemd(int* key,semd_t* semd) 
{
	if(semd == NULL)
	    return NULL;
	else if(semd->s_key == key)
	   return semd;
	else return _getSemd(key,semd->s_next);
}

semd_t* getSemd(int* key)
{
	return _getSemd(key,semd_h);
}

int insertBlocked(int* key,pcb_t* p) /*semd_h*/
{
	semd_t* semdToInsert = getSemd(key);
	
	/* assume key and p not null */
	if(semdToInsert == NULL) /* semd_h is empty or key is not present in semd_h */
	{
		if(semdFree_h == NULL)
		   return TRUE;
		else
		{
			/* SEMD allocation */
			semd_t* newSEMD = semdFree_h;
			semdFree_h = semdFree_h->s_next;
			newSEMD->s_key = key;
			p->p_semkey = key;
			p->p_next = NULL;
			newSEMD->s_procQ = p;
			newSEMD->s_next = semd_h;
			semd_h = newSEMD;
			return FALSE;
		}
	}
	else
	{
		insertProcQ(&semdToInsert->s_procQ,p);
		p->p_semkey = key;
	    return FALSE;
   }
}

static pcb_t* _removeBlocked(int* key,semd_t* semd_h)
{
	/* semd_h != NULL thanks to removeBlocked pre-check */
	if(semd_h->s_next != NULL)
	{
		if(semd_h->s_next->s_key == key)
		{
			pcb_t* result = removeProcQ(&semd_h->s_next->s_procQ);
			result->p_semkey = NULL; /* result is no longer blocked on the semaphore p_semkey */
			if(headProcQ(semd_h->s_next->s_procQ) == NULL)
			{
				semd_t* tmp = semd_h->s_next;
				semd_h->s_next = semd_h->s_next->s_next;
				tmp->s_next = semdFree_h;
				semdFree_h = tmp;
			}
			return result;
		}
		else return _removeBlocked(key,semd_h->s_next);
	}
	else return NULL; /*element not present */
}

pcb_t* removeBlocked(int* key)
{
	if(semd_h != NULL)
	{
		if(semd_h->s_key == key)
		{
			pcb_t* result = removeProcQ(&semd_h->s_procQ);
			result->p_semkey = NULL; /* result is no longer blocked on the semaphore p_semkey */
			if(headProcQ(semd_h->s_procQ) == NULL)
			{
			   semd_t* tmp = semd_h;
			   semd_h = semd_h->s_next;
			   tmp->s_next = semdFree_h;
			   semdFree_h = tmp;
		    }
		    return result;
		}
		else
		{
			return _removeBlocked(key,semd_h);
		}
	}
	else return NULL; /* semd_h empty */
}

pcb_t* headBlocked(int* key)
{
	semd_t* tmp = getSemd(key);
	if(tmp == NULL)
	   return NULL; /* sem does not exist */
    else
       return headProcQ(tmp->s_procQ);
}

static void _outBlocked(semd_t* target,semd_t* list)
{
	if(list->s_next != NULL)
	{
		if(list->s_next == target)
		{
			list->s_next = list->s_next->s_next;
			target->s_next = semdFree_h;
			semdFree_h = target;
		}
		else
		{
			_outBlocked(target,list->s_next);
		}
	}
}

pcb_t* outBlocked(pcb_t *p)
{
	/* assume p != null */
	if(p->p_semkey != NULL)
	{
		semd_t* sem = getSemd(p->p_semkey); /* sem must be != NULL because p_semkey != NULL */
		pcb_t* result = outProcQ(&sem->s_procQ,p);
		result->p_semkey = NULL; /* result is no longer blocked on the semaphore p_semkey */
		if(headProcQ(sem->s_procQ) == NULL)
		{
			if(sem == semd_h)
			{
				semd_h = semd_h->s_next;
				sem->s_next = semdFree_h;
				semdFree_h = sem;
			}
			else
			{
				_outBlocked(sem,semd_h);
			}
		}
		return result;
	}
	else
	   return NULL;
}

static void _outChildBlocked(pcb_t* p)
{
	outBlocked(p);
	if(p->p_sib != NULL)
	   _outChildBlocked(p->p_sib);
	if(p->p_first_child != NULL)
	   _outChildBlocked(p->p_first_child);
}

void outChildBlocked(pcb_t* p)
{
	outBlocked(p);
	if(p->p_first_child != NULL)
	{
		_outChildBlocked(p->p_first_child);
	}
}

static void _linkSemd(unsigned int i,unsigned int dim)
{
	if(i<dim-1) /* there is another element after the current "i"*/
	{
		semd_table[i].s_next = &semd_table[i+1];
		_linkSemd(i+1,dim);
	}
	else if (i==dim-1) /* "i" is the last element*/
	{
		semd_table[i].s_next = NULL;
	}
		
}

void initASL()
{
	semdFree_h = semd_table;
	_linkSemd(0,MAXPROC);
}
