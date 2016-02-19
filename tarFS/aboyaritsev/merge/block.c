#include<linux/buffer_head.h>
#include<linux/slab.h>

#include"tarfs.h"
#include"tarparser.h"


int tarfs_block_read(struct super_block* sb,char* data,int block_no)
{
	struct buffer_head* bh;
	bh = sb_bread(sb,block_no);
	if (NULL == bh || NULL == bh -> b_data)
	{
		printk("+ tarfs_block_read impossibe to read block\n");
		return -EBLOCK;
	}
	memcpy(data,bh->b_data,BLOCK_LENGTH);
	return 0;
}


int init_disk_position(struct disk_position* disk_pos)
{
	disk_pos -> buffers = kmalloc(2 * sizeof(struct BLOCK_BUFFER),GFP_KERNEL);
	if (!disk_pos->buffers)
	{
		return 1;
	}
	disk_pos -> last_fill_buffer_index  = -1;
	disk_pos -> offset_in_first = 0;
	disk_pos -> first_block_no = -1;
	return 0;
}

void deinit_disk_position(struct disk_position* disk_pos)
{
	kfree(disk_pos->buffers);
}