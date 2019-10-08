
// REFERENCE: https://www.tldp.org/LDP/lkmpg/2.6/lkmpg.pdf

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

MODULE_DESCRIPTION("Bad module");
MODULE_AUTHOR("sidhuj23 and brarjas8");
MODULE_LICENSE("GPL");

int bad_init(void)
{
	printk(KERN_INFO "Bad module initializing\n");
	*((unsigned long*)(NULL)) = 0xFFFFFFFFFFFFFFFF;
	return 0;
}

void bad_exit(void)
{
	printk(KERN_INFO "Bad module exiting\n");
}

module_init(bad_init);
module_exit(bad_exit);

