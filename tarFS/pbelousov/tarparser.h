#ifndef __TARPARSER_H__
#define __TARPARSER_H__ 

#define NAME 		0
#define MODE 		1
#define UID 		2
#define GID 		3
#define SIZE 		4
#define MTIME 		5
#define CHKSUM 		6
#define TYPEFLAG 	7
#define LINKNAME 	8
#define MAGIC 		9
#define VERSION 	10
#define UNAME 		11
#define GNAME 		12
#define DEVMAJOR 	13
#define DEVMINOR 	14
#define PREFIX 		15

#define SIZE_TAR_HEADER 	16 /* Number of fields in tar header */

struct off_len{
	unsigned length;
	unsigned offset;
};

extern const struct off_len ustar_offsets[];

extern const unsigned size_name_tar_file;

extern char* get_tar_field(char*, char*, int);
extern s64 get_tar_octal_field(char*, unsigned);
extern size_t get_tar_name(char*, char*);
extern size_t get_tar_size(char*);
extern int get_tar_gid(char*);
extern int get_tar_uid(char*);
extern int get_tar_mode(char*);
extern int get_tar_type(char*);
extern struct timespec get_tar_mtime(char*);

#endif /* __TARPARSER_H__ */