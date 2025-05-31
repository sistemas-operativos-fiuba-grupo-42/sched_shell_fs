#ifndef FS_H
#define FS_H

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <stdbool.h>

#define MAX_INODES 1024
#define MAX_FILE_SIZE 4096
#define MAX_PATH_SIZE 256

typedef struct inode
{
    bool is_directory;
    bool valid;
	mode_t mode;
	gid_t gid;
	uid_t uid;
    size_t size;
    time_t creation_time;
    time_t last_access;
    time_t last_modified;
    int nlink;
	char path[MAX_PATH_SIZE];
    char data[MAX_FILE_SIZE];
} inode_t;

struct fs
{
    inode_t inodes[MAX_INODES];
};

// auxiliares
int get_inode_index(const char *path);
int get_unused_inode();
int get_nfiles(const char* path);

void* fs_init(const char *filedisk);
void fs_destroy(const char *filedisk);
int fs_getattr(const char *path, struct stat *st);
int fs_read(const char *path, char *buffer, size_t size, off_t offset);
int fs_create(const char *path, mode_t mode);
int fs_write(const char *path, const char *buffer, size_t size, off_t offset);
int fs_mkdir(const char *path, mode_t mode);
int fs_utimens(const char *path, const struct timespec tv[2]);
int fs_truncate(const char *path, off_t length);
int fs_unlink(const char *path);
int fs_rmdir(const char *path);
int fs_flush(const char *path, char *filedisk);

int get_inode_in_directory(const char *path, int *index, char *entry_name);
int fs_is_directory(const char *path);
#endif
