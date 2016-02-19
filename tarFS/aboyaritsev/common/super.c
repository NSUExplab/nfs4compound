#include<linux/string.h>
#include<linux/pagemap.h>
#include<linux/stat.h>
#include<linux/kernel.h>
#include<linux/dcache.h>
#include<linux/uidgid.h>
#include<linux/module.h>
#include<linux/init.h>
#include<linux/slab.h>
#include<linux/fs.h>
#include<linux/kallsyms.h>
#include<linux/buffer_head.h>

#define HEADER 0
#define FULL_FILE 1

#define BLOCK_LENGTH 512
#define TAR_BLOCK_SIZE 512

#define SUCCESS_CHECK(x) if (0 != x) { return x; }

#define ind_file_name 0
#define ind_file_mode 1
#define ind_file_uid 2
#define ind_file_gid 3
#define ind_file_size 4
#define ind_file_mtime 5
#define ind_file_chksum 6
#define ind_file_typeflag 7
#define ind_file_linkname 8
#define ind_file_magic 9
#define ind_file_version 10
#define ind_file_uname 11
#define ind_file_gname 12
#define ind_file_devmajor 13
#define ind_file_devminor 14
#define ind_file_prefix 15


#define EBLOCK 1
#define ETAREND 2


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Boyarintsev");

size_t widthes[] = { 100,8,8,8,12,12,8,1,100,6,2,32,32,8,8,167};
const int META_INFO_SIZE = 512;

static struct off_len
{
    int offset;
    int length;
} off_lens[16];

static struct BLOCK_BUFFER
{
	char buffer[BLOCK_LENGTH];
	//int block_no;
	//int offset;
};

static struct disk_position
{
	struct BLOCK_BUFFER* buffers;
	int first_block_no;
	int offset_in_first;
	int last_fill_buffer_index;
};

struct inode* aufs_inode_get(struct super_block* sb,int no);
struct dentry* aufs_lookup(struct inode* dir,struct dentry* name,unsigned flags);
int aufs_open(struct inode*, struct file*);
int aufs_block_read(struct super_block* sb,char* data,int block_no);
int aufs_readdir(struct file*, void*, filldir_t);
int get_size(struct super_block* sb,struct disk_position* disk_pos);
int get_tar_field(struct super_block* sb,struct disk_position* disk_pos,int field,char* put_place);
static int aufs_readpage(struct file*flip,struct page* page);
ssize_t aufs_read(struct file* file, const char __user * buf,size_t len,loff_t* ppos);
int init_disk_position(struct disk_position* disk_pos);
int get_file_by_inode_no(struct inode * ,struct disk_position*);
void deinit_disk_position(struct disk_position*disk_pos);
int get_next_file(struct super_block* sb,struct disk_position* disk_pos,char* open_file_name);

const struct address_space_operations aufs_aops = {
	.readpage = aufs_readpage,
};

const struct inode_operations aufs_inode_ops = 
{
    .lookup = aufs_lookup,
};

static int delete_dentry(const struct dentry* dentry)
{
    return 1;
}

const struct dentry_operations aufs_dentry_ops=
{
    .d_delete  = delete_dentry,
};

const struct file_operations aufs_dir_operations = 
{
	.open = aufs_open,
	.readdir = aufs_readdir,
	.read = generic_read_dir,
	//ead = do_sync_read,
};

