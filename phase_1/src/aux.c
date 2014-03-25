/*
 * Auxiliar Functions for SOS phase1
 */

void memset(void* ptr,int value,unsigned int num)
{
    if(num != 0)
    {
	*((unsigned char*)ptr) = (unsigned char)value;
	memset(++ptr,value,--num);
    }
}
