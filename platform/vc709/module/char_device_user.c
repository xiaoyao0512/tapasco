//
// Copyright (C) 2014 David de la Chevallerie, TU Darmstadt
//
// This file is part of Tapasco (TPC).
//
// Tapasco is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Tapasco is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with Tapasco.  If not, see <http://www.gnu.org/licenses/>.
//
/**
 * @file char_device_user.c
 * @brief Implementation of char-device calls for user-specific calls
	the char-device allows access to the user-registers over pcie
	besides it handles blocking execution of hw-function
	irqs are managed here as well, this code expects a Xilinx IRQ IP-Core as the source the irq
	each minor node handles one IRQ-Core, multiple minor node will be available
 * */

/******************************************************************************/

#include "char_device_user.h"

/******************************************************************************/
/* global struct and variable declarations */

/* file operations for sw-calls as a char device */
static struct file_operations user_fops = {
	.owner          = THIS_MODULE,
	.open           = user_open,
	.release        = user_close,
	.unlocked_ioctl = user_ioctl,
	.read           = user_read,
	.write          = user_write,
	.mmap		= user_mmap
};

/* struct array to hold data over multiple fops-calls */
static struct priv_data_struct priv_data;

/* char device structure basically for dev_t and fops */
static struct cdev char_user_cdev;
/* char device number for f3_char_driver major and minor number */
static dev_t char_user_dev_t;
/* device class entry for sysfs */
struct class *char_user_class;

static int device_opened = 0;

static DEFINE_SPINLOCK(device_open_close_mutex);

/******************************************************************************/
/* functions for user-space interaction */

/**
 * @brief When minor node is opened, mutexes and waiting queues will be initialized,
	performs setup code to activate interrupt controller
	caution: when irq-core is not present, this will cause a deadlock
 * @param inode Representation of node in /dev, used to get major-/minor-number
 * @param filp Mostly used to allocate private data for consecutive calls
 * @return Zero, if char-device could be opened, error code otherwise
 * */
static int user_open(struct inode *inode, struct file *filp)
{
	spin_lock(&device_open_close_mutex);

	fflink_notice("Already %d files in use.\n", device_opened);

	++device_opened;
	/* set filp for further sys calls to this minor number */
	filp->private_data = &priv_data;
	spin_unlock(&device_open_close_mutex);
	return 0;
}

/**
 * @brief Tidy up device, nothing to do here currently
 * @param inode Representation of node in /dev, used to get major-/minor-number
 * @param filp Mostly used to allocate private data for consecutive calls
 * @return Zero, if char-device could be opened, error code otherwise
 * */
static int user_close(struct inode *inode, struct file *filp)
{
	spin_lock(&device_open_close_mutex);
	--device_opened;
	fflink_notice("Still %d files in use.\n", device_opened);
	spin_unlock(&device_open_close_mutex);
	return 0;
}

/******************************************************************************/
/* functions for user-space interaction */

/**
 * @brief Read out consecutive register of user-specific function over pcie
	workaroung: using specific struct (user_ioctl_calls) to pass needed parameters
 * @param filp Needed to identify device node and get access to corresponding buffers
 * @param buf Pointer to user space buffer
 * @param count Bytes to be transferred
 * @param f_pos Offset in file, currently not supported
 * @return Zero, if transfer was successful, error code otherwise
 * */
static ssize_t user_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	struct user_rw_params params;
	uint32_t i, err = 0, static_buffer[STATIC_BUFFER_SIZE], *copy_buffer;
	bool use_dynamic = false;
	fflink_notice("Called for device minor %d\n", ((struct priv_data_struct *) filp->private_data)->minor);

	copy_buffer = static_buffer;

	if(count != sizeof(struct user_rw_params)) {
		fflink_warn("Wrong size to parse parameters accordingly %ld vs %ld\n", count, sizeof(struct user_rw_params));
		return -EACCES;
	}
	if(copy_from_user(&params, buf, count)) {
		fflink_warn("Couldn't copy all bytes from user-space to parse parameters\n");
		return -EACCES;
	}

	if(params.btt > STATIC_BUFFER_SIZE*REGISTER_BYTE_SIZE) {
		fflink_info("Allocating %d bytes dynamically - only %d available statically\n", params.btt, STATIC_BUFFER_SIZE*REGISTER_BYTE_SIZE);

		copy_buffer = kmalloc(params.btt, GFP_KERNEL);
		if(!copy_buffer) {
			fflink_warn("Couldn't allocate dynamic buffer for transfer\n");
			return -EACCES;
		}

		use_dynamic = true;
	}

	fflink_info("Copy %d bytes from address %llX to address %llX\n", params.btt, params.fpga_addr, params.host_addr);
	for(i = 0; i < params.btt/4; i++)
		copy_buffer[i] = pcie_readl((void*) (params.fpga_addr + i*4));

	if(copy_to_user((void *)params.host_addr, copy_buffer, params.btt)) {
		fflink_warn("Couldn't copy all bytes to user-space\n");
		err = -EACCES;
	}

	if(unlikely(use_dynamic)) {
		fflink_info("Freeing dynamic buffer\n");
		kfree(copy_buffer);
	}

	return err;
}

