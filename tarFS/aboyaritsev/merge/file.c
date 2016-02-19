#include<linux/fs.h>
#include<linux/string.h>
#include<linux/stat.h>
#include<linux/uidgid.h>

#include"tarfs.h"
#include"tarparser.h"


struct file_operations tarfs_dir_operations = 
{
	.open = tarfs_open,
	.readdir = tarfs_readdir,
	.read = generic_read_dir,
};

int tarfs_open(struct inode* node,struct file* file)
{
	printk("open: hello\n");
	return generic_file_open(node,file);
}

int is_child_of_first_level(const char* dir,const char* subdir)
{
	char prefix_of_subdir[100] = { 0 };
	int slash_count = 0;
	int len_dir = strlen(dir);
	int len_subdir = strlen(subdir);
	strncpy(prefix_of_subdir,subdir,len_dir);
	if  (!strcmp(prefix_of_subdir,dir) )
	{
		int i;
		for (i = len_subdir-1; i >= len_dir-1; --i)
		{
			if (subdir[i] == '/')
			{
				++slash_count;
			}
		}
	}
	return (slash_count == 1);
}

char* get_last_dentry(char* file_name)
{
	int i;
	int last_index = 1;
	size_t file_length = strlen(file_name);
	for (i = 0; i < file_length; ++i)
	{
		if (file_name[i] == '/')
		{
			last_index = i+1;
		}
	}
	return file_name + last_index;
}

int tarfs_readdir(struct file* f,void * d,filldir_t fldt)
{
	struct inode* file_node = file_inode(f);
	struct super_block* sb = file_node -> i_sb;
	struct disk_position disk_pos;
	
	char directory_name_ar[101] = { 0 } ;
	char * directory_name;
	char open_file_name[101] = {0};
	
	int file_count = 0;
	int i;
	int ret;
	
	if (f->f_pos)
	{
		return 0;
	}
	
	init_disk_position(&disk_pos);
	
	directory_name = dentry_path_raw(f->f_path.dentry,directory_name_ar,100);
	
	while(1)
	{
		for (i = 0;i< 101;++i)
		{
			open_file_name[i] = 0;
		}
		ret = get_next_file(sb,&disk_pos,open_file_name+1);
		if (ret)
		{
			break;
		}
		file_count ++;
		open_file_name[0] = '/';
		if (is_child_of_first_level(directory_name,open_file_name))
		{
			char* last_dentry = get_last_dentry(open_file_name);
			fldt(d,last_dentry,off_lens[0].length,f->f_pos,1,file_node->i_mode);
		}
	}
	deinit_disk_position(&disk_pos);
	f -> f_pos = file_count;
	return 0;
}



