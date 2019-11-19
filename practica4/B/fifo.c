#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kfifo.h>
#include <linux/semaphore.h>
#include <linux/errno.h>
#include <linux/uaccess.h>
#include <linux/proc_fs.h>
static struct proc_dir_entry *proc_entry;

#define MAX_FIFO_BUF 256


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
	
	if(down_interruptible(&mtx))
		return -1;
	
	/* Esperar hasta que haya hueco para insertar (debe haber consumidores) */
	while (kfifo_avail(&cbuffer) < len && cons_count > 0) {
		nr_prod_waiting++;
		up(&mtx);
		if(down_interruptible(&sem_prod)){
			down(&mtx);
			nr_prod_waiting--;
			up(&mtx);
			return -1;
		}
		if(down_interruptible(&mtx){
			nr_prod_waiting--;
			return -1;
		}
	}
	
	/* Detectar fin de comunicación por error (consumidor cierra FIFO antes) */
	if (cons_count == 0) {up(&mtx); return -EPIPE;} // porque error en cons= 0 si se espera a q sea 0
	
	kfifo_in(&cbuffer, kbuffer, len);
	
	/* Despertar a posible consumidor bloqueado */
	if(nr_cons_waiting > 0){
			up(&sem_cons);
			nr_cons_waiting--;
	}
	
	up(&mtx);
	
	return len;

}

// Consumidores
int fifoproc_read(const char* buff, int len) {	

	char kbuffer[MAX_KBUF];

	if (len > MAX_CBUFFER_LEN || len > MAX_KBUF) { return Error;}
	
	lock(mtx);
	
	/* Esperar hasta que haya datos que consumir (debe haber productores) */
	while (kfifo_len(&cbuffer) < len && prod_count > 0) {
		nr_cons_waiting++;
		up(&mtx);
		if(down_interruptible(&sem_cons)){
			down(&mtx);
			nr_cons_waiting--;
			up(&mtx);
			return -1;
		}
		if(down_interruptible(&mtx)){
			nr_cons_waiting--;
			return -1;
		}
		
	}
	
	/* Detectar fin de comunicación por error (consumidor cierra FIFO antes) */
	if (prod_count == 0 && kfifo_is_empty(&cbuffer)) {up(mtx); return -EPIPE;}
	
	kfifo_out(&cbuffer, kbuffer, len);
	
	/* Despertar a posible productor bloqueado */
	if( nr_prod_waiting > 0){
		up(&sem_prod);
		nr_prod_waiting--;
	
	if (copy_to_user(buff, kbuffer, len)) { return Error;}
	
	
	up(mtx);
	
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

static const file_operations proc_entry_fops = {
	.open = fifoproc_open,
	.write = fifoproc_write,
	.read = fifoproc_read,
	.release = fifoproc_release,
};

int init_fifo_module(void){
	
	proc_entry = proc_create_data("fifo", 0666, NULL, &proc_entry_fops, NULL);
	if(proc_entry==NULL) return -1;
	
	int aux = kfifo_alloc(&cbuffer, MAX_FIFO_BUF*sizeof(int), GFP_KERNEL);
	
	sema_init(&mtx, 1);  //inicializamos el semaforo a 1 para usarlo como un mutex
	
	sema_init(&sem_cons, 0);
	sema_init(&sem_prod, 0);
	
	nr_prod_waiting = 0;
	nr_cons_waiting = 0;
	cons_count = 0;
	prod_count = 0;
	
	return aux;
}

int exit_fifo_module(void){
	kfifo_reset(&cbuffer);
	kfifo_free(&cbuffer);
	remove_proc_entry("fifo", NULL);
	return 0;
}

module_init(init_fifo_module);
module_exit(exit_fifo_module);

	
	
	
	
	