/**
 * @brief Write to consecutive register of user-specific function over pcie
	workaroung: using specific struct (user_ioctl_calls) to pass needed parameters
 * @param filp Needed to identify device node and get access to corresponding buffers
 * @param buf Pointer to user space buffer
 * @param count Bytes to be transferred
 * @param f_pos Offset in file, currently not supported
 * @return Zero, if transfer was successful, error code otherwise
 * */
static ssize_t user_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
	struct user_rw_params params;
	uint32_t i, err = 0, static_buffer[STATIC_BUFFER_SIZE], *copy_buffer;
	bool use_dynamic = false;
	fflink_notice("Called for device minor %d\n", ((struct priv_data_struct *) filp->private_data)->minor);

	copy_buffer = static_buffer;

	if(count != sizeof(struct user_rw_params)) {
		fflink_warn("Wrong size to parse parameters accordingly %ld vs %ld\n", count, sizeof(struct user_rw_params));
		return -EACCES;
	}
	if(copy_from_user(&params, buf, count)) {
		fflink_warn("Couldn't copy all bytes from user-space to parse parameters\n");
		return -EACCES;
	}

	if(params.btt > STATIC_BUFFER_SIZE*REGISTER_BYTE_SIZE) {
		fflink_info("Allocating %d bytes dynamically - only %d available statically\n", params.btt, STATIC_BUFFER_SIZE*REGISTER_BYTE_SIZE);

		copy_buffer = kmalloc(params.btt, GFP_KERNEL);
		if(!copy_buffer) {
			fflink_warn("Couldn't allocate dynamic buffer for transfer\n");
			return -EACCES;
		}

		use_dynamic = true;
	}

	if(copy_from_user(copy_buffer, (void*)params.host_addr, params.btt)) {
		fflink_warn("Couldn't copy all bytes from user-space\n");
		err = -EACCES;
		goto USER_WRITE_CLEANUP;
	}

	fflink_info("Copy %d bytes to address %llX from address %llX\n", params.btt, params.fpga_addr, params.host_addr);
	for(i = 0; i < params.btt/4; i++)
		pcie_writel(copy_buffer[i], (void*) (params.fpga_addr + i*4));

USER_WRITE_CLEANUP:
	if(unlikely(use_dynamic)) {
		fflink_info("Freeing dynamic buffer\n");
		kfree(copy_buffer);
	}

	return err;
}


/******************************************************************************/
/* function for user-space interaction */

/**
 * @brief User space communication to trigger hw-function to run and wait for its interrupt
	identification of function is done with event field of the struct user_ioctl_params
 * @param filp Needed to identify device node and get access to corresponding buffers
 * @param ioctl_num magic number from ioctl_calls header
 * @param ioctl_param pointer to arguments from user corresponding to provided commands
 * @return Zero, if command could be executed successfully, otherwise error code
 * */
static long user_ioctl(struct file *filp, unsigned int ioctl_num, unsigned long ioctl_param)
{
	int irq_counter;
	struct priv_data_struct * p = (struct priv_data_struct *) filp->private_data;
	struct user_ioctl_params params;
	fflink_notice("Called for device minor %d\n", p->minor);

	if(_IOC_SIZE(ioctl_num) != sizeof(struct user_ioctl_params)) {
		fflink_warn("Wrong size to read out registers %d vs %ld\n", _IOC_SIZE(ioctl_num), sizeof(struct user_ioctl_params));
		return -EACCES;
	}
	if(copy_from_user(&params, (void *)ioctl_param, _IOC_SIZE(ioctl_num))) {
		fflink_warn("Couldn't copy all bytes\n");
		return -EACCES;
	}

	switch(ioctl_num) {
		case IOCTL_CMD_USER_WAIT_EVENT:
			fflink_info("IOCTL_CMD_USER_WAIT_EVENT with Param-Size: %d byte\n", _IOC_SIZE(ioctl_num));
			fflink_info("Want to write %X to address %llX with event %d\n", params.data, params.fpga_addr, params.event);

			irq_counter = priv_data.user_condition[params.event];

			pcie_writel(params.data, (void*) params.fpga_addr);

			if(wait_event_interruptible(p->user_wait_queue[params.event], (irq_counter != p->user_condition[params.event]))) {
				fflink_warn("got killed while hanging in waiting queue\n");
				break;
			}

			break;
		default:
			fflink_warn("default case - nothing to do here\n");
			break;
	}

	return 0;
}

/******************************************************************************/
/* function for user-space interaction */

/**
 * @brief not implemented yet - could map register to user-space directly here
	but not recommended, cause address space will not be filled completely rather than sparse
	performing a transaction on the 'wholes' will cause deadlocks immediatly
 * @param filp Needed to identify device node and get access to corresponding buffers
 * @param vma Struct of virtual memory representation, will be modified to allow user-space access
 * @return Zero, if memory could be mapped, error code otherwise
 * */