int get_file_by_inode_no(struct inode* file_node, struct disk_position* disk_pos)
{
	char open_file_name[100];
	unsigned long i;
	unsigned long inode_no = file_node -> i_ino;
	struct super_block* sb = file_node -> i_sb;
//	printk("node no is %lld\n",inode_no);
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
	//printk("file by node: %s\n",open_file_name);
	return 0;
	
}

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
	err = get_file_by_inode_no(file_node,&disk_pos);
	if (err)
	{
		ret = err;
		goto out;
	}
	offset_in_first_block = disk_pos.offset_in_first;
	block_no = disk_pos.first_block_no;
	last_fill_buffer_index = disk_pos.last_fill_buffer_index;

	file_size = get_size ( file_node -> i_sb,&disk_pos );
	printk("start is  %d\n",start);
	printk("block_no start is %d\n",block_no);
	was_written_in_data = 0;
	{
		int offset = offset_in_first_block + META_INFO_SIZE + start;
		char* place = kmalloc(sizeof(struct BLOCK_BUFFER),GFP_KERNEL);
		int remain;
		//printk("start is %d\n",start);
		//printk("file size is %d\n",file_size);
		how_much = file_size - start < how_much ? file_size - start : how_much;
		memcpy(place, disk_pos.buffers[0].buffer,BLOCK_LENGTH);
		while (offset >= BLOCK_LENGTH)
		{
			offset = offset - BLOCK_LENGTH;
			SUCCESS_CHECK(aufs_block_read(file_node->i_sb,place,++block_no));
		}
		printk("but i start from %d\n",block_no);
		//need to consider the start param!
		remain  = how_much;
		while(remain > 0)
		{
			int write_bytes = BLOCK_LENGTH - offset > remain ? remain : BLOCK_LENGTH - offset;
			memcpy(data+was_written_in_data,place + offset,write_bytes);
			offset = 0;
			was_written_in_data += write_bytes;
			remain -= write_bytes;
			//printk("remain is %d\n",remain);
			if (remain > 0)
			{
				SUCCESS_CHECK(aufs_block_read(file_node->i_sb,place,++block_no));
			}
		}
		printk("and finish at block %d\n",block_no);
	}
	//memcpy(data,"hello\n",5/*how_much*/);
	memset(data+was_written_in_data, 0 ,PAGE_CACHE_SIZE - was_written_in_data);
	ret = how_much;
out:
	deinit_disk_position(&disk_pos);
	return how_much;
}

static int aufs_readpage(struct file* flip,struct page* page)
{
	//dump_stack();
	void* data = kmap(page);
	//printk("add1 == %p\n",flip->f_inode);
	//printk("add2 == %p\n",page->mapping->host);
	//printk("contion is %d\n",PageUptodate(page));
	if (!PageUptodate(page))
	{
//		printk("data is %s\n",data);
//		kunmap(page);
		printk("file pos is %d\n",flip->f_pos);
		printk("index is %d\n",page->index);
		int ret = map_file(flip->f_inode,data,PAGE_CACHE_SIZE * page->index,PAGE_CACHE_SIZE);
		if (ret < 0)
		{
			printk("Aaaaaaaaaaaaaa\n");
			return ret;
		}
		flip -> f_pos += ret;
		//page->index += ret;
		printk("ret is %d\n",flip->f_pos);
		flush_dcache_page(page);
		SetPageUptodate(page);
	
		kunmap(page);
	}
	unlock_page(page);
	return 0;
}

int aufs_block_read(struct super_block* sb,char* data,int block_no)
{
	struct buffer_head* bh;
//	printk("hello block_no is %d\n",block_no);
	bh = sb_bread(sb,block_no);
//	printk("buy!\n");
	if (NULL == bh || NULL == bh -> b_data)
	{
		printk("+ aufs_block_read impossibe to read block\n");
		return -EBLOCK;
	}
	memcpy(data,bh->b_data,BLOCK_LENGTH);
	return 0;
}



