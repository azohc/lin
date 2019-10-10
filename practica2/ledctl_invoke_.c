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
	char * buf;
	unsigned int dist, numb;
	char * aux;
	if (argc != 1) {
		perror("Usage: ./ledctl_invoke <ledmask>");
		return -1;
	} 
	buf= argv[0];
	if (strlen(argv[0]) > 2){
		if (argv[0][0] == 0 && argv[0][1] == 'x'){
			dist = strlen(argv[0])-2;
			memcpy(aux, &argv[0][2], dist;
			aux[dist]= '\0';
			buf = aux;
		}

	}

	numb = strtoul(buf, NULL, 10);

	if(ledctl(num) == -1)
		perror("Se ha producido un error.");
	
	return 0;
}



// syscall() devuelve -1 en caso de error
// El código de error queda almacenado en la variable errno (usar perror())
