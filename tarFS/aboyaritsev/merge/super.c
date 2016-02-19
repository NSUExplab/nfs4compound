#include<linux/slab.h>
#include<linux/buffer_head.h>

#include"tarfs.h"
#include"tarparser.h"

static void tarfs_put_super(struct super_block* sb)
{
    printk("tarfs super block destroyed\n");
}

int allocation_counter = 0;

static struct inode* tarfs_alloc_inode(struct super_block* sb)
{ 
	//need blockers
	struct tarfs_inode* tarfs_node = kmem_cache_alloc(tarfs_inode_cachep,GFP_KERNEL);

	if (NULL == tarfs_node)
	{
		return NULL;
	}
	printk("Memory was allocated!\n");
	allocation_counter++;
	return & tarfs_node -> vfs_inode;
}
static void tarfs_destroy_inode(struct inode* node)
{
	allocation_counter--;
	kmem_cache_free(tarfs_inode_cachep,node);
}
static struct super_operations const tarfs_super_ops =
{
	.alloc_inode = tarfs_alloc_inode,
	.destroy_inode = tarfs_destroy_inode,
	.put_super = tarfs_put_super,
};

static int tarfs_fill_sb(struct super_block * sb, void * data,int silent)
{
	struct inode * root = NULL;

	sb->s_magic = TARFS_MAGIC;
	sb->s_op = &tarfs_super_ops;
	
	if (!sb_set_blocksize(sb,BLOCK_LENGTH))
	{
	    return -EINVAL;
	}

	root = tarfs_inode_get(sb,ROOT_NODE,NULL); //hardcode 0 number is root node, node of where we mount it.

	if (IS_ERR(root))
	{
	    printk("inode allocation failed...\n");
	    return -ENOMEM;
	}
	
	tarfs_inode_init(root); // only init inode operations.
	inode_init_owner(root,NULL,S_IFDIR);

	sb -> s_root = d_make_root(root); // from inode to dentry.
	
	if (! sb->s_root)
	{
	    printk("root creation failed\n");
	    return -ENOMEM;
	}
	return 0;
}


struct dentry* tarfs_mount ( struct file_system_type* type,int flags,char const* dev,void* data)
{
	struct dentry* const entry = mount_bdev(type,flags,dev,data,tarfs_fill_sb);
	if (IS_ERR(entry))
	{
		printk("tarfs mounting failed\n");
	}
	else
	{
		printk("tarfs_success...");
	}
	return entry;
}

