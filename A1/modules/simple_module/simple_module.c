
// REFERENCE: https://www.tldp.org/LDP/lkmpg/2.6/lkmpg.pdf

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

MODULE_DESCRIPTION("Simple module");
MODULE_AUTHOR("sidhuj23 and brarjas8");
MODULE_LICENSE("GPL");

int simple_init(void)
{
	// song: https://supersimple.com/song/hello/
	printk(KERN_INFO "Hello, hello, hello, how are you?\n");
	return 0;
}

void simple_exit(void)
{
	// song: https://supersimple.com/song/bye-bye-goodbye/
	printk(KERN_INFO "Bye bye. Goodbye.\n");
}

module_init(simple_init);
module_exit(simple_exit);

