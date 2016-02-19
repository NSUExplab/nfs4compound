#include <linux/fs.h>
#include <linux/pagemap.h>
#include <linux/slab.h>

#include "tfs.h"

static size_t init_tar_size(struct super_block *sb)
{
	struct header_pos pos;
	init_pos_start(&pos);
	char *header = (char *)kmalloc(sizeof(char) * TFS_BLOCK_SIZE, GFP_KERNEL);

	while(1) 
		if(get_header(sb,&pos,header)) {
			kfree(header);
			return pos.offset_next_header;
		}
}

static struct inode *tfs_get_root_inode(struct super_block *sb)
{
	/*root inode has number 0*/
	struct inode *inode = iget_locked(sb, 0);
	int err;

	if (!inode)
		return ERR_PTR(-ENOMEM);

	if(!(inode->i_state & I_NEW)){
		return inode;
	} 

	inode->i_sb = sb;
	inode->i_atime = inode->i_mtime = inode->i_ctime = CURRENT_TIME;
	inode->i_op = &tfs_dir_iop;
	inode->i_fop = &tfs_dir_fop;
	inode_init_owner(inode, NULL, S_IFDIR | ACCESS_ROOT_INODE);
	err = fill_private_inode(inode);
	if(err){
		iget_failed(inode);
		return ERR_PTR(err);
	}

	unlock_new_inode(inode);
	return inode;
}

static int tfs_fill_sb(struct super_block *sb, void *data, int silent)
{
	struct inode *root;
	struct tfs_sb_info *sb_info = kmalloc(sizeof(struct tfs_sb_info), GFP_KERNEL);

	if (!sb_set_blocksize(sb, TFS_BLOCK_SIZE)) {
		printk("bad block size\n");
		return -EINVAL;
	}

	root = tfs_get_root_inode(sb);

	if(!root || ! sb_info){
		printk("super block allocation failed\n");
		return -ENOMEM;
	}
	sb->s_root = d_make_root(root);
	sb->s_op = &tfs_sops;

	if(!sb->s_root){
		printk("root creation failed\n");
		return -ENOMEM;
	}

	sb->s_fs_info = sb_info;
	sb_info->tar_size = init_tar_size(sb);

	return 0;
}

static struct dentry *tfs_mount(struct file_system_type *type, int flags,
					char const *dev, void *data)
{
	struct dentry *entry = mount_bdev(type, flags, dev, data, tfs_fill_sb);

	if(IS_ERR(entry))
		printk("tfs mounting failed\n");
	else
		printk("tfs mounting\n");

	return entry;
}

static void tfs_kill_block_super(struct super_block *sb){
	kfree(sb->s_fs_info);
	kill_block_super(sb);
}

static void tfs_i_free_callback(struct rcu_head *head)
{
	struct inode *inode =container_of(head, struct inode, i_rcu);
	kmem_cache_free(tfs_inode_cache, inode);
}

static void tfs_destroy_inode(struct inode *inode)
{
	struct tfs_i_info *private = (struct tfs_i_info*)(inode->i_private);
	kfree(private);

	call_rcu(&inode->i_rcu,tfs_i_free_callback);
}

struct inode* tfs_alloc_inode(struct super_block *sb)
{
	return kmem_cache_alloc(tfs_inode_cache, GFP_KERNEL);
}

struct file_system_type tfs_type = {
	.owner = THIS_MODULE,
	.name = "tfs",
	.mount = tfs_mount,
	.kill_sb = tfs_kill_block_super,
};

const struct super_operations tfs_sops = {
	.destroy_inode = tfs_destroy_inode,
	.alloc_inode = tfs_alloc_inode
};