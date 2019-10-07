#include <linux/syscalls.h>
#include <linux/kernel.h>
#include <linux/tty.h>      /* For fg_console */
#include <linux/kd.h>       /* For KDSETLED */
#include <linux/vt_kern.h>


#define ALL_LEDS_ON 0x7
#define ALL_LEDS_OFF 0


/* Get driver handler */
struct tty_driver* get_kbd_driver_handler(void) {
	printk(KERN_INFO "\nledctl: loading . . .\n");
	printk(KERN_INFO "\nledctl: fgconsole is %x\n", fg_console);
	return vc_cons[fg_console].d->port.tty->driver;
}


/* Set led state to that specified by mask */
static inline int set_leds(struct tty_driver* handler, unsigned int mask) {
	return (handler->ops->ioctl) (vc_cons[fg_console].d->port.tty, KDSETLED, mask);
}


SYSCALL_DEFINE1(ledctl, unsigned int, leds) {

	tty_driver* kbd_driver = get_kbd_driver_handler();

	set_leds(kbd_driver, leds);

	return 0;
}