#include <linux/errno.h>
#include <sys/syscall.h>
#include <linux/unistd.h> 
#include <stdlib.h> 
#include <stdio.h>
#define __NR_gettid 332

extern int errno ;

long ledctl(void) {
	return (long) syscall(__NR_gettid);
}

int main (int argc, char *argv[]) {
	char * buf;
	unsigned int numb;
	char * aux;
	if (argc != 1) {
		perror("Usage: ./ledctl_invoke <ledmask>");
		return -1;
	} 

	numb = strtoul(argv[0], NULL, 16);

	if(ledctl(numb) == -1)
		perror("Se ha producido un error: %d", errno);
	
	return 0;
}



// syscall() devuelve -1 en caso de error
// El código de error queda almacenado en la variable errno (usar perror())
