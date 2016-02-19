#pragma once

#define EBLOCK 1
#define ETAREND 2
#define ENOCACHE 3

#define BLOCK_LENGTH 512
#define TAR_BLOCK_SIZE 512

#define TARFS_MAGIC 0x3341234
#define ROOT_NODE 0

#define SUCCESS_CHECK(x) if (0 != x) { return x; }

struct off_len
{
    int offset;
    int length;
};

struct BLOCK_BUFFER
{
	char buffer[BLOCK_LENGTH];
};

/*
    *	disk_position  -- describe copy of disk in RAM
    *	buffers -- buffers, that contain snapshot of disk.
    *	first_block_no -- first block number of disk, that contains in buffers
    *	offset in first block is offset of beginning of file at the first block.
    *	last_fill_buffer_index is last available index of buffers.
*/
struct disk_position
{
	struct BLOCK_BUFFER* buffers;
	int first_block_no;
	int offset_in_first;
	int last_fill_buffer_index;
};

struct tarfs_inode
{
	struct inode vfs_inode;
	int first_block_no;
	int offset_in_first;
	int last_fill_buffer_index;
};


extern struct kmem_cache* tarfs_inode_cachep;
extern struct off_len off_lens[];
extern struct dentry_operations tarfs_dentry_ops;
extern struct file_operations tarfs_dir_operations;
extern struct address_space_operations tarfs_aops;

struct dentry* tarfs_mount ( struct file_system_type* type,int flags,char const* dev,void* data); 
struct inode* tarfs_inode_get(struct super_block* sb,int no,struct disk_position *);
struct dentry* tarfs_lookup(struct inode* dir,struct dentry* name,unsigned flags);

int get_file_by_inode(struct inode* file_node, struct disk_position* disk_pos);
int tarfs_open(struct inode* node,struct file* file);
int get_next_file(struct super_block* sb,struct disk_position* disk_pos,char* open_file_name);
int tarfs_block_read(struct super_block* sb,char* data,int block_no);
int init_disk_position(struct disk_position* disk_pos);
int tarfs_readpage(struct file* flip,struct page* page);
int tarfs_readdir(struct file* f,void * d,filldir_t fldt);

void deinit_disk_position(struct disk_position* disk_pos);
void tarfs_inode_init(struct inode*);
void fill_node(struct inode* file_node, struct disk_position* disk_pos);