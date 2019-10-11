#include <linux/errno.h>
#include <sys/syscall.h>
#include <linux/unistd.h> 
#include <stdlib.h> 
#include <stdio.h>
#define __NR_gettid 332

extern int errno ;

long ledctl(unsigned int n) {
	return (long) syscall(__NR_gettid, n);
}

int main (int argc, char *argv[]) {
	unsigned int numb;

	if (argc != 2) {
		printf("Usage: ./ledctl_invoke <ledmask>\n");
		return -1;
	}

	numb = strtoul(argv[1], NULL, 16);

	if(ledctl(numb) == -1) {
		perror("Se ha producido un error" /* %d", errno*/);
		return -1;
	}
	
	return 0;
}



// syscall() devuelve -1 en caso de error
// El código de error queda almacenado en la variable errno (usar perror())