static int user_mmap(struct file *filp, struct vm_area_struct *vma)
{
	fflink_notice("Called for device minor %d\n", ((struct priv_data_struct *) filp->private_data)->minor);

	return 0;
}

/******************************************************************************/
/* functions for irq-handling */

/**
 * @brief Interrupt handler for thread pools
	wakes up corresponding process, which waits for the function to finish
	only work, when function was activated over ioctl
 * @param irq Interrupt number of calling line
 * @param dev_id magic number for interrupt sharing (not needed)
 * @return Tells OS, that irq is handled properly
 * */
irqreturn_t intr_handler_user(int irq, void * dev_id)
{
	int irq_translated = pcie_translate_irq_number(irq) - 4;
	if(irq_translated != -1) {
		fflink_info("User interrupt for IRQ %d called with irq %d\n", irq_translated, irq);
		priv_data.user_condition[irq_translated] += 1;
		wake_up_interruptible_sync(&priv_data.user_wait_queue[irq_translated]);
	} else {
		fflink_info("Illegal interrupt %d\n", irq);
	}
	return IRQ_HANDLED;
}

/******************************************************************************/
/* helper functions externally called e.g. to (un/)load this char device */

/**
 * @brief Registers char device with multiple minor nodes in /dev
 * @param none
 * @return Returns error code or zero if success
 * */
int char_user_register(void)
{
	int err = 0, hw_id = 0, i;
	struct device *device = NULL;

	fflink_info("Try to add char_device to /dev\n");

	/* create device class to register under sysfs */
	err = alloc_chrdev_region(&char_user_dev_t, 0, FFLINK_USER_NODES, FFLINK_USER_NAME);
	if (err < 0 || MINOR(char_user_dev_t) != 0) {
		fflink_warn("failed to allocate chrdev with %d minors\n", FFLINK_USER_NODES);
		goto error_no_device;
	}

	/* create device class to register under udev/sysfs */
	if (IS_ERR((char_user_class = class_create(THIS_MODULE, FFLINK_USER_NAME)))) {
		fflink_warn("failed to create class\n");
		goto error_class_invalid;
	}

	/* initialize char dev with fops to prepare for adding */
	cdev_init(&char_user_cdev, &user_fops);
	char_user_cdev.owner = THIS_MODULE;

	/* try to add char dev */
	err = cdev_add(&char_user_cdev, char_user_dev_t, FFLINK_USER_NODES);
	if (err) {
		fflink_warn("failed to add char dev\n");
		goto error_add_to_system;
	}

	for(i = 0; i < FFLINK_USER_NODES; i++) {
		/* create device file via udev */
		device = device_create(char_user_class, NULL, MKDEV(MAJOR(char_user_dev_t), MINOR(char_user_dev_t)+i), NULL, FFLINK_USER_NAME "_%d", MINOR(char_user_dev_t)+i);
		if (IS_ERR(device)) {
			err = PTR_ERR(device);
			fflink_warn("failed while device create %d\n", MINOR(char_user_dev_t));
			goto error_device_create;
		}
	}

	fflink_notice("Called for device (<%d,%d>)\n", imajor(inode), iminor(inode));

	/* check if ID core is readable */
	hw_id = pcie_readl((void*) HW_ID_ADDR);
	if(hw_id != HW_ID_MAGIC) {
		fflink_warn("ID Core not found (was %X - should: %X)\n", hw_id, HW_ID_MAGIC);
		return -ENOTEMPTY;
	}

	/* init control structures for synchron sys-calls */
	for(i = 0; i < PE_IRQS; ++i) {
		init_waitqueue_head(&priv_data.user_wait_queue[i]);
		priv_data.user_condition[i] = 0;
	}

	return 0;

	/* tidy up for everything successfully allocated */
error_device_create:
	for(i = i - 1; i >= 0; i--) {
		device_destroy(char_user_class, MKDEV(MAJOR(char_user_dev_t), MINOR(char_user_dev_t)+i));
	}
	cdev_del(&char_user_cdev);
error_add_to_system:
	class_destroy(char_user_class);
error_class_invalid:
	unregister_chrdev_region(char_user_dev_t, FFLINK_USER_NODES);
error_no_device:
	return -ENODEV;
}

/**
 * @brief Unregisters char device, which was initialized with user_register before
 * @param none
 * @return none
 * */
void char_user_unregister(void)
{
	int i;

	fflink_info("Tidy up\n");

	for(i = 0; i < FFLINK_USER_NODES; i++) {
		device_destroy(char_user_class, MKDEV(MAJOR(char_user_dev_t), MINOR(char_user_dev_t)+i));
	}

	cdev_del(&char_user_cdev);

	class_destroy(char_user_class);

	unregister_chrdev_region(char_user_dev_t, FFLINK_USER_NODES);
}

/******************************************************************************/
