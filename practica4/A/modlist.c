#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/vmalloc.h>
#include <linux/uaccess.h>
#include <linux/list.h>
#include <linux/ftrace.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Pablo Jimenez Cruz & Juan Chozas Sumbera");

DEFINE_SPINLOCK(sp);

#define BUFFER_LENGTH (PAGE_SIZE / 4)
#define MAX_BUFF_SIZE 257


static struct proc_dir_entry *proc_entry;
static struct list_head the_list;

struct list_item {
    struct list_head links;
    int data;
};

int digits(int n) {

    int d = 1;

    if (n < 10) {
        return 1;
    } else {
        while (n >= 10) {
            n = n / 10;
            d++;
        }
    }

    return d;
}


// añade un entero a la lista
void add_to_the_list(int n) {
    struct list_item *item = vmalloc(sizeof(struct list_item));    // reserva memoria para struct list_item
    item->data = n;
        
    spin_lock(&sp);                                            // asignar dato
    list_add_tail(&item->links, &the_list);                        // añadir al final de la lista
    spin_unlock(&sp);
    trace_printk("\n[m0dLiSt] add: added %d to the list\n", n);
}


// borra enteros iguales a n de la lista
void remove_from_the_list(int n) {
    struct list_item *item = NULL;
    struct list_head *cur_node = NULL;
    struct list_head *aux = NULL;
    int nr_dels = 0;

    spin_lock(&sp);

    list_for_each_safe(cur_node, aux, &the_list) {
        item = list_entry(cur_node, struct list_item, links);
        if (item->data == n) {
            nr_dels++;
            list_del(cur_node);     // borrar nodo de la lista
            vfree(item);            // liberar memoria del nodo
        }
    }

    spin_unlock(&sp);
    trace_printk("\n[m0dLiSt] remove: removed %d %d's from the list\n", nr_dels, n);
}



// vacía la lista
void cleanup_the_list(void) {
    struct list_item *item = NULL;
    struct list_head *cur_node = NULL;
    struct list_head *aux = NULL;
    int nr_dels = 0;

    spin_lock(&sp);

    list_for_each_safe(cur_node, aux, &the_list) {
        item = list_entry(cur_node, struct list_item, links);
        nr_dels++;
        trace_printk("\n[m0dLiSt] cleanup: removed %d -%d digits\n", item->data, digits(item->data));
        list_del(cur_node);
        vfree(item);
    }

    trace_printk("\n[m0dLiSt] cleanup: %d items deleted\n", nr_dels);


     spin_unlock(&sp);
}


// copia la lista a dest. devuelve un puntero a la
// primera posicion de memoria despues del último \n
char* write_list(char* dest) {
    char* pnt = dest;
    struct list_item *item = NULL;
    struct list_head *cur_node = NULL;
    int nr_byter = 0;

    // escribir elementos en dest
    list_for_each(cur_node, &the_list) {
        item = list_entry(cur_node, struct list_item, links);
        nr_byter += digits(item->data) + 1;
        if (nr_byter <= MAX_BUFF_SIZE)
            pnt += sprintf(pnt, "%d\n", item->data);
        else
            return NULL;
    }
    return pnt;
}


// modifica la lista con add, remove o cleanup
static ssize_t module_write(struct file *filp, const char __user *buf, size_t len, loff_t *off) {
    int available_space = BUFFER_LENGTH - 1;
    char buffer[BUFFER_LENGTH];

    if ((*off) > 0) // the application can write in this entry just once !!
        return 0;

    if (len > available_space) {
        trace_printk("\n[m0dLiSt:ERROR] not enough space for writing\n");
        return -ENOSPC;
    }

    // transfer data from user to kernel space
    // copy_from_user(void* to, const void __user* from, unsigned long bytes_to_copy)
    if (copy_from_user(buffer, buf, len))
        return -EFAULT;

    buffer[len] = '\0'; // Add the `\0' to convert to string
    *off += len;

    int num;
    char cmd[len];

    // parsear str 
    int retval = sscanf(buffer, "%s %d", cmd, &num);

    

    // modificar m0dLiSt
    if (retval == 1 && !strcmp(cmd, "cleanup")) {
        cleanup_the_list();
    } else if (retval == 2 && !strcmp(cmd, "add")) {
        add_to_the_list(num);
    } else if (retval == 2 && !strcmp(cmd, "remove")) {
        remove_from_the_list(num);
    } else {
        trace_printk("\n[m0dLiSt:ERROR] could not parse command\n");
    }


    
    return len;
}


// imprime los contenidos de la lista
static ssize_t module_read(struct file *filp, char __user *buf, size_t len, loff_t *off) {
    int nr_bytes = 0;

    if ((*off) > 0) // there is nothing left to read
        return 0;

    // cantidad de digitos en la lista + un salto de linea por cada elem
    char buffer[MAX_BUFF_SIZE]; 

    spin_lock(&sp);
    // copiar lista al buffer
    char *pnt = write_list(buffer);
    if (pnt == NULL){
        return -ENOSPC;
    }

    spin_unlock(&sp);


    // restar punteros para hallar cantidad de bytes escritos a buffer
    nr_bytes = pnt - buffer;

    // asegurar que caben nr_bytes en buf
    if (len < nr_bytes)
        return -ENOSPC;

    spin_lock(&sp);

    if (list_empty(&the_list)) {
        trace_printk("\n[m0dLiSt] list empty\n");
    } else {
        trace_printk("\n[m0dLiSt] %d bytes in string(list):\n%s", nr_bytes, buffer);
    }

    // transfer data (elems de lista) from the kernel to userspace
    // copy_to_user(void __user* to, const void* from, unsigned long bytes_to_copy)
    if (copy_to_user(buf, buffer, nr_bytes))
        return -EINVAL;

    spin_unlock(&sp);

    // update the file pointer
    (*off) += len;

    return nr_bytes;
}


static const struct file_operations proc_entry_fops = {
    .read   = module_read,
    .write  = module_write,
};


// inicializa el módulo
int init_modlist( void ) {
    int ret = 0;

    // initialize the list head
    INIT_LIST_HEAD(&the_list);

    // create /proc/m0dLiSt to interact with the list
    proc_entry = proc_create( "m0dLiSt", 0666, NULL, &proc_entry_fops);
    if (proc_entry == NULL) {
        ret = -ENOMEM;
        trace_printk("\n[m0dLiSt:ERROR] couldn't create entry in /proc\n");
    } else {
        trace_printk("\n[m0dLiSt] loaded\n");
    }

    return ret;
}


// destruye el módulo
void exit_modlist( void ) {
    // borrar /proc/m0dLiSt
    remove_proc_entry("m0dLiSt", NULL);
    // liberar memoria si lista no está vacía
    cleanup_the_list();
    trace_printk("\n[m0dLiSt]: unloaded\n");
}

module_init(init_modlist);
module_exit(exit_modlist);
