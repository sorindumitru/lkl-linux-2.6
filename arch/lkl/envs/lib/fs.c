#include <asm/env.h>
#include <asm/callbacks.h>
#include <asm/page.h>
#include <asm/errno.h>
#include <linux/fs.h>

#include <asm/env.h>

extern int get_filesystem_list(char * buf);

static void get_fs_names(char *page)
{
	char *s = page;
	int len = get_filesystem_list(page);
	char *p, *next;
	
	page[len] = '\0';
	for (p = page-1; p; p = next) {
		next = strchr(++p, '\n');
		if (*p++ != '\t')
			continue;
		while ((*s++ = *p++) != '\n')
			;
		s[-1] = '\0';
	}
	
	*s = '\0';
}

long lkl_mount(char *dev, char *mnt, int flags, void *data)
{
	int err;
	char *p, *fs_names=(char*)linux_nops->mem_alloc(PAGE_SIZE);
	
	get_fs_names(fs_names);
retry:
	for (p = fs_names; *p; p += strlen(p)+1) {
		err = lkl_sys_mount(dev, mnt, p, flags, data);
		switch (err) {
			case 0:
				goto out;
			case -EACCES:
				flags |= MS_RDONLY;
				goto retry;
			case -EINVAL:
				continue;
		}
	}
out:
	linux_nops->mem_free(fs_names);

	return err;
}

long lkl_mount_dev(__kernel_dev_t dev, char *fs_type, int flags,
		   void *data, char *mnt_str, int mnt_str_len)
{
	char dev_str[] = { "/dev/xxxxxxxxxxxxxxxx" };
	int err;

	if (mnt_str_len < sizeof("/mnt/xxxxxxxxxxxxxxxx"))
		return -ENOMEM;

	snprintf(dev_str, sizeof(dev_str), "/dev/%016x", dev);
	snprintf(mnt_str, mnt_str_len, "/mnt/%016x", dev);

	err=lkl_sys_mknod(dev_str, S_IFBLK|0600, dev);
	if (err < 0)
		return err;

	if (lkl_sys_access("/mnt", 0700) < 0) {
		err=lkl_sys_mkdir("/mnt", 0700);
		if (err < 0) 
			return err;
	}

	err=lkl_sys_mkdir(mnt_str, 0700);
	if (err < 0) {
		lkl_sys_unlink(dev_str);
		return err;
	}

	if (fs_type)
		err=lkl_sys_mount(dev_str, mnt_str, fs_type, flags, data);
	else
		err=lkl_mount(dev_str, mnt_str, flags, data);
	if (err < 0) {
		lkl_sys_unlink(dev_str);
		lkl_sys_rmdir(mnt_str);
		return err;
	}

	return 0;
}

long lkl_umount_dev(__kernel_dev_t dev, int flags)
{
	char dev_str[] = { "/dev/xxxxxxxxxxxxxxxx" };
	char mnt_str[] = { "/mnt/xxxxxxxxxxxxxxxx" };
	int err;
	

	snprintf(dev_str, sizeof(dev_str), "/dev/%016x", dev);
	snprintf(mnt_str, sizeof(mnt_str), "/mnt/%016x", dev);

	err=lkl_sys_umount(mnt_str, 0);

	if (err < 0)
		return err;

	lkl_sys_unlink(dev_str);
	lkl_sys_rmdir(mnt_str);

	return 0;
}

