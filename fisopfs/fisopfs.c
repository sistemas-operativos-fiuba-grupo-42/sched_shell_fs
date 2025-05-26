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


int get_inode_index(const char *path)
{
	for (int i = 0; i < MAX_INODES; i++) {
		if (strcmp(fs.inodes[i].path, path) == 0) {
			return i;
		}
	}
	return -1;
}

int get_unused_inode() {
	for (int i = 0; i < MAX_INODES; i++) {
		if (!fs.inodes[i].valid) {
			return i;
		}
	}
	return -1;
}


/** Get file attributes.
 *
 * Similar to stat().  The 'st_dev' and 'st_blksize' fields are
 * ignored.	 The 'st_ino' field is ignored except if the 'use_ino'
 * mount option is given.
 */
static int
fisopfs_getattr(const char *path, struct stat *st)
{
	printf("[debug] fisopfs_getattr - path: %s\n", path);

	int inode_index = get_inode_index(path);
	if (inode_index == -1){
		fprintf(stderr, "[debug] fisopfs_getattr - path: %s\n", strerror(ENOENT));
		return -ENOENT;
	}
	inode_t inode = fs.inodes[inode_index];

	st->st_uid = inode.uid;
	st->st_gid = inode.gid;
	st->st_mode = inode.mode;

	st->st_size = inode.size;
	st->st_nlink = inode.nlink;

	st->st_atime = inode.last_access;
	st->st_mtime = inode.last_modified;
	st->st_ctime = inode.creation_time;

	return 0;
}

/**
 * Initialize filesystem
 *
 * The return value will passed in the private_data field of
 * fuse_context to all file operations and as a parameter to the
 * destroy() method.
 *
 * Introduced in version 2.3
 * Changed in version 2.6
	void *(*init) (struct fuse_conn_info *conn);
 */
void* fisopfs_init(struct fuse_conn_info *conn){
	printf("[debug] fisopfs_init\n");

	FILE *file = fopen(filedisk, "r");
	if (file == NULL) {
		memset(&fs, 0, sizeof(fs));
		inode_t *root = &(fs.inodes[0]);
		root->valid = 1;
		strcpy(root->path, "/");
		root->is_directory = true;
		root->uid = getuid();
		root->gid = getgid();
		root->mode = __S_IFDIR | 0755;
		root->creation_time = time(NULL);
		root->last_access = time(NULL);
		root->last_modified = time(NULL);
		root->nlink = 1;
	} else {
		size_t bytes_read = fread(&fs, sizeof(fs), 1, file);
		if (bytes_read != 1) {
			fprintf(stderr, "[debug] fisopfs_init %s\n", strerror(errno));
			fclose(file);
			return NULL;
		}
		fclose(file);
	}
	return 0;
}

/**
 * Clean up filesystem
 *
 * Called on filesystem exit.
 *
 * Introduced in version 2.3
 */
void destroy(void *fs) {
}


/** Function to add an entry in a readdir() operation
 *
 * @param buf the buffer passed to the readdir() operation
 * @param name the file name of the directory entry
 * @param stat file attributes, can be NULL
 * @param off offset of the next entry or zero
 * @return 1 if buffer is full, zero otherwise
*/

static int
fisopfs_readdir(const char *path,
                void *buffer,
                fuse_fill_dir_t filler,
                off_t offset,
                struct fuse_file_info *fi)
{
	printf("[debug] fisopfs_readdir - path: %s\n", path);

	// Los directorios '.' y '..'
	filler(buffer, ".", NULL, 0);
	filler(buffer, "..", NULL, 0);

	int inode_index = get_inode_index(path);
	if(inode_index == -1){
		fprintf(stderr, "[debug] fisopfs_readdir - path: %s\n", strerror(ENOENT));
		return -ENOENT;
	}

	inode_t *inode = &(fs.inodes[inode_index]);

	if(!inode->is_directory){
		fprintf(stderr, "[debug] fisopfs_readdir - %s\n", strerror(ENOTDIR));
		return -ENOTDIR;
	}
	inode->last_access = time(NULL);

