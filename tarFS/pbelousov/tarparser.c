#include <linux/slab.h>
#include <uapi/linux/stat.h> /* mode file constants */

#include "tarparser.h"

const struct off_len ustar_offsets[] = {
	{100, 0}, 	// NAME
	{8, 100}, 	// MODE
	{8, 108},	// UID
	{8, 116},	// GID
	{12, 124},	// SIZE
	{12, 136},	// MTIME
	{8, 148},	// CHKSUM
	{1, 156},	// TYPEFLAG
	{100, 157},	// LINKNAME
	{6, 257},	// MAGIC
	{2, 263},	// VERSION
	{32, 265},	// UNAME
	{32, 297},	// GNAME
	{8, 329},	// DEVMAJOR
	{8, 337},	// DEVMINOR
	{155, 345},	// PREFIX
};

// NAME length + PREFIX length
const unsigned size_name_tar_file = 255;

char* get_tar_field(char* to, char* from, int field)
{
	return strncpy(to, from + ustar_offsets[field].offset, ustar_offsets[field].length);
}

s64 get_tar_octal_field(char* data, unsigned field_name){
	s64 field;
	char mem[ustar_offsets[field_name].length + 1];
	mem[0] = '0';
	strcpy(mem + 1, data + ustar_offsets[field_name].offset);
	sscanf(mem, "%lli", &field);
	return field;
}

/* 
 * char* name length must be >= 256.
 * return %NUL terminated string.
 */
size_t get_tar_name(char* data, char* name){
	get_tar_field(name, data, PREFIX);
	get_tar_field(name + strlen(name), data, NAME);
	name[255] = '\0';
	return strlen(name);
}

struct timespec get_tar_mtime(char* data)
{
	return ns_to_timespec(NSEC_PER_SEC * get_tar_octal_field(data, MTIME));
}

int get_tar_uid(char* data)
{
	return get_tar_octal_field(data, UID);
}

int get_tar_gid(char* data)
{
	return get_tar_octal_field(data, GID);
}

size_t get_tar_size(char* data)
{
	return get_tar_octal_field(data, SIZE);
}

int get_tar_mode(char* data)
{
	return get_tar_octal_field(data, MODE);
}

int get_tar_type(char* data)
{
	char type;
	int typebit;
	sscanf(data + ustar_offsets[TYPEFLAG].offset, "%c", &type);
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