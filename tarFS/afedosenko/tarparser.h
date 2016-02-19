#pragma once
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
#define HEADERSIZE 	16

struct off_len{
	int length;
	int offset;
};
struct tar_header{
	char* name;
	int mode;
	int uid;
	int gid;
	int size;
	int chksum;
	int typeflag;

};
static int header_size = 0;
static struct off_len offsets[HEADERSIZE];
void init_offsets(void);
int get_name(char* data, char* name);
char* get_field(char* to, char* from, int field);
long long get_long_field(char*, int);
int get_header_size(void);
int get_max_length(void);
int get_tar_gid(char*);
int get_tar_uid(char*);
int get_mode(char*);
int get_type(char*);
long long get_size(char*);
struct timespec get_tar_mtime(char*);