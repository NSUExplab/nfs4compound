#include "tarfs.h"

#include <linux/string.h>

static ssize_t tarfs_read(struct file *file, char *buff, size_t size, loff_t *offset){

	struct tar_inode* inode = (struct tar_inode*)(file->f_inode);
	struct super_block *sb  = ((struct inode*)inode)->i_sb;
	struct buffer_head *bh;	
	char *buff_start = buff;
	int f_size = ((struct inode*)inode)->i_size;
	int need_read;

	do{
		bh = sb_bread(sb, (*offset)/512 + inode->offset + 1);
		if (!bh){
			pr_err("tarfs: impossible to read block #%d\n", (int)((*offset)/512 + inode->offset + 1));
			return -ENOMEM;
		}
		need_read = 512 - (*offset) % 512 < size - (buff - buff_start) ?
			    512 - (*offset) % 512 : size - (buff - buff_start);	
		need_read = need_read < f_size - (*offset) ?
			    need_read : f_size - (*offset);		
		memcpy(buff, bh->b_data + (*offset) % 512, need_read);
		buff += need_read;
		(*offset) += need_read;
	
	} while (need_read > 0);

	return buff - buff_start;
}

const struct file_operations file_fops = {
	.llseek = generic_file_llseek,
	.read = tarfs_read,
	.read_iter = generic_file_read_iter,
	.mmap = generic_file_mmap,
	.splice_read = generic_file_splice_read
};

