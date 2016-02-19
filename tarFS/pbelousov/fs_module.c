#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>

#include "tfs.h"
#include "tarparser.h"

MODULE_LICENSE("GPL");

struct kmem_cache *tfs_inode_cache;

static void init_once(void *foo)
{
	struct inode *inode = (struct inode*) foo;
	inode_init_once(inode);
}

static int tfs_inode_cache_create(void)
{
	tfs_inode_cache = kmem_cache_create("tfs_inode", 
		sizeof(struct inode),
		0, SLAB_RECLAIM_ACCOUNT | SLAB_MEM_SPREAD,
		init_once);
	if(!tfs_inode_cache)
		return -ENOMEM;
	return 0;
}

static void tfs_inode_cache_destroy(void)
{
	rcu_barrier();
	kmem_cache_destroy(tfs_inode_cache);
}

static int __init tfs_init(void)
{
	int ret = tfs_inode_cache_create();

	if(ret){
		printk("cannot tfs inode cache\n");
		return ret;
	}
	ret = register_filesystem(&tfs_type);
	if(ret){
		tfs_inode_cache_destroy();
		printk("connot register tfs\n");
		return ret;
	}
	return 0;
}

static void __exit tfs_exit(void)
{
	if(unregister_filesystem(&tfs_type))
		printk("connot unregister tfs\n");
	tfs_inode_cache_destroy();
}

module_init(tfs_init);
module_exit(tfs_exit);