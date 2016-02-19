#include <linux/fs.h>
#include <linux/slab.h>

#include "tfs.h"
#include "tarparser.h"


/* 
 * return 0 if success.
 */
int init_pos_offset(struct super_block *sb, struct header_pos *pos, uint64_t offset)
{
	init_pos_start(pos);
	size_t tar_size = ((struct tfs_sb_info *)(sb->s_fs_info))->tar_size;

	if(!offset)
		return 0;
	if(offset == tar_size){
		pos->offset_next_header = offset;
		return 0;
	}
	if(offset < tar_size) {
		char *header = (char *)kmalloc(sizeof(char) * TFS_BLOCK_SIZE, GFP_KERNEL);
		while(1){
			if(get_header(sb, pos, header) || pos->offset_next_header > offset) {
				kfree(header);
				return -EINVAL;
			}
			if(pos->offset_next_header == offset){
				kfree(header);
				return 0;
			}
		}
	} else 
		return -EINVAL;
}

/*
 * return 1 if file with name file_name is entry of header, 0 otherwise
 */
static int is_child_file(char *file_name, char *header, char *child_name, size_t *child_len)
{
	char buf[size_name_tar_file + 1];
	size_t header_name_len = get_tar_name(header, buf);
	char *strptr = buf;
	size_t file_name_len = strlen(file_name);

	// you may create archive with argument.
	if(strptr[0] == '.'){
		strptr += 2;
		header_name_len -= 2;
	}

	// not exist different between directory and regular file.
	if(strptr[header_name_len - 1] == '/') {
		strptr[header_name_len - 1] = '\0';
		--header_name_len;
	}

	//exactly no child
	if(header_name_len <= file_name_len)
		return 0;

	if(strncmp(file_name, strptr, file_name_len))
		return 0;

	//skip all '/' before child name
	while(strptr[file_name_len] == '/')
		++file_name_len;

	*child_len = header_name_len - file_name_len;
	for (int i = 0; i < *child_len; ++i) {
		char c;
		c = child_name[i] = strptr[file_name_len + i];
		if(c == '/')
			return 0;
	}
	child_name[*child_len] = '\0';

	return 1;
}

static int tfs_readdir(struct file *f, void *dirent, filldir_t fldt)
{
	int err;
	struct header_pos pos;
	struct super_block *sb = file_inode(f)->i_sb;
	size_t child_len;
	char *header = (char *)kmalloc(sizeof(char) * TFS_BLOCK_SIZE, GFP_KERNEL);
	char *buf_name = (char *)kmalloc(sizeof(char) * (size_name_tar_file + 1), GFP_KERNEL);
	char *child_name = (char *)kmalloc(sizeof(char) * (size_name_tar_file + 1), GFP_KERNEL);

	if(!header || !buf_name)
		return -ENOMEM;

	err = init_pos_offset(sb, &pos, f->f_pos);
	if(err){
		kfree(child_name);
		kfree(header);
		kfree(buf_name);
		return err;
	}

	char *name = dentry_path_raw(f->f_path.dentry, buf_name, size_name_tar_file + 1);

	while(1) {
		if(get_header(sb, &pos, header))
			break;
		if(is_child_file(name + 1, header, child_name, &child_len))
			fldt(dirent, child_name, child_len, pos.offset, 
				pos.num, file_inode(f)->i_mode);
	}

	f->f_pos = pos.offset_next_header;
	kfree(child_name);
	kfree(header);
	kfree(buf_name);
	return 0;
}

const struct file_operations tfs_dir_fop = {
	.readdir = tfs_readdir
};