	for(int i = 0; i < MAX_INODES; i++){
		if(fs.inodes[i].valid){
			size_t n = strlen(inode->path);
			if(strncmp(inode->path, fs.inodes[i].path, n) == 0){
				filler(buffer, fs.inodes[i].path+n, NULL, 0);
			}
		}
	}

	return -ENOENT;
}

#define MAX_CONTENIDO 100
static char fisop_file_contenidos[MAX_CONTENIDO] = "hola fisopfs!\n";

	/** Read data from an open file
	 *
	 * Read should return exactly the number of bytes requested except
	 * on EOF or error, otherwise the rest of the data will be
	 * substituted with zeroes.	 An exception to this is when the
	 * 'direct_io' mount option is specified, in which case the return
	 * value of the read system call will reflect the return value of
	 * this operation.
	 *
	 * Changed in version 2.2
	 */

static int
fisopfs_read(const char *path,
             char *buffer,
             size_t size,
             off_t offset,
             struct fuse_file_info *fi)
{
	printf("[debug] fisopfs_read - path: %s, offset: %lu, size: %lu\n",
	       path,
	       offset,
	       size);


	int inode_index = get_inode_index(path);
	if(inode_index == -1){
		fprintf(stderr, "[debug] fisopfs_read - path: %s\n", strerror(ENOENT));
		return -ENOENT;
	}

	inode_t *inode = &(fs.inodes[inode_index]);

	if(inode->is_directory){
		fprintf(stderr, "[debug] fisopfs_read - %s\n", strerror(EISDIR));
		return -EISDIR;
	}

	inode->last_access = time(NULL);
	char *data = inode->data;

	if (offset > inode->size) {
		fprintf(stderr, "[debug] fisopfs_read - %s\n", strerror(EINVAL));
		return -EINVAL;
	}

	size_t bytes = size;
	if (inode->size - offset < bytes){
		bytes = inode->size - offset;
	}

	memcpy(buffer, data + offset, bytes);

	return bytes;
}

int fisopfs_create(const char *path, mode_t mode, struct fuse_file_info *info)
{
	printf("[debug] fisopfs_create - path: %s, mode: %o\n", path, mode);

	if (strlen(path) - 1 > MAX_PATH_SIZE){
		fprintf(stderr, "[debug] fisopfs_create - %s\n", strerror(ENAMETOOLONG));
		return -ENAMETOOLONG;
	}

	int inode_index = get_unused_inode();
	if (inode_index == -1){
		fprintf(stderr, "[debug] fisopfs_create - %s\n", strerror(ENOMEM));
		return -ENOMEM;
	}


	inode_t *inode = &(fs.inodes[inode_index]);

	inode->mode = __S_IFREG | (mode & 0777);
	inode->uid = getuid();
	inode->gid = getgid();
	inode->creation_time = time(NULL);
	inode->last_access = time(NULL);
	inode->last_modified = time(NULL);
	inode->size = 0;
	inode->is_directory = false;

	return 0;
}

static int fisopfs_write(const char *path,
const char *buffer,
size_t size,
off_t offset,
struct fuse_file_info *fi)
{

	printf("[debug] fisopfs_write - path: %s, offset: %lu, size: %lu\n", path, offset, size);

	if(offset + size > MAX_CONTENIDO){
		fprintf(stderr, "[debug] fisopfs_write - %s\n", strerror(E2BIG));
		return -E2BIG;
	}

	int idx = get_inode_index(path);
	if (idx == -1) {
		fprintf(stderr, "[debug] fisopfs_read - path: %s\n", strerror(ENOENT));
		return -ENOENT;
	}

	inode_t* inode = &(fs.inodes[idx]);


	inode->last_access = time(NULL);
	inode->creation_time = time(NULL);
}

static struct fuse_operations operations = {
	.getattr = fisopfs_getattr,
	.readdir = fisopfs_readdir,
	.read = fisopfs_read,
	.init = fisopfs_init,
	.create = fisopfs_create,
	.write = fisopfs_write
};


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