int fill_disk_position_next_file(struct super_block* sb,struct disk_position* disk_pos)
{
	int last_fill_buffer_index = disk_pos -> last_fill_buffer_index;
	int first_block_no = 0;
	int start_offset;
	if (last_fill_buffer_index == -1)
	{
		SUCCESS_CHECK(aufs_block_read(sb,disk_pos->buffers[0].buffer,0));
		disk_pos -> last_fill_buffer_index = 0 ;
		disk_pos -> first_block_no = 0;
		disk_pos -> offset_in_first = 0;
		first_block_no = 0 ;
		start_offset = 0;
	}
	else 
	{
		first_block_no = disk_pos -> first_block_no;

		int previous_file_start_offset = disk_pos -> offset_in_first;
		size_t previous_file_size = get_size(sb,disk_pos);
		//printk("size is %d\n\n",previous_file_size);
		if (previous_file_size > 0)
		{
			//alignment
			previous_file_size += (TAR_BLOCK_SIZE - ((TAR_BLOCK_SIZE + previous_file_size)%TAR_BLOCK_SIZE));
		}

		int previous_file_end_offset = (previous_file_start_offset + META_INFO_SIZE + previous_file_size);
		
		int previous_file_end_offset_in_block = previous_file_end_offset % BLOCK_LENGTH;
		first_block_no = first_block_no + (previous_file_end_offset - previous_file_start_offset) / BLOCK_LENGTH ;
		//first_block_no -= (BLOCK_LENGTH == (TAR_BLOCK_SIZE));
		
		if (previous_file_end_offset_in_block == 0)
		{
			//first_block_no += 1;
		}
		//[5~printk("first block _ no is %d\n",first_block_no);
		SUCCESS_CHECK(aufs_block_read(sb,disk_pos -> buffers[0].buffer,first_block_no));
		disk_pos -> last_fill_buffer_index = 0;
		
		disk_pos -> first_block_no = first_block_no;
		
		disk_pos -> offset_in_first = previous_file_end_offset_in_block;

		if (BLOCK_LENGTH - previous_file_end_offset_in_block < META_INFO_SIZE) // Meta doesn't placed in only one block
		{
			SUCCESS_CHECK(aufs_block_read(sb,disk_pos->buffers[1].buffer,first_block_no+1));
			disk_pos-> last_fill_buffer_index = 1 ;
		}
	}
	//printk("+ fill_disk_position aufs debug:\n");
	//printk("last_fill_buffer_index == %d\n",disk_pos->last_fill_buffer_index);
	//printk("offset in first block is %d\n",disk_pos->buffers[0].offset);
	//printk("first block is %d\n",disk_pos->buffers[0].block_no);
	//printk("+ fill_disk_position aufs debug finished\n");
	
	return 0;
}
int is_parent(struct dentry* dentry)
{
	return (dentry->d_name.name[0] == '/');
}
int get_full_name(struct dentry* file_dentry,/* out */char* put_argument)
{
	struct dentry* comp = file_dentry;
	const int name_field_length = 100;
	char tmp[101] = { 0 };
	int offset = name_field_length - 1;
	int length = 0;
	while(!is_parent(comp))
	{
		length = strlen(comp -> d_name.name);
		//printk("get_full_name_debug:\n");
		//printk("	offset is %d\n",offset);
		//printk("	name is %s\n",comp->d_name.name);
		//printk("get_full_name_debug finish\n");
		memcpy(tmp + offset - length,comp->d_name.name,length);
		offset -= length;
		printk("%s\n",tmp+offset-length);
		comp = comp->d_parent;
		tmp[--offset]='/';
	}
	tmp[100]='\0';
	strcpy(put_argument,tmp + offset+1);
	return 0;
}

int get_next_file(struct super_block* sb,struct disk_position* disk_pos,char* open_file_name)
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
//	printk("file name is %s\n",open_file_name);
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

int find_file(struct dentry* file_dentry,struct super_block* sb,struct disk_position* disk_pos)
{
	const int stop_node = 32;
	char file_name[100];
	char open_file_name[100] = { 0 };
	int inode_no = 0;
	int err;
	
	err = get_full_name(file_dentry,file_name);
	if (err)
	{
		printk("impossible to get full name:(\n");
		return -1;
	}
//	printk("+ aufs required file %s\n",file_name);
//	printk("parent -- %s\n",file_dentry -> d_parent -> d_name.name);
	
	do
	{
		int k;
		int err;
		
		for (k = 0; k < 100; ++k)
		{
			open_file_name[ k ] = 0; //defer
		}		
//		printk("try to open...\n");
		
		err = get_next_file(sb,disk_pos,open_file_name);
		//printk("new file is %s\n",open_file_name);
		if (err)
		{
			//printk("asdfasdfaufs buffer prepare fail...\n");
			return err;
		}
//		printk("+ aufs was open %s\n",open_file_name);
		
		inode_no++;
	} while (strcmp(open_file_name,file_name) && inode_no < stop_node);
	if (inode_no >= stop_node)
	{
		return -1;
	}
	printk("+ aufs I have found!\n");
	return inode_no;
}

