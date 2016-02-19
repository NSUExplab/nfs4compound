#include<linux/pagemap.h>
#include<linux/slab.h>
#include<linux/fs.h>

#include"tarfs.h"
#include"tarparser.h"

struct address_space_operations tarfs_aops = {
	.readpage = tarfs_readpage,
};

int map_file(struct inode* file_node,void* data,int start,int how_much)
{
	struct disk_position disk_pos;
	size_t file_size;
	int offset_in_first_block;
	int block_no;
	int was_written_in_data;
	int last_fill_buffer_index;
	int err;
	int ret;
	err = init_disk_position(&disk_pos);
	if (err)
	{
		ret = err;
		goto out;
	}
	err = get_file_by_inode(file_node,&disk_pos);
	if (err)
	{
		ret = err;
		goto out;
	}
	offset_in_first_block = disk_pos.offset_in_first;
	block_no = disk_pos.first_block_no;
	last_fill_buffer_index = disk_pos.last_fill_buffer_index;

	file_size = get_size ( file_node -> i_sb,&disk_pos );
	was_written_in_data = 0;
	{
		int offset = offset_in_first_block + META_INFO_SIZE + start;
		char* place = kmalloc(sizeof(struct BLOCK_BUFFER),GFP_KERNEL);
		int remain;
		how_much = file_size - start < how_much ? file_size - start : how_much;
		memcpy(place, disk_pos.buffers[0].buffer,BLOCK_LENGTH);
		while (offset >= BLOCK_LENGTH)
		{
			offset = offset - BLOCK_LENGTH;
			SUCCESS_CHECK(tarfs_block_read(file_node->i_sb,place,++block_no));
		}
		remain  = how_much;
		while(remain > 0)
		{
			int write_bytes = BLOCK_LENGTH - offset > remain ? remain : BLOCK_LENGTH - offset;
			memcpy(data+was_written_in_data,place + offset,write_bytes);
			offset = 0;
			was_written_in_data += write_bytes;
			remain -= write_bytes;
			if (remain > 0)
			{
				SUCCESS_CHECK(tarfs_block_read(file_node->i_sb,place,++block_no));
			}
		}
	}
	memset(data+was_written_in_data, 0 ,PAGE_CACHE_SIZE - was_written_in_data);
	ret = how_much;
out:
	deinit_disk_position(&disk_pos);
	return how_much;
}

int tarfs_readpage(struct file* flip,struct page* page)
{
	//kmap() - this function is simple for pages, that are not in HIGHMEM ZONE - just return the page_address(page);
	//but it is little difficult for pages that are in hIGHMEM ZONE, so we MUST make kunmap for such pages.
	//when you see the comments, that are below this lines delete it...(in /* .. */)
	/*
		Not all physical pages can be mapped to kernel space. It mean, that virtual address is mapping
		not in kernel space. So pages, that are relevant to group of such pages, are Highmem pages.
		and are placed in special group HIGHMEM_ZONE!
	*/
	//goto out;
	if (!PageUptodate(page))
	{
		/*
			I often ask myself, what is fucking uptodate flag? 
			So apparently, when high level code pass the struct page to this func, it
			mark this page as invalid. Checking this flag, and see it as invalid, means
			that this page can be use for mapping part of file in this page.
			So, When we finish the work, we must to set 'uptodate' flag for
			high level, that it can understand, that io operation was successfully completed
			otherwise, it suppose that was io error..
			I think that it not require to make check of this flag at the beginning of this function.
			Uptodate flag means that the data is placed in page are actual.
		*/
		int ret;
		void* data = kmap(page);
		ret = map_file(flip->f_inode,data,PAGE_CACHE_SIZE * page->index,PAGE_CACHE_SIZE);
		if (ret < 0)
		{
			printk("Aaaaaaaaaaaaaa\n");
			goto out;
		}
		flip -> f_pos += ret;
		//flush_dcache_page(page); ?? I still don't know of nessessary of this function... ??
		SetPageUptodate(page); //set this flag for high level code, otherwise it thinks, that io error occur
		kunmap(page);
	}
out:
	unlock_page(page);
	return 0;
}
