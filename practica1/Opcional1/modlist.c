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

#define BUFFER_LENGTH (PAGE_SIZE / 4)

static struct proc_dir_entry *proc_entry;
static struct list_head the_list;

static int nr_elems = 0;    // numero de elementos de la lista

#ifdef PARTE_OPCIONAL
    static int nr_chars = 0;   // numero de caracteres de todas las cadenas de la lista

    struct list_item {
        struct list_head links;
        char *data;
    };


    // añade una cadena a la lista
    void add_to_the_list(char * str) {
        struct list_item *item = NULL; 
        item = vmalloc(sizeof(struct list_item));   // reserva memoria para struct list_item
        item->data = (char*) vmalloc(strlen(str));  // reserva espacio para la cadena
        item->data = str;                           // asignar dato
        list_add_tail(&item->links, &the_list);     // añadir al final de la lista

        nr_chars += strlen(str); // actualizar contadores
        nr_elems++;

        trace_printk("\n[m0dLiSt] add: added %s to the list\n%d elems, %d chars\n", str, nr_elems, nr_chars);
    }


    // borra cadenas iguales a str de la lista
    void remove_from_the_list(char *str) {
        struct list_item *item = NULL;
        struct list_head *cur_node = NULL;
        struct list_head *aux = NULL;
        int nr_dels = 0;

        list_for_each_safe(cur_node, aux, &the_list) {
            item = list_entry(cur_node, struct list_item, links);
            if (!strcmp(item->data, str)) {
                trace_printk("\nitemdata: %s, str %s\n", item->data, str); //TODO remov
                list_del(cur_node);  // borrar nodo de la lista
    			vfree(item->data);   // liberar memoria de la cadena
                vfree(item);         // liberar memoria del nodo
                nr_dels++;           // actualizar contadores
                nr_chars -= strlen(str); 
                nr_elems--;
            }
        }
        trace_printk("\n[m0dLiSt] remove: removed %d %s's from the list\n%d elems, %d chars\n", nr_dels, str, nr_elems, nr_chars);
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
                nr_dels++;                      // actualizar contadores
                nr_chars -= strlen(item->data);
                nr_elems--;
                list_del(cur_node);
                vfree(item->data);
                vfree(item);
            }
            trace_printk("\n[m0dLiSt] cleanup: %d items deleted\n%d elems, %d chars\n", nr_dels, nr_elems, nr_chars);
        }
    }

#else
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

#endif


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
    
	#ifdef PARTE_OPCIONAL
        char* str = vmalloc(len);
        char cmd[len]; 

        // parsear str
        int retval = sscanf(buffer, "%s %s", cmd, str);
        
        // modificar m0dLiSt
        if (retval == 1 && !strcmp(cmd, "cleanup")) {
            cleanup_the_list();
        } else if (retval == 2 && !strcmp(cmd, "add")) {
            add_to_the_list(str);
        } else if (retval == 2 && !strcmp(cmd, "remove")) {
            remove_from_the_list(str);
        } else {
            trace_printk("\n[m0dLiSt:ERROR] could not parse command\n");
        }

        vfree(str);
	#else
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

	#endif

    return len;
}


// imprime los contenidos de la lista (cat)
static ssize_t module_read(struct file *filp, char __user *buf, size_t len, loff_t *off) {
    int nr_bytes = 0;

    if ((*off) > 0)  // there is nothing left to read
        return 0;
    
    struct list_item *item = NULL;
    struct list_head *cur_node = NULL;


	#ifdef PARTE_OPCIONAL
        char buffer[nr_chars + nr_elems];   // cantidad de chars en la lista + un salto de linea por cada elem

        char* pnt = buffer;

        // copiar la lista a buffer
        list_for_each(cur_node, &the_list) {
            item = list_entry(cur_node, struct list_item, links);
            pnt += sprintf(pnt, "%s\n", item->data);
        }
	#else
        char buffer[nr_digits + nr_elems];  // cantidad de digitos en la lista + un salto de linea por cada elem
        char* pnt = buffer;
        // copiar la lista a buffer
        list_for_each(cur_node, &the_list) {
            item = list_entry(cur_node, struct list_item, links);
            pnt += sprintf(pnt, "%d\n", item->data);
        }
	#endif
   
    // restar punteros para hallar cantidad de bytes escritos a buffer
    nr_bytes = pnt - buffer;

    // asegurar que caben nr_bytes en buf
    if (len < nr_bytes)
        return -ENOSPC;

    if (list_empty(&the_list)) {
        trace_printk("\n[m0dLiSt] list empty\n");
    } else {
        trace_printk("\n[m0dLiSt] %d bytes in string(list):\n%s", nr_bytes, buffer);
    }

   
    // transfer data (elems de lista) from the kernel to userspace
    // copy_to_user(void __user* to, const void* from, unsigned long bytes_to_copy)
    if (copy_to_user(buf, buffer, nr_bytes))
        return -EINVAL;

    // update the file pointer
    (*off) += len;

    return nr_bytes;
}


static const struct file_operations proc_entry_fops = {
    .read     = module_read,
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
        #ifdef PARTE_OPCIONAL
            trace_printk("\n[m0dLiSt] parte opcional cargada\n");
        #else
            trace_printk("\n[m0dLiSt] parte obligatoria cargada\n");
        #endif
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
