#include<linux/fs.h>

#include"tarfs.h"
#include"tarparser.h"

size_t widthes[] = { 100,8,8,8,12,12,8,1,100,6,2,32,32,8,8,167};
struct off_len off_lens[16];
const int META_INFO_SIZE = 512;

void init_off_lens(void)
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
		printk("+ tarfs error -- can not get field mode\n");
		return (umode_t)0;
	}
	err = get_tar_field(sb,pos,ind_file_typeflag,&type);
	if (err)
	{
		printk("+ tarfs error -- can not get field typeflag\n");
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
		printk("+ tarfs error -- can not get field uid\n");
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
		printk("+ tarfs error -- can not get field gid\n");
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
		printk("+ tarfs error -- can not get field gid\n");
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
 