static void init_off_lens()
{
	int i;
	off_lens[0].offset = 0;
	off_lens[0].length = 100;
	for ( i = 1; i < 16; ++i)
	{
		off_lens[i].offset = off_lens[i-1].offset + off_lens[i-1].length;
		off_lens[i].length = widthes[i];
	}
}

int aufs_open(struct inode* node,struct file* file)
{
    printk("open: hello\n");
    return generic_file_open(node,file);
}

void aufs_inode_init(struct inode* root)
{
        root->i_op = &aufs_inode_ops;
}

struct aufs_inode
{
	struct inode vfs_inode;
	int offset;
//    int length;
};

umode_t get_mode(struct super_block* sb, struct disk_position* pos)
{
	char data[8] = { 0 };
	char type;
	short d;
	short v = 0;
	mode_t mode;
	int err = get_tar_field(sb,pos,ind_file_mode,data);
	if (err)
	{
		printk("+ aufs error -- can not get field mode\n");
		return (umode_t)0;
	}
	err = get_tar_field(sb,pos,ind_file_typeflag,&type);
	if (err)
	{
		printk("+ aufs error -- can not get field typeflag\n");
		return (umode_t)0;
	}
	
	sscanf(data,"%hi",&d);
	switch(type)
	{
		case '0':
			v = S_IFREG;
			break;
		case '5':
			v = S_IFDIR;
			break;
	}
	mode = v | d;
	return mode;
}

kuid_t get_uid(struct super_block* sb,struct disk_position* disk_pos)
{	
	char data[8] = {0};
	kuid_t uid = { -1  };
	int err = get_tar_field(sb,disk_pos,ind_file_uid,data);
	if (err)
	{
		printk("+ aufs error -- can not get field uid\n");
		return uid;
	}
	sscanf(data,"%u",&uid.val);    
	return uid;
}

kgid_t get_gid(struct super_block* sb, struct disk_position* disk_pos)
{
	char data[8] = {0};
	kgid_t gid = { -1 };
	int err = get_tar_field(sb,disk_pos,ind_file_gid,data);
	if (err)
	{
		printk("+ aufs error -- can not get field gid\n");
		return gid;
	}
	sscanf(data,"%u",&gid.val);    
	return gid;
}

int get_size(struct super_block* sb,struct disk_position* disk_pos)
{
	char data[12] = { 0 };
	int len;
	int err = get_tar_field(sb,disk_pos,ind_file_size,data);
	if (err)
	{
		printk("+ aufs error -- can not get field gid\n");
		return 0;
	}
	//printk("data: %s\n",data);
	sscanf(data,"%i",&len);
	//printk("len: %i\n",len);
	return len;
}

int get_tar_field(struct super_block* sb,struct disk_position* disk_pos,int field,char* put_place)
{
	int start_file = disk_pos -> offset_in_first;
	int offset = start_file + off_lens[field].offset;
	int required_field_length = off_lens[field].length;
	memcpy(put_place, disk_pos->buffers[0].buffer + offset,required_field_length);
	return 0;
}
void fill_node(struct inode* file_node, struct disk_position* disk_pos)
{
        umode_t mode = get_mode(file_node->i_sb, disk_pos);
	kuid_t uid =  get_uid(file_node -> i_sb, disk_pos);
	kgid_t gid =  get_gid(file_node -> i_sb, disk_pos);
	size_t size = get_size(file_node->i_sb, disk_pos);

	file_node -> i_mode = mode;
	file_node -> i_uid = uid;
	file_node -> i_gid = gid;
	file_node -> i_size = size;

	file_node -> i_atime = file_node -> i_ctime = file_node -> i_mtime = CURRENT_TIME;
//	file_node->i_fop = &aufs_file_operations;
	//file_node->i_data.a_ops = &aufs_aops;
	file_node->i_op = &aufs_inode_ops;
	//return ;
	switch(mode & S_IFMT)
	{
		case S_IFREG:
		    file_node->i_fop = &generic_ro_fops;
		    file_node->i_data.a_ops = &aufs_aops;
		    break;
		case S_IFDIR:
		    file_node->i_fop = &aufs_dir_operations;
		    //inode->i_op = &aufs_inode_operations;
		    break;
	}
	return;
}

