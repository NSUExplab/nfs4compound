#include "tarfs.h"

#include <linux/string.h>

unsigned long read_octal_int(char *data){
	unsigned long n = 0;
	while (data && *data != ' ' && *data != 0){
		n *= 8;
		n += (*data) - '0';
		++data;
	}
	return n;
}

void set_type(umode_t *mode, char flag){
	switch (flag){
		case  0 :
		case '0':
			*mode |= S_IFREG;
			break;
		case '1':
			*mode |= S_IFLNK;
			break;
		case '3':
			*mode |= S_IFCHR;
			break;		
		case '4':
			*mode |= S_IFBLK;
			break;
		case '5':
			*mode |= S_IFDIR;
			break;
		case '6':
			*mode |= S_IFIFO;
			break;
		case '2':
		case '7' :
		default:
			pr_err("tarfs: incorrect type flag \"%c\"\n", flag);
	}
}

int get_blocks_count(int bytes){
	int n = bytes / 512;
	if (bytes % 512 != 0)
		++n;
	return n;
}

int read_inode_by_offset(struct super_block *sb, struct tar_inode *inode, int offset){
	struct buffer_head *bh;
	struct inode *i = (struct inode*)inode;	

	bh = sb_bread(sb, offset);
	if (!bh){
		pr_err("tarfs: unable to read block #%d\n", offset);
		brelse(bh);
		return -EINVAL;
	}
	i->i_mode = read_octal_int(bh->b_data + 100);
	set_type(&i->i_mode, bh->b_data[156]);
	if (i->i_mode & S_IFDIR){		
		i->i_op = &dir_ops;
        i->i_fop = &dir_fops;
	}
	else if (i->i_mode & S_IFREG){
		i->i_fop = &file_fops;
	}
	i_uid_write(i, (uid_t)read_octal_int(bh->b_data + 108));
	i_gid_write(i, (uid_t)read_octal_int(bh->b_data + 116));

	i->i_size = read_octal_int(bh->b_data + 124);
	i->i_blocks = get_blocks_count(i->i_size);

	i->i_ctime.tv_sec = read_octal_int(bh->b_data + 136);
	i->i_mtime.tv_sec = i->i_atime.tv_sec = i->i_ctime.tv_sec;
	i->i_mtime.tv_nsec = i->i_atime.tv_nsec = i->i_ctime.tv_nsec = 0;
	
	i->i_ino = offset + 1;

	inode->offset = offset;

	brelse(bh);
	return 0;
}

struct inode* create_inode_by_name(struct super_block *sb, char *name){
	struct buffer_head *bh;	
	struct tar_inode *inode;
	char buff[100];
	int len;
	int i = 0;
	for(;;++i){
		bh = sb_bread(sb, i);
		if (*(bh->b_data) == 0)
			return NULL;
		//pr_debug("tarfs: check name %s\n", bh->b_data);
		
		strncpy(buff, bh->b_data, 100);
		len = strlen(bh->b_data);		

		if (len > 0 && buff[len - 1] == '/'){	
			buff[len - 1] = 0;
		//	pr_debug("tarfs: found '/', change to \"%s\"\n", buff);
		}
		if (!strcmp(name, buff)){
			inode = (struct tar_inode*)new_inode(sb);
			read_inode_by_offset(sb, inode, i);
			return (struct inode*)inode;	
		}
		i += get_blocks_count(read_octal_int(bh->b_data + 124));
	}
}

