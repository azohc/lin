#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/vmalloc.h>
#include <linux/uaccess.h>
#include <linux/list.h>
#include <linux/ftrace.h>
#include <linux/seq_file.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Pablo Jimenez Cruz & Juan Chozas Sumbera");

#define BUFFER_LENGTH (PAGE_SIZE / 4)

static struct proc_dir_entry *proc_entry;
static struct list_head the_list;

static int nr_elems = 0;    // numero de elementos de la lista
static int nr_digits = 0;   // numero de digitos de todos los elementos de la lista


struct list_item {
    struct list_head links;
    int data;
};


// dado un entero, devuelve cuantos digitos tiene
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
    item->data = n;                                                // asignar dato
    list_add_tail(&item->links, &the_list);                        // añadir al final de la lista

    nr_digits += digits(n); // actualizar contadores
    nr_elems++;

    trace_printk("\n[m0dLiSt] add: added %d to the list\n%d elems, %d digits\n", n, nr_elems, nr_digits);
}


// borra enteros iguales a n de la lista
void remove_from_the_list(int n) {
    struct list_item *item = NULL;
    struct list_head *cur_node = NULL;
    struct list_head *aux = NULL;
    int nr_dels = 0;

    list_for_each_safe(cur_node, aux, &the_list) {
        item = list_entry(cur_node, struct list_item, links);
        if (item->data == n) {
            list_del(cur_node);     // borrar nodo de la lista
            vfree(item);            // liberar memoria del nodo
            nr_dels++;              // actualizar contadores
            nr_digits -= digits(n); 
            nr_elems--;
        }
    }
    trace_printk("\n[m0dLiSt] remove: removed %d %d's from the list\n%d elems, %d digits\n", nr_dels, n, nr_elems, nr_digits);
}


// vacía la lista
void cleanup_the_list(void) {
    struct list_item *item = NULL;
    struct list_head *cur_node = NULL;
    struct list_head *aux = NULL;
    int nr_dels = 0;

    if (!list_empty(&the_list)) {
        list_for_each_safe(cur_node, aux, &the_list) {
            item = list_entry(cur_node, struct list_item, links);
            nr_dels++;
            nr_digits -= digits(item->data); // actualizar contadores
            nr_elems--;
            trace_printk("\n[m0dLiSt] cleanup: removed %d -%d digits\n", item->data, digits(item->data));
            list_del(cur_node);
            vfree(item);
        }

        trace_printk("\n[m0dLiSt] cleanup: %d items deleted\n%d elems, %d digits\n", nr_dels, nr_elems, nr_digits);
    }
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


// copia la lista a dest. devuelve un puntero a la
// primera posicion de memoria despues del último \n
char* write_list(char* dest) {
    char* pnt = dest;
    struct list_item *item = NULL;
    struct list_head *cur_node = NULL;

    // escribir elementos en dest
    list_for_each(cur_node, &the_list) {
        item = list_entry(cur_node, struct list_item, links);
        pnt += sprintf(pnt, "%d\n", item->data);
    }

    return pnt;
}


// devuelve el i-ésimo elemento de la lista
int list_get(loff_t* pos) {
    unsigned long* i = vmalloc(sizeof(int));    // reservar mem para i
    memcpy(i, pos, sizeof(int));                // copiar valor de pos a i

    if (!list_empty(&the_list)) {
        struct list_head* cur_node = NULL;
        struct list_item* item = NULL;

        list_for_each(cur_node, &the_list) {
            item = list_entry(cur_node, struct list_item, links);
            if (!(*i)) {
                vfree(i);
                return item->data;  
            } else {
                (*i)--;
            }
        }
    }

    return NULL;
}


// primera llamada de read/cat: inicio de la secuencia
static void *seq_start(struct seq_file *s, loff_t *pos) {
    int* v = vmalloc(sizeof(int));

    if (*pos == 0 && !list_empty(&the_list)) {  // secuencia nueva
        *v = list_get(pos);
        return v;                               // devolver primer elem para empezar read
    } else {                                    // final de secuencia
        *pos = 0;
        return NULL;                            // devolver NULL para no volver a mpezar
    }
}


// calcula el siguiente elemento de la secuencia
static void *seq_next(struct seq_file *s, void *v, loff_t *pos) {
    int* elem_siguiente = v;

    (*pos)++;

    if (*pos == nr_elems) { // fin de lista
        return NULL;        // parar la secuencia
    } else {
        if ((*elem_siguiente = list_get(pos)) == NULL) {
            trace_printk("\n[m0dLiSt:ERROR] list_get returned NULL\n");
            return NULL;
        }
    }
    return elem_siguiente;  // volver a llamar a next
}


// salida de la secuencia: de aqui se vuelve a seq_start
static void seq_stop(struct seq_file *s, void *v) { 
    vfree(v);
}


// imprime v
static int seq_show(struct seq_file *s, void *v) {
    int *elem = (int *) v;
    seq_printf(s, "%d\n", *elem);
    return 0;
}


static struct seq_operations ct_seq_ops = {
    .start = seq_start,
    .next  = seq_next,
    .stop  = seq_stop,
    .show  = seq_show
};


// registra ct_seq_ops y abre el fichero file
static int ct_open(struct inode *inode, struct file *file) {
    return seq_open(file, &ct_seq_ops);
};


static const struct file_operations proc_entry_fops = {
    .read   = seq_read,
    .open   = ct_open,
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
