#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <asm/current.h>
#include <asm/uaccess.h>

MODULE_LICENSE("GPL");

struct data {
	char val;
	rwlock_t lock;
};

#ifdef STATIC
#else
static int dev_major;
static int dev_minor;
static struct cdev dev_cdev;
static struct class *dev_class;
static dev_t dev_dev;
static int devone_devs = 1;
#endif
static ssize_t driver_read(struct file *filp, char *buf, size_t count, loff_t *ppos)
{
	struct data *p = filp -> private_data;
	int i;
	char val;

	read_lock(&p->lock);
	val = p->val;
	read_unlock(&p->lock);

	printk("device read\n");
	printk("read: %c \n", val);
	for(i = 0; i < count; i++)
		if(copy_to_user(&buf[i], &val, 1)) {
			return -EFAULT;
		}
	return count;
}
static ssize_t driver_write(struct file *filp, const char *buf, size_t count, loff_t *ppos)
{
	struct data *p = filp -> private_data;
	char val;
	printk("device write\n");

	if(count >= 1){
		if (copy_from_user(&val, &buf[0], 1)) {
			return -EFAULT;
		}
		write_lock(&p->lock);
		p->val = val;
		write_unlock(&p->lock);
	}
	printk("write: %c \n", p -> val);
	return count;
}
static int driver_open(struct inode *inode, struct file *filp)
{
	struct data *p;

	p = kmalloc(sizeof(struct data), GFP_KERNEL);
	if (!p) {
		printk(KERN_INFO "%s: Not memory\n", __func__);
		return -ENOMEM;
	}
	p->val = (char) 'm';
	rwlock_init(&p->lock);
	filp->private_data = p;
	printk("device open\n");
	return 0;
}

static int driver_release(struct inode *inode, struct file *filp)
{
	kfree(filp->private_data);
	filp->private_data = NULL;
	printk("device close\n");
	return 0;
}
struct file_operations driver_fops =
{
	read: driver_read,
	write: driver_write,
	open: driver_open,
	release: driver_release,
};
#define MAJOR_NUM 70
#define MODULE_NAME "test"

#ifdef STATIC
static int test_init(void)
{
	if (register_chrdev(MAJOR_NUM, "test", &driver_fops)< 0) {
		printk("%s driver can't get major %d\n", MODULE_NAME, MAJOR_NUM);
		return (-EBUSY);
	}
	printk("%s driver started\n", MODULE_NAME);
	return 0;
}

static void test_exit(void)
{
	unregister_chrdev(MAJOR_NUM, "test");
	printk("%s driver removed\n", MODULE_NAME);
}
#else
static int test_init(void)
{
	dev_t dev;
	int major;
	if(alloc_chrdev_region(&dev, 0, devone_devs, "test")){
		unregister_chrdev_region(dev, devone_devs);
		printk("%s driver can't get major\n", MODULE_NAME);
		return (-EBUSY);
	}
	dev_major = major = MAJOR(dev);
	cdev_init(&dev_cdev, &driver_fops);
	dev_cdev.owner = THIS_MODULE;
	if(cdev_add(&dev_cdev, MKDEV(dev_major, 0), devone_devs)){
		cdev_del(&dev_cdev);
		printk("add error\n");
		return (-EBUSY);
	}
	printk("%s is installed, major number is %d.\n", MODULE_NAME,major);
	return 0;
}

static void test_exit(void)
{
	dev_t dev = MKDEV(dev_major, 0);
	cdev_del(&dev_cdev);
	unregister_chrdev_region(dev, 1);
	printk(KERN_INFO "%s driver removed.\n", MODULE_NAME);
}

#endif

module_init(test_init);
module_exit(test_exit);
