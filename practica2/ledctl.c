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

	// crear nueva mascara: intercambiar  valores de bits 0 y 2
	// 0x3 = 011 => 110 = 0x6
	// 0x1 = 001 => 100 = 0x4
	// 0x2 = 010 => 010 = 0x2

	int nleds;

	if (leds == 0x1) {
		nleds = 0x4;
	} else if (leds == 0x3) {
		nleds = 0x6;
	} else if (leds == 0x4) {
		nleds = 0x1;
	} else if (leds == 0x6) {
		nleds = 0x3;
	} else if (leds >= 0x0 && leds <= 0x7) {
		nleds = leds;
	} else {
		printk("Error... aborting...");
		return -1;
	}


	return set_leds(kbd_driver, nleds);
}