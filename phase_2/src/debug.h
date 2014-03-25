#define MAXBUF 1024
char buffer[MAXBUF];
void printf(char* str){
    char *ptr = buffer;
    while(*(ptr++) = *(str++));
}

