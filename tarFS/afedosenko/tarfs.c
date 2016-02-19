#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/pagemap.h>
#include <linux/vfs.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/buffer_head.h>
#include "tarparser.h"
#define TFS_MAGIC 0x19980122
#define DRIVER_AUTHOR "Alex"
#define DRIVER_DESC   "Tarfs"
MODULE_LICENSE("GPL");
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
/*
// Open file

*/
struct tarfs_inode_data{
	loff_t file_offset;
};
struct dentry* tfs_lookup(struct inode* dir, struct dentry* dentry, unsigned int flags);
ssize_t tfs_read_block(struct super_block *sb, char *buf, size_t size, loff_t offset);

static int tfs_read(struct file* file, char* data, size_t count, loff_t* offset){
	struct inode* finode = file->f_dentry->d_inode;
	printk("tarfs: read from %d\n", (int)(file->f_dentry->d_inode->i_private));
	if(*offset + count > finode->i_size) count = finode->i_size - *offset;
	int amount = tfs_read_block(finode->i_sb, data, count, (int)(finode->i_private) + *offset);
	(*offset) += amount;
	printk("tarfs: tried offset %lld, count %lld, read %d\n", *offset, count, amount);
	return amount;
}
static int tfs_open(struct inode *inode, struct file *file){
	generic_file_open(inode, file);
	return 0;
}

// Read file
/*
static int tfs_read(struct file *file, char *buf, size_t count, loff_t *offset){
	printk("read dir: \n");
	generic_read_dir(file, buf, count, offset);
	return 0;
}
*/
// File operations
static struct file_operations tfs_dir_ops = {
 //   .open           = tfs_open,
    .release        = dcache_dir_close,
    .llseek         = dcache_dir_lseek,
    .read           = generic_read_dir,
    .fsync          = noop_fsync,
};
static struct file_operations tfs_file_ops = {
    .open           = tfs_open,
    .read 			= tfs_read,
    .fsync          = noop_fsync,
};
static struct inode_operations tfs_i_ops = {
	.lookup 	= tfs_lookup
};


static struct inode *tfs_make_inode(struct super_block *sb, int i_no)
{
	struct inode *ret = iget_locked(sb, i_no);
	if(ret){
		ret->i_mode = 0755;
		ret->i_uid.val = 0;
		ret->i_gid.val = 0;
		ret->i_blocks = 0;
		ret->i_atime = CURRENT_TIME;
		ret->i_mtime = CURRENT_TIME;
		ret->i_ctime = CURRENT_TIME;
	}
	return ret;
}


char* get_path(struct dentry* dentry)
{
	int name_length = 0, seek = 1;
	struct dentry *nowd = dentry;
	char* path;
	while(nowd != nowd->d_parent){
		name_length += nowd->d_name.len + 1;
		nowd = nowd->d_parent;
	}
	nowd = dentry;
	path = (char*)kzalloc(sizeof(char) * name_length, GFP_NOFS);
	if(!path) return NULL;
	path[name_length - seek] = '\0';
	while(nowd != nowd->d_parent)
	{
		if(seek != 1){ 
			seek++;
			path[name_length - seek] = '/';
		}
		seek += nowd->d_name.len;
		memcpy(path + name_length - seek, nowd->d_name.name, nowd->d_name.len);
		nowd = nowd->d_parent;
	}
	return path;
}

ssize_t tfs_read_block(struct super_block *sb, char *buf, size_t size, loff_t start_byte)
{
	unsigned long blocksize = sb->s_blocksize;
	sector_t first_block = start_byte/blocksize;
	sector_t last_block = (start_byte + size - 1)/blocksize;
	uint64_t blocks = last_block - first_block + 1;
	ssize_t seek = 0;
	struct buffer_head *bh;
	sector_t i = 0;
	unsigned long offset = start_byte - first_block * blocksize;

	if (!(bh = sb_bread(sb, first_block)))
		return seek;

	if (blocks == 1) {
		memcpy(buf, (bh->b_data + offset), size);
		brelse(bh);
		seek += size;
		return seek;
	} else {
		memcpy(buf, (bh->b_data + offset), blocksize - offset);
		brelse(bh);
		seek += blocksize - offset;
	}

	for (i = 1; i < blocks - 1; ++i) {
		bh = sb_bread(sb, first_block + i);
		if (!bh)
			return seek;

		memcpy(buf + seek, bh->b_data, blocksize);
		seek += blocksize;
		brelse(bh);
	}

	bh = sb_bread(sb, last_block);
	if (!bh)
		return seek;

	memcpy(buf + seek, bh->b_data, size - seek);
	seek += (size - seek);
	brelse(bh);
	
	return seek;
}