int init_disk_position(struct disk_position* disk_pos)
{
	disk_pos->buffers = kmalloc(2 * sizeof(struct BLOCK_BUFFER),GFP_KERNEL);
	if (!disk_pos->buffers)
	{
		return 1;
	}
	disk_pos->last_fill_buffer_index  = -1;
	disk_pos-> offset_in_first = 0;
	disk_pos->first_block_no = -1;
	return 0;
}
void deinit_disk_position(struct disk_position* disk_pos)
{
	kfree(disk_pos->buffers);
}
struct dentry* aufs_lookup(struct inode* dir,struct dentry* name,unsigned flags)
{
	// we should put inode in dentry(d_add)
	int inode_no = 0;
	struct disk_position disk_pos;
	struct inode* file;
	struct super_block* sb = dir->i_sb;
	int err = init_disk_position(&disk_pos);
	if (err)
	{
		return NULL;
	}
	inode_no = find_file(name, sb, &disk_pos);
	
	if (inode_no < 0 )
	{
		printk("+ aufs seems like file wasn't found\n");
		goto out;
	}
	
	file = aufs_inode_get(sb,inode_no);
	
	if (IS_ERR(file))
	{
		printk("+ aufs inode is ERR!\n");
		goto out;
	}
	fill_node(file,&disk_pos);
	name->d_inode = file;

	if ( !name->d_sb->s_d_op )
	{
		//printk("dentry ops is NULL\n");
		d_set_d_op(name,&aufs_dentry_ops);
	}
	d_add(name,file);
out:
	deinit_disk_position(&disk_pos);
//	printk("node no is %d\n",file->i_ino);
	return NULL;
}
int is_child_of_first_level(const char* dir,const char* subdir)
{
	char prefix_of_subdir[100] = { 0 };
	int slash_count = 0;
	int len_dir = strlen(dir);
	int len_subdir = strlen(subdir);
	strncpy(prefix_of_subdir,subdir,len_dir);
	//printk("dir is %s\n",dir);
	//printk("subdir is %s\n",subdir);
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
		//printk("slash count is %d\n",slash_count);
	}
	
	return (len_dir + 1 )*(slash_count == 1);
}

int aufs_readdir(struct file* f,void * d,filldir_t fldt)
{
	char file_name[101] = { 0 } ;
	char open_file_name[101] = {0};
	struct inode* file_node = file_inode(f);
	struct disk_position disk_pos;
	struct super_block* sb = file_node -> i_sb;
	int file_count_stop = 30;
	int file_count = f->f_pos;
	int i;
	int err;
	if (file_count)
	{
		return 0;
	}
	
	if (file_node->i_mode & S_IFREG)
	{
		return -EINVAL;
	}
	init_disk_position(&disk_pos);
	
	printk("hello from readdir \n");
	
	get_full_name(f->f_path.dentry,file_name + 1);
	file_name[0] = '/';
	printk("%s\n",file_name);
	
	while(file_count < file_count_stop)
	{
		for (i = 0;i< 101;++i)
		{
			open_file_name[i] = 0;
		}
		err = get_next_file(sb,&disk_pos,open_file_name+1);
		if (err)
		{
			break;
		}
		file_count ++;
		open_file_name[0] = '/';
		int tail_of_name = is_child_of_first_level(file_name,open_file_name);
		if (tail_of_name)
		{
			//printk("%d %d hello! %s\n",file_count,open_file_name[1],open_file_name);
			fldt(d,open_file_name+tail_of_name -(file_name[1]==0),off_lens[0].length,f->f_pos,1,file_node->i_mode);
			//break;
		}
	}
	deinit_disk_position(&disk_pos);
	f -> f_pos = file_count;
	return 0;
}
/*
ssize_t aufs_read(struct file* f, const char __user * buf,size_t len,loff_t* ppos)
{
	char file_name[100] = { 0 } ;
	char open_file_name[100] = {0};
	struct inode* file_node = file_inode(f);
	struct disk_position disk_pos;
	struct super_block* sb = file_node -> i_sb;
	int i;
	int err;
	if (file_node->i_mode & S_IFREG)
	{
		//return -EINVAL;
	}
	err = init_disk_position(&disk_pos);
	if (err)
	{
		return 0;
	}
	


	get_full_name(f->f_path.dentry,file_name);
	printk("hello from read function %s\n",file_name);
	find_file(f->f_path.dentry,sb,&disk_pos);
	int start_file = disk_pos.offset_in_first + 512 + *ppos;
	int size = get_size(sb,&disk_pos);
	int block_index = 0;
	if (size - *ppos <=0)
	{
		return 0;
	}

	while (start_file >= 4096)
	{
		start_file -= 4096;
		++ block_index;
	}
	printk("aufs read debug:\n");
	printk("block_index is%d\n",block_index);
	printk("file size is %d\n",size);
	printk("start file is %d\n",start_file);
	printk("block_no start %d\n",disk_pos.first_block_no);
	printk("aufs read debug stop:\n");
	int ret = 0;
	int remain = size - *ppos;
	int offset = start_file;
	while(remain > 0)
	{
		int how_much = BLOCK_LENGTH - offset > remain ? remain : BLOCK_LENGTH - offset;
		printk("how much -- %d\n",how_much);
		if (ret + how_much > len)
		{
			break;
		}
		memcpy(buf + ret,disk_pos.buffers[block_index].buffer+offset,how_much);
		remain -= how_much;
		block_index++;
		ret+=how_much;
		offset = 0;
	}
	deinit_disk_position(&disk_pos);
	*ppos += ret;
	return ret;
}
*/
struct inode* aufs_inode_get(struct super_block* sb,int no)
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
        inode ->i_fop = &aufs_dir_operations;
        return inode;
    }

	if (!inode -> i_fop)
	{
	    printk("i was hree\n");
	    //inode ->i_fop = &aufs_dir_operations;
	}
	unlock_new_inode(inode);
	return inode;
}


