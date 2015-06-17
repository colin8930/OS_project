#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <asm/current.h>
#include <asm/uaccess.h>

MODULE_LICENSE("GPL");
#define DRIVER_NAME "devone"

static int devone_devs = 1;
static int devone_major;
static int devone_minor;
static struct cdev devone_cdev;
static struct class *devone_class;
static dev_t devone_dev;

struct devone_data {
	unsigned char val;
	rwlock_t lock;
};

ssize_t devone_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
	struct devone_data *p = filp->private_data;
	unsigned char val;
	int retval = 0;
	printk(KERN_INFO "%s: count %zd pos %lld\n", __func__, count, *f_pos);
	if (count >= 1) {
		if (copy_from_user(&val, &buf[0], 1)) {
			retval = -EFAULT;
			goto out;
		}
		write_lock(&p->lock);
		p->val = val;
		write_unlock(&p->lock);
		retval = count;
	}
	out:
	return retval;
}

ssize_t devone_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	struct devone_data *p = filp->private_data;
	int i;
	unsigned char val;
	int retval;
	read_lock(&p->lock);
	val = p->val;
	read_unlock(&p->lock);
	printk(KERN_INFO "%s: count %zd pos %lld\n", __func__, count, *f_pos);
		for (i = 0; i < count; i++) {
			if (copy_to_user(&buf[i], &val, 1)) {
			retval = -EFAULT;
			goto out;
		}
	}
	retval = count;
	out:
	return retval;
}
static int devone_open(struct inode *inode, struct file *file)
{
	struct devone_data *p;
	printk(KERN_INFO "%s: major %d minor %d (pid %d)\n", __func__, imajor(inode), iminor(inode), current->pid);
	p = kmalloc(sizeof(struct devone_data), GFP_KERNEL);
	if (!p) {
		printk(KERN_INFO "%s: Not memory\n", __func__);
		return -ENOMEM;
	}
	p->val = 0xff;
	rwlock_init(&p->lock);
	file->private_data = p;
	return 0;
}

static int devone_close(struct inode *inode, struct file *file)
{
	printk(KERN_INFO "%s: major %d minor %d (pid %d)\n", __func__, imajor(inode), iminor(inode), current->pid);
	kfree(file->private_data);
	file->private_data = NULL;
	return 0;
}

const struct file_operations devone_fops = {
	.open = devone_open,
	.release = devone_close,
	.read = devone_read,
	.write = devone_write,
};

static int devone_init(void)
{
	dev_t dev;
	int major;
	int alloc_ret = 1;
	int cdev_err = 1;
	alloc_ret = alloc_chrdev_region(&dev, 0, devone_devs, DRIVER_NAME);
	if (alloc_ret)
		goto error;
	devone_major = major = MAJOR(dev);
	cdev_init(&devone_cdev, &devone_fops);
	devone_cdev.owner = THIS_MODULE;
	cdev_err = cdev_add(&devone_cdev, MKDEV(devone_major, 0), devone_devs);
	if (cdev_err)
	goto error;
	/* register class */
	devone_class = class_create(THIS_MODULE, DRIVER_NAME);
	if (IS_ERR(devone_class))
	goto error;
	devone_dev = MKDEV(devone_major, devone_minor);
	device_create(devone_class, NULL, devone_dev, NULL, "devone%d", devone_minor);
	printk(KERN_INFO "%s driver(major %d) installed.\n", DRIVER_NAME, major);
	return 0;

error:
	if (!cdev_err)
	cdev_del(&devone_cdev);
	if (!alloc_ret)
	unregister_chrdev_region(dev, devone_devs);
	return -1;
}

static void devone_exit(void)
{
	dev_t dev = MKDEV(devone_major, 0);
	/* unregister class */
	device_destroy(devone_class, devone_dev);
	class_destroy(devone_class);
	cdev_del(&devone_cdev);
	unregister_chrdev_region(dev, devone_devs);
	printk(KERN_INFO "%s driver removed.\n", DRIVER_NAME);
}

module_init(devone_init);
module_exit(devone_exit);
