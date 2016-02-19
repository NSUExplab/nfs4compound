#include "tarparser.h"
#include <linux/slab.h>
#include <uapi/linux/stat.h>

void init_offsets(void)
{
	int i = 1;
	offsets[NAME].length = 100;
	offsets[NAME].offset = 0;
	offsets[MODE].length = 8;
	offsets[UID].length = 8;
	offsets[GID].length = 8;
	offsets[SIZE].length = 12;
	offsets[MTIME].length = 12;
	offsets[CHKSUM].length = 8;
	offsets[TYPEFLAG].length = 1;
	offsets[LINKNAME].length = 100;
	offsets[MAGIC].length = 6;
	offsets[VERSION].length = 2;
	offsets[UNAME].length = 32;
	offsets[GNAME].length = 32;
	offsets[DEVMAJOR].length = 8;
	offsets[DEVMINOR].length = 8;
	offsets[PREFIX].length = 155;
	for(i = 1; i < HEADERSIZE; i++){
		offsets[i].offset = offsets[i-1].length + offsets[i-1].offset;
		header_size += offsets[i-1].length;
	}
	header_size = offsets[HEADERSIZE-1].length + offsets[HEADERSIZE-1].offset;
}

char* get_field(char* to, char* from, int field)
{
	strncpy(to, from + offsets[field].offset, offsets[field].length);
	return to;
}
/* 	gets filename from tar.
	char* name length must be >= 256. */
int get_name(char* data, char* name){
	strcpy(name, data + offsets[PREFIX].offset);
	strcpy(name + strlen(name), data + offsets[NAME].offset);
	return strlen(name);
}
long long get_long_field(char* data, int field_name){
	long long field;
	char mem[offsets[field_name].length + 1];
	mem[0] = '0';
	strcpy(mem + 1, data + offsets[field_name].offset);
	sscanf(mem, "%lli", &field);
	return field;
}
struct timespec get_tar_mtime(char* data)
{
	long long mtime = get_long_field(data, MTIME), nanomtime, nanosec = 1000000000;
	nanomtime = mtime * nanosec;
	return ns_to_timespec(nanomtime);
}
int get_tar_uid(char* data)
{
	return get_long_field(data, UID);
}
int get_tar_gid(char* data)
{
	return get_long_field(data, GID);
}
long long get_size(char* data)
{
	return get_long_field(data, SIZE);
}
int get_mode(char* data)
{
	return get_long_field(data, MODE);
}
int get_type(char* data)
{
	char type;
	int typebit;
	sscanf(data + offsets[TYPEFLAG].offset, "%c", &type);
	switch (type){
		case '0':
			typebit = S_IFREG;
			break;
		case '1':
			typebit = S_IFLNK;
			break;
		case '3':
			typebit = S_IFCHR;
			break;
		case '4':
			typebit = S_IFBLK;
			break;
		case '5':
			typebit = S_IFDIR;
			break;
		case '6':
			typebit = S_IFIFO;
			break;
		default:
			typebit = S_IFREG;
	}
	return typebit;
}
int get_header_size(void)
{
	return header_size + 12;
}
int get_max_length(void)
{
	return 155;
}