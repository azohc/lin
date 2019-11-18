#include <getopt.h>
#include <stdio.h>
#include <sys/types.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <err.h>
#include <errno.h>

struct kfifo cbuffer; /* Buffer circular */

int prod_count = 0; /* Número de procesos que abrieron la entrada
						/proc para escritura (productores) */
int cons_count = 0; /* Número de procesos que abrieron la entrada
						/proc para lectura (consumidores) */
						
struct semaphore mtx; /* para garantizar Exclusión Mutua */
struct semaphore sem_prod; /* cola de espera para productor(es) */
struct semaphore sem_cons; /* cola de espera para consumidor(es) */

int nr_prod_waiting=0; /* Número de procesos productores esperando */
int nr_cons_waiting=0; /* Número de procesos consumidores esperando */


void fifoproc_open(bool abre_para_lectura) {

	lock(mtx);
	if (abre_para_lectura)
	{
	/* Un consumidor abrió el FIFO */
		cons_count++;
		cond_signal(prod, mtx);
		while (!prod_count) {
			cond_wait(cons, mtx);
		} 
	} else {
	/* Un productor abrió el FIFO */
		prod_count++;
		cond_signal(cons, mtx);
		while (!cons_count) {
			cond_wait(prod, mtx);
		}
	}
	unlock(mtx);
}

// Productores
int fifoproc_write(char* buff, int len) {

	char kbuffer[MAX_KBUF];
	
	if (len > MAX_CBUFFER_LEN || len > MAX_KBUF) { return Error;}
	if (copy_from_user(kbuffer, buff, len)) { return Error;}
	
	lock(mtx);
	
	/* Esperar hasta que haya hueco para insertar (debe haber consumidores) */
	while (kfifo_avail(&cbuffer) < len && cons_count > 0) {
		cond_wait(prod, mtx);
	}
	
	/* Detectar fin de comunicación por error (consumidor cierra FIFO antes) */
	if (cons_count == 0) {unlock(mtx); return -EPIPE;} // porque error en cons= 0 si se espera a q sea 0
	
	kfifo_in(&cbuffer, kbuffer, len);
	
	/* Despertar a posible consumidor bloqueado */
	cond_signal(cons);
	
	unlock(mtx);
	
	return len;

}

// Consumidores
int fifoproc_read(const char* buff, int len) {	

	char kbuffer[MAX_KBUF];

	if (len > MAX_CBUFFER_LEN || len > MAX_KBUF) { return Error;}
	if (copy_from_user(kbuffer, buff, len)) { return Error;}
	
	lock(mtx);
	
	/* Esperar hasta que haya datos que consumir (debe haber productores) */
	while (kfifo_len(&cbuffer) < len && prod_count > 0) {
		cond_wait(cons, mtx);
	}
	
	/* Detectar fin de comunicación por error (consumidor cierra FIFO antes) */
	if (prod_count == 0) {unlock(mtx); return -EPIPE;}
	
	kfifo_out(&cbuffer, kbuffer, len);
	
	if (copy_to_user(buff, kbuffer, len)) { return Error;}
	
	/* Despertar a posible productor bloqueado */
	cond_signal(prod);
	unlock(mtx);
	
	return len;

}
void fifoproc_release(bool lectura) {
	lock(mtx);
	if(lectura){
		if(kfifo_is_empty(&cbuffer))
			kfifo_reset();
		cons_cont--;
		cond_signal(prod);
	} else {
		if(kfifo_is_free(&cbuffer))
			kfifo_reset();
		prod_count--;
		cond_signal(cons);
	}}
	unlock(mtx);
}
	
