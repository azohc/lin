#include <linux/errno.h>
#include <sys/syscall.h>
#include <linux/unistd.h> 
#include <stdlib.h> 
#include <stdio.h> 

// #define __NR_gettid 332
extern int errno ;

// long ledctl(void) {
// 	return (long) syscall(__NR_gettid);
// }

int main (int argc, char *argv[]) {

	if (argc != 1) {
		perror("Usage: ./ledctl_invoke <ledmask>");
		return -1;
	} else if () {
		
		errnum = errno;
      	fprintf(stderr, "Value of errno: %d\n", errno);

		int number = (int)strtol(argv, NULL, 0);

		errnum = errno;
      	fprintf(stderr, "Value of errno: %d\n", errno);
	}


	// long retval = ledctl();
	// printf("El código de retorno de la llamada gettid es %ld\n", retval);
	return 0;
}

// syscall() devuelve -1 en caso de error
// El código de error queda almacenado en la variable errno (usar perror())