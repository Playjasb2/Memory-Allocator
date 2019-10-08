
// REFERENCE: https://www.tldp.org/LDP/lkmpg/2.6/lkmpg.pdf
// REFERENCE: https://www.tldp.org/LDP/lkmpg/2.6/html/x323.html

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

MODULE_DESCRIPTION("Simple module");
MODULE_AUTHOR("sidhuj23 and brarjas8");
MODULE_LICENSE("GPL");

static int print_greeting = 0;
static char* message = "default message";

module_param(print_greeting, int, 0);
MODULE_PARM_DESC(print_greeting, "boolean value - prints greeting if non-zero");

module_param(message, charp, 0);
MODULE_PARM_DESC(message, "the message which will be printed");

int simple_init(void)
{
	// song: https://supersimple.com/song/hello/
	if(print_greeting != 0)
	{
		printk(KERN_INFO "Hello, hello, hello, how are you?\n");
	}
	printk(KERN_INFO "%s\n", message);
	return 0;
}

void simple_exit(void)
{
	// song: https://supersimple.com/song/bye-bye-goodbye/
	if(print_greeting != 0)
	{
		printk(KERN_INFO "Bye bye. Goodbye.\n");
	}
	printk(KERN_INFO "%s\n", message);
}

module_init(simple_init);
module_exit(simple_exit);

