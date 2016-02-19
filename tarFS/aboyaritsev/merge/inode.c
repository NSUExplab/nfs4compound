#include<linux/string.h>
#include<linux/kernel.h>
#include<linux/stat.h>
#include<linux/uidgid.h>
#include<linux/fs.h>

#include"tarfs.h"
#include"tarparser.h"


const struct inode_operations tarfs_inode_ops = 
{
	.lookup = tarfs_lookup,
};

static int delete_dentry(const struct dentry* dentry)
{
	return 1;
}

struct dentry_operations tarfs_dentry_ops=
{
	.d_delete  = delete_dentry,
};



static int fill_disk_position_next_file(struct super_block* sb,struct disk_position* disk_pos)
{
	int last_fill_buffer_index = disk_pos -> last_fill_buffer_index;
	if (last_fill_buffer_index == -1)
	{
		SUCCESS_CHECK(tarfs_block_read(sb,disk_pos->buffers[0].buffer,0));
		disk_pos -> last_fill_buffer_index = 0 ;
		disk_pos -> first_block_no = 0;
		disk_pos -> offset_in_first = 0;
	}
	else 
	{
		int first_block_no = disk_pos -> first_block_no;
		int previous_file_start_offset = disk_pos -> offset_in_first;
		int previous_file_end_offset,previous_file_end_offset_in_block;
		size_t previous_file_size = get_size(sb,disk_pos);
		if (previous_file_size > 0)
		{
			//alignment
			previous_file_size += (TAR_BLOCK_SIZE - ((TAR_BLOCK_SIZE + previous_file_size)%TAR_BLOCK_SIZE));
		}
		previous_file_end_offset = (previous_file_start_offset + META_INFO_SIZE + previous_file_size);
		
		previous_file_end_offset_in_block = previous_file_end_offset % BLOCK_LENGTH;
		
		first_block_no = first_block_no + (previous_file_end_offset - previous_file_start_offset) / BLOCK_LENGTH ;

		//if (previous_file_end_offset_in_block == 0)

		SUCCESS_CHECK(tarfs_block_read(sb,disk_pos -> buffers[0].buffer,first_block_no));
		
		disk_pos -> last_fill_buffer_index = 0;
		
		disk_pos -> first_block_no = first_block_no;
		
		disk_pos -> offset_in_first = previous_file_end_offset_in_block;

		if (BLOCK_LENGTH - previous_file_end_offset_in_block < META_INFO_SIZE) // Meta doesn't placed in only one block
		{
			SUCCESS_CHECK(tarfs_block_read(sb,disk_pos->buffers[1].buffer,first_block_no+1));
			disk_pos-> last_fill_buffer_index = 1 ;
		}
	}
	return 0;
}

static int find_file_by_dentry(struct dentry* file_dentry,struct super_block* sb,struct disk_position* disk_pos)
{
	char file_name_array[100] = { 0 };
	char open_file_name[100] = { 0 };
	char * file_name;
	int inode_no = 0;
	
	file_name = dentry_path_raw(file_dentry,file_name_array, 100) + 1;
	do
	{
		int k;
		int err;
		for (k = 0; k < 100; ++k)
		{
			open_file_name[ k ] = 0; //defer
		}		
		err = get_next_file(sb,disk_pos,open_file_name);
		if (err)
		{
			return err;
		}
		
		inode_no++;
	} while (strcmp(open_file_name,file_name));
	return inode_no;
}


int get_file_by_inode(struct inode* file_node, struct disk_position* disk_pos)
{
	struct tarfs_inode* tarfs_node = container_of(file_node,struct tarfs_inode,vfs_inode);
	struct super_block* sb = file_node -> i_sb;
	
	int first_block_no = tarfs_node -> first_block_no;
	int offset_in_first = tarfs_node -> offset_in_first;
	int last_fill_buffer_index = tarfs_node -> last_fill_buffer_index;
	
	SUCCESS_CHECK(tarfs_block_read(sb,disk_pos->buffers[0].buffer,first_block_no));
	if (last_fill_buffer_index == 1)
	{
		SUCCESS_CHECK(tarfs_block_read(sb,disk_pos->buffers[1].buffer,first_block_no + 1));
	}
	disk_pos -> first_block_no = first_block_no;
	disk_pos -> offset_in_first = offset_in_first;
	disk_pos -> last_fill_buffer_index = last_fill_buffer_index;
	return 0;
/*
	char open_file_name[100];
	unsigned long i;
	unsigned long inode_no = file_node -> i_ino;
	
	for ( i = 0 ; i < inode_no ; ++ i)
	{
		int err;
		int j;
		for( j = 0; j < 100;++j)
		{
			open_file_name[j] = 0;
		}
		err = get_next_file(sb,disk_pos,open_file_name);
		if (err)
		{
			return err;
		}
	}
	return 0;
*/
}


