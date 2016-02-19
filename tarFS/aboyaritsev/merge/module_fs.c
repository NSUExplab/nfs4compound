#include<linux/module.h>
#include<linux/init.h>
#include<linux/buffer_head.h>
#include<linux/slab.h>
#include<linux/fs.h>

#include "tarfs.h"
#include "tarparser.h"


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Boyarintsev");

struct kmem_cache* tarfs_inode_cachep;
extern int allocation_counter;
static struct file_system_type tarfs_type =
{
	.owner = THIS_MODULE,
	.name = "tarfs",
	.mount = tarfs_mount,
	.kill_sb = kill_block_super,
	.fs_flags = FS_REQUIRES_DEV,
};

static void tarfs_init_once(void* data)
{
	struct tarfs_inode* node = (struct tarfs_inode*)data;
	inode_init_once(&node->vfs_inode);
}
static int __init init_fs(void)
{
	int ret;
	init_off_lens();
	tarfs_inode_cachep = kmem_cache_create("tarfs inode cache",sizeof(struct tarfs_inode),0,
						(SLAB_RECLAIM_ACCOUNT|SLAB_PANIC|SLAB_MEM_SPREAD),
						    tarfs_init_once);
	if (NULL == tarfs_inode_cachep)
	{
		printk("cache wasn't init\n");
		return -ENOCACHE;
	}
	ret = register_filesystem(&tarfs_type);
	if (0!= ret)
	{
		printk("can not register tarfs\n");
		kmem_cache_destroy(tarfs_inode_cachep);
		return ret;
	}
	printk("tarfs module loaded\n");
	return 0;
}
static void __exit exit_fs(void)
{
	int ret;
	kmem_cache_destroy(tarfs_inode_cachep);
	ret = unregister_filesystem(&tarfs_type);
	if (ret != 0)
	{
		printk("unregister tarfs failed..\n");
		return;
	}
	printk("Module unloaded! Allocation Counter is %d\n",allocation_counter);
}
module_init(init_fs);
module_exit(exit_fs);