struct dentry* tfs_lookup(struct inode* dir, struct dentry* dentry, unsigned int flags)
{
	int i_no = 0, filesize, mode, type;
	uint64_t seek = 0; 
	size_t h_size = get_header_size();
	int amount, namelen;
	struct inode* finode;
	char* data = kzalloc(sizeof(char) * (h_size + 1), GFP_NOFS);
	char* name = kzalloc(sizeof(char) * 256, GFP_NOFS);
	char* path = get_path(dentry);
	if(!path) goto err;
	printk("tarfs: path: %s\n", path);
	while(1)
	{
		amount = tfs_read_block(dir->i_sb, data, h_size, seek);
		if(amount < h_size) goto err;
		i_no++;
		filesize = get_size(data);
		get_name(data, name);
		namelen = strlen(name);
		if(name[namelen - 1] == '/') name[namelen - 1] = '\0';
		printk("tarfs: Name %s\n", name);
		printk("tarfs: filesize %d\n", filesize);
		printk("tarfs: seek %d\n", seek);
		if(strcmp(name, path) == 0) {
			finode = tfs_make_inode(dir->i_sb, i_no);
			if (!finode)
				goto err;
			if (finode->i_state & I_NEW){
				printk(KERN_INFO "tarfs: Got new VFS inode for #%d, let's fill in\n", i_no);
				type = get_type(data);	
				mode = get_mode(data);		
				finode->i_mode = 	mode | type;
				finode->i_uid.val = get_tar_uid(data);
				finode->i_gid.val = get_tar_gid(data);
				finode->i_mtime = 	finode->i_atime = finode->i_ctime = get_tar_mtime(data);
				finode->i_size = 	filesize;
				if(type == S_IFDIR)
					finode->i_fop = 	&tfs_dir_ops;
				else if(type == S_IFREG)
					finode->i_fop = 	&tfs_file_ops;
				finode->i_op = 		&tfs_i_ops;
				finode->i_private = (void*)(h_size + seek);
				unlock_new_inode(finode);
			}
			else{
				printk(KERN_INFO "tarfs: Got VFS inode from inode cache\n");
			}
			d_add(dentry, finode);
			goto out;
		}
		if(filesize != 0) filesize = filesize + (h_size - filesize%h_size);
		seek += (h_size + filesize);
	}
err:
	kfree((void*)data);
	kfree((void*)path);
	kfree((void*)name);
	return ERR_PTR(-EACCES);
out:
	kfree((void*)data);
	kfree((void*)path);
	kfree((void*)name);
	return NULL;
}

// Filling Superblock
static int tfs_fill_super(struct super_block *sb, void *data, int silent)
{
	struct inode *root;
//	Initialisation of superblock fields,
	sb->s_blocksize = PAGE_CACHE_SIZE;
	sb->s_blocksize_bits = PAGE_CACHE_SHIFT;
	sb->s_magic = TFS_MAGIC;
//	Creating of root inode
	root = tfs_make_inode (sb, 0);
	if(!root)
		goto out;
//	if(root->i_state & I_NEW){
	root->i_mode =	S_IFDIR | 0755;
	root->i_op =	&tfs_i_ops;
	root->i_fop =	&tfs_dir_ops;
	unlock_new_inode(root);
//	}else{
//		printk("tarfs: Got root's vfs inode from inode cache\n");
//	}
	
//	Creating of root dentry	
	sb->s_root = d_make_root(root);
	if(!sb->s_root)
		goto out_iput;
	
	return 0;
out_iput:
	iput(root);
out:
	printk("tarfs: error in fill super\n");
	return -ENOMEM;
}

// Getting Superblock
static struct dentry *tfs_mount(struct file_system_type *fst, int flags, const char *devname, void *data)
{
	return mount_bdev(fst, flags, devname, data, tfs_fill_super);
};

// tarfs type
static struct file_system_type tfs_type = {
	.owner 		= THIS_MODULE,
	.name 		= "tarfs",
	.kill_sb	= kill_block_super,
	.mount 		= tfs_mount
};

// Initialisaton of tarfs
static int __init tfs_init(void)
{
	init_offsets();
	printk("tarfs: register filesystem tarfs\n");
	return register_filesystem(&tfs_type);
}

static void __exit tfs_exit(void)
{
	unregister_filesystem(&tfs_type);
}
module_init(tfs_init);
module_exit(tfs_exit);
