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

	struct tty_driver* kbd_driver = get_kbd_driver_handler();

	// crear nueva mascara: intercambiar  valores de bits 1 y 2
	// 0x3 = 011 => 101 = 0x5
	// 0x1 = 001 => 001 = 0x1
	// 0x2 = 010 => 100 = 0x4

	int x0 = leds & 0x1;
	int x1 = (leds >> 1) & 0x1;
	int x2 = (leds >> 2) & 0x1;

	int nleds = (x2 << 2) | (x1 << 1) | x0;


	return set_leds(kbd_driver, nleds);
}