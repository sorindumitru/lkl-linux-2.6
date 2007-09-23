#include <linux/slab.h>
#include <linux/types.h>
#include <linux/syscalls.h>

/*
 * sys_mount (is sloppy?? and) copies a full page from dev_name, type and
 * data. This can trigger page faults (which a normal trigger would
 * ignore). 
 */
asmlinkage long sys_safe_mount(char __user *dev_name, char __user *dir_name,
				char __user *type, unsigned long flags,
				void __user *data)
{
	int err;
	unsigned long _dev_name=0, _type=0, _data=0;

	err=-ENOMEM;
	if (dev_name) {
		_dev_name=__get_free_page(GFP_KERNEL);
		if (!_dev_name)
			goto out_free;
		strcpy((char*)_dev_name, dev_name);
	}

	if (type) {
		_type=__get_free_page(GFP_KERNEL);
		if (!_type)
			goto out_free;
		strcpy((char*)_type, type);
	}

	if (data) {
		_data=__get_free_page(GFP_KERNEL);
		if (!_data)
			goto out_free;
		strcpy((char*)_data, data);
	}

	err=sys_mount((char __user*)_dev_name, dir_name, (char __user*)_type,
		      flags, (char __user*)_data);

out_free:
	if (_dev_name)
		free_page(_dev_name);
	if (_type)
		free_page(_type);
	if (_data)
		free_page(_data);

	return err;
}