struct inode* tarfs_inode_get(struct super_block* sb,int no,struct disk_position* disk_pos)
{
	struct inode* inode;
	inode = iget_locked(sb, no);
	if (!inode)
	{
		 return ERR_PTR(-ENOMEM);
	}
	if (!(inode->i_state & I_NEW))
	{
		return inode; 
	}
    
	if (no == 0)
	{
		inode -> i_size = 4096;
		inode -> i_ino = 0;
		inode -> i_sb = sb;
		inode -> i_atime = inode->i_ctime = inode->i_mtime = CURRENT_TIME;
		unlock_new_inode(inode);
		inode ->i_fop = &tarfs_dir_operations;
		return inode;
	}

	if (!inode -> i_fop)
	{
	    printk("i was hree\n");
	    //inode ->i_fop = &tarfs_dir_operations;
	}
	/*This function clear I_NEW flag of inode, so it seems that it can solve our first main task!!!!
	    see the users.nccs.gov/~fwang2/linux/lk_iopath.txt
	*/
	fill_node(inode,disk_pos);
	unlock_new_inode(inode);
	return inode;
} 

struct dentry* tarfs_lookup(struct inode* dir,struct dentry* name,unsigned flags)
{
	// we should put inode in dentry(d_add)
	struct super_block* sb = dir->i_sb;
	struct disk_position disk_pos;
	struct inode* file;
	
	int inode_no = 0;
	int err = init_disk_position(&disk_pos);
	
	if (err)
	{
		return NULL;
	}
	
	inode_no = find_file_by_dentry(name, sb, &disk_pos);
	
	if (inode_no < 0 )
	{
		goto out;
	}
	
	file = tarfs_inode_get(sb,inode_no,&disk_pos);
	
	if (IS_ERR(file))
	{
		printk("+ tarfs inode is ERR!\n");
		goto out;
	}

	if ( !name->d_sb->s_d_op )
	{
		d_set_d_op(name,&tarfs_dentry_ops);
	}
	d_add(name,file);
out:
	deinit_disk_position(&disk_pos);
	return NULL;
}


int get_next_file(struct super_block* sb,struct disk_position* disk_pos, /* OUT */ char* open_file_name)
{
	int len;
	int err = fill_disk_position_next_file(sb,disk_pos);
	if (err)
	{
		return err;
	}
	
	err = get_tar_field(sb,disk_pos,ind_file_name,open_file_name);
	if (err)
	{
		return err;
	}
	len = strlen(open_file_name);
	if (len == 0)
	{
		printk("seems like archive is finished...\n");
		return -ETAREND;
	}
	if (open_file_name[len - 1] == '/')
	{
		open_file_name[len-1] = '\0';
	}
	return 0;
}

void tarfs_inode_init(struct inode* root)
{
	root -> i_op = &tarfs_inode_ops;
}



void fill_node(struct inode* file_node, struct disk_position* disk_pos)
{
	struct tarfs_inode * tarfs_node = (struct tarfs_inode*) file_node;
	
        umode_t mode = get_mode(file_node->i_sb, disk_pos);
	kuid_t uid =  get_uid(file_node -> i_sb, disk_pos);
	kgid_t gid =  get_gid(file_node -> i_sb, disk_pos);
	size_t size = get_size(file_node->i_sb, disk_pos);

	file_node -> i_mode = mode;
	file_node -> i_uid = uid;
	file_node -> i_gid = gid;
	file_node -> i_size = size;

	file_node -> i_atime = file_node -> i_ctime = file_node -> i_mtime = CURRENT_TIME;
	file_node -> i_op = &tarfs_inode_ops;
	
	tarfs_node -> first_block_no = disk_pos -> first_block_no;
	tarfs_node -> offset_in_first = disk_pos -> offset_in_first;
	tarfs_node -> last_fill_buffer_index = disk_pos -> last_fill_buffer_index;
	
	switch(mode & S_IFMT)
	{
		case S_IFREG:
		    file_node->i_fop = &generic_ro_fops;
		    file_node->i_data.a_ops = &tarfs_aops;
		    break;
		case S_IFDIR:
		    file_node->i_fop = &tarfs_dir_operations;
		    break;
	}
	return;
}