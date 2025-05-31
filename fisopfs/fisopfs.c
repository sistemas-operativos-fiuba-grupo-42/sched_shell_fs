#define FUSE_USE_VERSION 30
#include <fuse.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include "fs.h"

#define DEFAULT_FILE_DISK "persistence_file.fisopfs"

char *filedisk = DEFAULT_FILE_DISK;

void *
fisopfs_init(struct fuse_conn_info *conn)
{
	return fs_init(filedisk);
}

void
fisopfs_destroy(void *private_data)
{
	return fs_destroy(filedisk);
}

int
fisopfs_getattr(const char *path, struct stat *stbuf)
{
	return fs_getattr(path, stbuf);
}

int
fisopfs_readdir(const char *path,
                void *buffer,
                fuse_fill_dir_t filler,
                off_t offset,
                struct fuse_file_info *fi)
{
	printf("[debug] fisopfs_readdir - path: %s\n", path);

	// Add . and .. entries
	filler(buffer, ".", NULL, 0);
	filler(buffer, "..", NULL, 0);

	int res = fs_is_directory(path);
	if (res < 0) {
		return res;  // return errors
	}

	char full_path[MAX_PATH_SIZE];
	if (strcmp(path, "/") == 0) {
		strcpy(full_path, "/");
	} else {
		snprintf(full_path, sizeof(full_path), "%s/", path);
	}

	char entry_name[MAX_PATH_SIZE];
	int index = 0;
	res = get_inode_in_directory(full_path, &index, entry_name);
	while (res > 0) {
		filler(buffer, entry_name, NULL, 0);
		res = get_inode_in_directory(full_path, &index, entry_name);
	}
	return 0;
}

int
fisopfs_read(const char *path,
             char *buffer,
             size_t size,
             off_t offset,
             struct fuse_file_info *fi)
{
	return fs_read(path, buffer, size, offset);
}

int
fisopfs_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
	return fs_create(path, mode);
}

int
fisopfs_write(const char *path,
              const char *buffer,
              size_t size,
              off_t offset,
              struct fuse_file_info *fi)
{
	return fs_write(path, buffer, size, offset);
}

int
fisopfs_mkdir(const char *path, mode_t mode)
{
	return fs_mkdir(path, mode);
}

int
fisopfs_utimens(const char *path, const struct timespec tv[2])
{
	return fs_utimens(path, tv);
}

int
fisopfs_truncate(const char *path, off_t length)
{
	return fs_truncate(path, length);
}

int
fisopfs_unlink(const char *path)
{
	return fs_unlink(path);
}

int
fisopfs_rmdir(const char *path)
{
	return fs_rmdir(path);
}

int
fisopfs_flush(const char *path, struct fuse_file_info *fi)
{
	return fs_flush(path, filedisk);
}

static struct fuse_operations operations = { .init = fisopfs_init,
	                                     .getattr = fisopfs_getattr,
	                                     .readdir = fisopfs_readdir,
	                                     .read = fisopfs_read,
	                                     .create = fisopfs_create,
	                                     .write = fisopfs_write,
	                                     .mkdir = fisopfs_mkdir,
	                                     .utimens = fisopfs_utimens,
	                                     .truncate = fisopfs_truncate,
	                                     .unlink = fisopfs_unlink,
	                                     .rmdir = fisopfs_rmdir,
	                                     .destroy = fisopfs_destroy,
	                                     .flush = fisopfs_flush };

int
main(int argc, char *argv[])
{
	for (int i = 1; i < argc - 1; i++) {
		if (strcmp(argv[i], "--filedisk") == 0) {
			filedisk = argv[i + 1];

			// We remove the argument so that fuse doesn't use our
			// argument or name as folder.
			// Equivalent to a pop.
			for (int j = i; j < argc - 1; j++) {
				argv[j] = argv[j + 2];
			}

			argc = argc - 2;
			break;
		}
	}

	return fuse_main(argc, argv, &operations, NULL);
}
