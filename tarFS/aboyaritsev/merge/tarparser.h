#pragma once
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

extern size_t widthes[];
extern const int META_INFO_SIZE;

void init_off_lens(void);

umode_t get_mode(struct super_block* sb, struct disk_position* pos);

kuid_t get_uid(struct super_block* sb,struct disk_position* disk_pos);

kgid_t get_gid(struct super_block* sb, struct disk_position* disk_pos);

int get_size(struct super_block* sb,struct disk_position* disk_pos);
int get_tar_field(struct super_block* sb,struct disk_position* disk_pos,int field,char* put_place);
