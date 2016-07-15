#ifndef __TARFS_H__
#define __TARFS_H__

#include <linux/fs.h>
#include <linux/buffer_head.h>

static const char magic_numbers[2][8] = {"ustar", "ustar  "};

struct tar_inode{
	struct inode super;
	int offset;
};

extern const struct file_operations dir_fops;
extern const struct inode_operations dir_ops;
extern const struct file_operations file_fops;

int get_blocks_count(int bytes);
unsigned long read_octal_int(char *data);
void tar_inode_free(struct inode *i);
struct inode* tar_inode_alloc(struct super_block* sb);
struct inode* create_inode_by_name(struct super_block *sb, char *name);

#endif
