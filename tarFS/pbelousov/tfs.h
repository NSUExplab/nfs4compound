#ifndef __TFS_H__
#define __TFS_H__

/* 
 * It's convinient.
 * Every tar file header consist of 512 byte.
 */
#define TFS_BLOCK_SIZE 512

/*
 * Access for root directory.
 */
#define ACCESS_ROOT_INODE 0755

struct header_pos
{
	uint64_t offset; //start current header
	uint64_t offset_next_header;
	unsigned long num;
};

// private info inode
struct tfs_i_info{
	int num; //fake
};

// private info super block
struct tfs_sb_info{
	size_t tar_size;
};

extern const struct inode_operations tfs_dir_iop;
extern const struct file_operations tfs_dir_fop;
extern const struct super_operations tfs_sops;
extern struct file_system_type tfs_type;
extern struct kmem_cache *tfs_inode_cache;

extern struct inode *tfs_get_inode(struct super_block *, ino_t, char *);
extern ssize_t tfs_read_block(struct super_block *, char *, size_t, uint64_t);
extern int get_header(struct super_block *, struct header_pos *, char *);
extern int init_pos_offset(struct super_block *, struct header_pos *, uint64_t);
extern int fill_private_inode(struct inode*);

static inline void init_pos_start(struct header_pos *pos){
	pos->offset = 0;
	pos->offset_next_header = 0;
	pos->num = 0;
}

#endif /*__TFS_H__*/