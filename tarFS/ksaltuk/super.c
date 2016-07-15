#include "tarfs.h"

#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>

static struct kmem_cache *inodes_cache;


static void tarfs_put_super(struct super_block *sb){
	pr_debug("tarfs: destructor\n");
}


static struct super_operations const tarfs_super_ops = {
	.put_super = tarfs_put_super,
	.alloc_inode = tar_inode_alloc,
	.destroy_inode = tar_inode_free,
};

static int is_correct_magic(const char *magic){
	int i;
	const char *temp1, *temp2;
	for(i = 0; i < 2; ++i){
		temp1 = magic;
		temp2 = &(magic_numbers[i][0]);
		while (*temp1 && *temp2){
			if (*temp1 != *temp2)
				break;
			++temp1;
			++temp2;
		}
		if (*temp1 == 0 && *temp2 == 0)
			return true;
	}
	return false;
}

static int tarfs_fill_sb(struct super_block *sb, void *data, int silent){
	struct inode *root;
	struct buffer_head *bh;

	sb_set_blocksize(sb, 512);
	
	bh = sb_bread(sb, 0);
	if (!bh || bh->b_size != 512){
		pr_err("tarfs: unable to read superblock\n");
		return -EINVAL;
	}
	
	if (!is_correct_magic((bh->b_data) + 257)){
		pr_err("tarfs: incorrect magic: %s\n", (bh->b_data) + 257);
		brelse(bh);
		return -EINVAL;
	} 

	pr_debug("tarfs: found correct magic: %s\n", (bh->b_data) + 257);
	brelse(bh);

	sb->s_magic = *((unsigned long *)((bh->b_data) + 257));
	sb->s_op = &tarfs_super_ops;
	root = new_inode(sb);

	root->i_op = &dir_ops;
        root->i_fop = &dir_fops;	

	if (!root){
		pr_err("tarfs: inode creation failed\n");
		return -ENOMEM;
	}

	root->i_ino = 0;
	root->i_sb = sb;
	root->i_atime = root->i_mtime = root->i_ctime = CURRENT_TIME;
	inode_init_owner(root, NULL, S_IFDIR|0555);

	sb->s_root = d_make_root(root);
	
//	pr_debug("root: %lu\n", (unsigned long)(sb->s_root));

	if (!sb->s_root){
		pr_err("tarfs: root creation failed\n");
		return -ENOMEM;
	}

	return 0;
}

static struct dentry *tarfs_mount(struct file_system_type *type, int flags,
				  char const *dev, void *data)
{
	struct dentry *const entry = mount_bdev(type, flags, dev,
						data, tarfs_fill_sb);

//	pr_debug("mounted dentry: %lu\n", (unsigned long)(entry));
	if (IS_ERR(entry))
		pr_err("tarfs: mounting failed\n");
	else
		pr_debug("tarfs: mounted\n");

	return entry;
}

static struct file_system_type tarfs_type = {
	.owner = THIS_MODULE,
	.name = "tarfs",
	.mount = tarfs_mount,
	.kill_sb = kill_block_super,
	.fs_flags = FS_REQUIRES_DEV,
};

static void tar_inode_init_once(void *i){
	inode_init_once((struct inode*)i);
}

static int inodes_cache_create(void){
	inodes_cache = kmem_cache_create("tar_inode", sizeof(struct tar_inode), 0,
					 (SLAB_RECLAIM_ACCOUNT | SLAB_MEM_SPREAD),
					 tar_inode_init_once);
	if (!inodes_cache)
		return -ENOMEM;
	return 0;
}

struct inode* tar_inode_alloc(struct super_block* sb){
	static int n = 0;	
	struct tar_inode *i = (struct tar_inode*)kmem_cache_alloc(inodes_cache, GFP_KERNEL); 
	
	i->offset = -1;
	((struct inode*)i)->i_ino = ++n;
	pr_debug("tarfs: allocating inode #%lu\n", ((struct inode*)i)->i_ino);

	if (!i)
		return NULL;
	return (struct inode*)i;
	
}

void tar_inode_free_impl(struct rcu_head *head){
	struct inode *i = container_of(head, struct inode, i_rcu);
	
	pr_debug("tarfs: free inode %lu\n", i->i_ino);
	kmem_cache_free(inodes_cache, (struct tar_inode*)i);
}

void tar_inode_free(struct inode *i){
	call_rcu(&i->i_rcu, tar_inode_free_impl);
}
static void inodes_cache_destroy(void){
	rcu_barrier();
	kmem_cache_destroy(inodes_cache);
	inodes_cache = NULL;
}

static int __init tarfs_init(void){
	int ret;
	pr_debug("tarfs: module loaded\n");

	if ((ret = inodes_cache_create()) != 0){
		pr_err("tarfs: unable to create inodes cache\n");
		return ret;
	}

	if ((ret = register_filesystem(&tarfs_type)) != 0){
		pr_err("tarfs: registering failed\n");
		inodes_cache_destroy();
		return ret;
	}
	return 0;
}

static void __exit tarfs_fini(void){

	if (unregister_filesystem(&tarfs_type) != 0){
		pr_err("tarfs: failed to unregister filesystem\n");
	}

	inodes_cache_destroy();
	
	pr_debug("tarfs: module unloaded\n");

}

module_init(tarfs_init);
module_exit(tarfs_fini);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("saltukkos");
		
