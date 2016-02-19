#include <linux/fs.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/buffer_head.h>

#include "tfs.h"
#include "tarparser.h"

ssize_t tfs_read_block(struct super_block *sb, char *buf, size_t size, uint64_t start_byte)
{
	unsigned long blocksize = sb->s_blocksize;
	sector_t first_block = start_byte/blocksize;
	sector_t last_block = (start_byte + size - 1)/blocksize;
	uint64_t blocks = last_block - first_block + 1;
	ssize_t seek = 0;
	struct buffer_head *bh;
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

	for (sector_t i = 1; i < blocks - 1; ++i) {
		if (!(bh = sb_bread(sb, first_block + i)))
			return seek;

		memcpy(buf + seek, bh->b_data, blocksize);
		seek += blocksize;
		brelse(bh);
	}

	bh = sb_bread(sb, last_block);
	if (!bh)
		return seek;

	memcpy(buf + seek, bh->b_data, size + start_byte - last_block * blocksize);
	seek += size + start_byte - last_block * blocksize;
	brelse(bh);
	
	return seek;
}

int fill_private_inode(struct inode *inode)
{
	struct tfs_i_info *private = 
		(struct tfs_i_info*)kmalloc(sizeof(struct tfs_i_info), GFP_KERNEL);

	if(!private)
		return -ENOMEM;

	inode->i_private = private;
	return 0;
}

struct inode *tfs_get_inode(struct super_block *sb, ino_t no, char *header)
{
	struct inode *inode;
	int err;

	inode = iget_locked(sb, no);
	if (!inode)
		return ERR_PTR(-ENOMEM);

	if(!(inode->i_state & I_NEW)){
		return inode;
	}
	
	inode->i_mode = get_tar_type(header) | get_tar_mode(header);
	inode->i_uid.val = get_tar_uid(header);
	inode->i_gid.val = get_tar_gid(header);
	inode->i_mtime = inode->i_atime = inode->i_ctime = get_tar_mtime(header);
	inode->i_size = get_tar_size(header);

	inode->i_op = &tfs_dir_iop;
	inode->i_fop = &tfs_dir_fop;

	err = fill_private_inode(inode);
	if(err){
		iget_failed(inode);
		return ERR_PTR(err);
	}
	
	unlock_new_inode(inode);
	return inode;
}

static int name_header_cmp(char *file_name, char *header)
{
	char buf[size_name_tar_file + 1];
	char *strptr = buf;
	size_t len_str;

	len_str = get_tar_name(header, buf);

	// you may create archive with argument.
	if(strptr[0] == '.'){
		strptr += 2;
		len_str -= 2;
	}

	// not exist different between directory and regular file.
	if(strptr[len_str - 1] == '/'){
		strptr[len_str - 1] = '\0';
		--len_str;
	}

	return strcmp(file_name, strptr);
}

/*
 * return 0 if success and 1 if end of tar.
 */
int get_header(struct super_block *sb, struct header_pos *pos, char *header){
	size_t file_size;

	if(tfs_read_block(sb, header, TFS_BLOCK_SIZE, pos->offset_next_header) < TFS_BLOCK_SIZE)
			return 1;

	file_size = get_tar_size(header);
	// all files take natural number of blocks
	if(file_size  && file_size % TFS_BLOCK_SIZE)
		file_size = file_size + (TFS_BLOCK_SIZE - file_size % TFS_BLOCK_SIZE);

	pos->offset = pos->offset_next_header;
	pos->offset_next_header += file_size + TFS_BLOCK_SIZE;
	++(pos->num);
	return 0;
}

/*
 * seek_header - seek block that connect to file with name @filename.
 * If success return 0, if not found return 1.
 * @buf - seek_header return block into buf, size buf must be TFS_BLOCK_SIZE.
 * @number - index number file in tar.
 * @start_byte - it's start byte of file in tar.
 */
static int seek_header(struct super_block *sb, char *file_name, char *header,
			unsigned long *number, uint64_t *start_byte)
{
	struct header_pos pos;
	init_pos_start(&pos);

	while(1) {
		if(get_header(sb,&pos,header))
			return 1;

		if(!name_header_cmp(file_name + 1, header)) {
			*number = pos.num;
			*start_byte = pos.offset + TFS_BLOCK_SIZE;
			return 0;
		}
	}
}

static struct dentry *tfs_lookup(struct inode *dir, struct dentry *dentry, unsigned flags)
{
	uint64_t start_byte;
	struct inode *inode;
	char *header = (char *)kmalloc(sizeof(char) * TFS_BLOCK_SIZE, GFP_KERNEL);
	unsigned long i_ino;
	char *buf_name= (char *)kmalloc(sizeof(char) * (size_name_tar_file + 1), GFP_KERNEL);
	
	if(!header || !buf_name)
		return ERR_PTR(-ENOMEM);

	char *name = dentry_path_raw(dentry, buf_name, size_name_tar_file + 1);

	if(!seek_header(dir->i_sb, name, header, &i_ino, &start_byte)) {
		inode = tfs_get_inode(dir->i_sb, i_ino, header);
		d_add(dentry, inode);
	}

	kfree(header);
	kfree(buf_name);
	return NULL;
}

const struct inode_operations tfs_dir_iop = {
	.lookup = tfs_lookup
};