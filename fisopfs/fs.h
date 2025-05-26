#ifndef FS_H
#define FS_H

#include <fuse.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdbool.h>

#define MAX_INODES 1024
#define MAX_FILE_SIZE 4096
#define MAX_PATH_SIZE 256

typedef struct inode
{
    bool is_directory;
    bool valid;
	mode_t mode;
	int gid;
	int uid;
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

struct fs fs;

/*
int fisopfs_init();
int fisops_destroy();
int fisopfs_readdir();
int fisopfs_read();

int fisopfs_opendir(const char *, struct fuse_file_info *);
int mkdir(const char *);
int rmdir(const char *);
int unlink(const char *);
int symlink(const char *, const char *);
int link(const char *, const char *);
*/

#endif