static void aufs_put_super(struct super_block* sb)
{
    printk("aufs super block destroyed\n");
}

static struct super_operations const aufs_super_ops =
{
    .put_super = aufs_put_super,
};

static int aufs_fill_sb(struct super_block * sb, void * data,int silent)
{
    struct inode * root = NULL;

    sb->s_magic = 0x34342343434;
    sb->s_op = &aufs_super_ops;
    if (!sb_set_blocksize(sb,BLOCK_LENGTH))
    {
	return -EINVAL;
    }
    //sb->s_blocksize = BLOCK_LENGTH ;
    root = aufs_inode_get(sb,0); //hardcode 0 number is root node;

    if (IS_ERR(root))
    {
	printk("inode allocation failed...\n");
	return -ENOMEM;
    }
    aufs_inode_init(root); // only init inode operations.
    inode_init_owner(root,NULL,S_IFDIR);

    sb->s_root = d_make_root(root); // from inode to dentry.
    
    if (!sb->s_root)
    {
	printk("root creation failed\n");
	return -ENOMEM;
    }
    return 0;
}

static struct dentry* aufs_mount ( struct file_system_type* type,int flags,char const* dev,void* data)
{
    struct dentry* const entry = mount_bdev(type,flags,dev,data,aufs_fill_sb);
    if (IS_ERR(entry))
    {
	printk("aufs mounting failed\n");
    }
    else
    {
	printk("aufs_success...");
    }
    return entry;
}

static struct file_system_type aufs_type =
{
    .owner = THIS_MODULE,
    .name = "aufs",
    .mount = aufs_mount,
    .kill_sb = kill_block_super,
    .fs_flags = FS_REQUIRES_DEV,
};

static int __init init_fs(void)
{
    int ret;
    init_off_lens();
    ret = register_filesystem(&aufs_type);
    if (0!= ret)
    {
	printk("can not register aufs\n");
	return ret;
    }
    printk("aufs module loaded\n");
    return 0;
}
static void __exit exit_fs(void)
{
    int ret = unregister_filesystem(&aufs_type);
    if (ret != 0)
    {
	printk("unregister aufs failed..\n");
	return;
    }
    printk("module unloaded\n");
}
module_init(init_fs);
module_exit(exit_fs);
