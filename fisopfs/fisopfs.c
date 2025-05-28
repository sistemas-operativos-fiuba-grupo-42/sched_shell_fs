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
		if (strcmp(fs.inodes[i].path, path) == 0 && fs.inodes[i].valid == true) {
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

int get_nfiles(const char* path) {
	int cant = 0;
	size_t n = strlen(path);
	for (int i = 0; i < MAX_INODES; i++){
		inode_t inode = fs.inodes[i];
		if(strncmp(inode.path, path, n) == 0 && inode.valid) {
			cant++;
		}
	}
	return cant - 1;
}

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

void fisopfs_destroy(void *f) {
	printf("[debug] fisopfs_destroy\n");

	FILE *file = fopen(filedisk, "w");
	if (file == NULL){
		fprintf(stderr, "[debug] fisopfs_destroy %s\n", strerror(errno));
		return;
	}

	size_t bytes = fwrite(&fs, sizeof(fs), 1, file);
	if (bytes != 1){
		fprintf(stderr, "[debug] fisopfs_destroy %s\n", strerror(errno));
		return;
	}
	fflush(file);
	fclose(file);
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
	if (inode_index == -1){
		fprintf(stderr, "[debug] fisopfs_readdir - path: %s\n", strerror(ENOENT));
		return -ENOENT;
	}

	inode_t *inode = &(fs.inodes[inode_index]);

	if(!inode->is_directory){
		fprintf(stderr, "[debug] fisopfs_readdir - %s\n", strerror(ENOTDIR));
		return -ENOTDIR;
	}
	inode->last_access = time(NULL);

	char full_path[MAX_PATH_SIZE];
	if (strcmp(path, "/") == 0) {
	    strcpy(full_path, "/");
	} else {
        snprintf(full_path, sizeof(full_path), "%s/", path);
	}

	size_t n = strlen(full_path);

	for (int i = 0; i < MAX_INODES; i++){
		if (fs.inodes[i].valid) {
    		if (strncmp(full_path, fs.inodes[i].path, n) == 0 && strcmp(fs.inodes[i].path, path) != 0) {
                const char *entry = fs.inodes[i].path + n;
                if (strchr(entry, '/') == NULL) {
                    printf("[debug] adding entry: '%s'\n", entry);
                    filler(buffer, entry, NULL, 0);
                }
            }
		}
	}

	return 0;
}

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

	strncpy(inode->path, path, MAX_PATH_SIZE);
	inode->valid = true;
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

	if(offset + size > MAX_FILE_SIZE){
		fprintf(stderr, "[debug] fisopfs_write - %s\n", strerror(E2BIG));
		return -E2BIG;
	}

	int idx = get_inode_index(path);
	if (idx == -1) {
		fprintf(stderr, "[debug] fisopfs_write - path: %s\n", strerror(ENOENT));
		return -ENOENT;
	}

	inode_t* inode = &(fs.inodes[idx]);
	if (offset == 0) {
		inode->size = 0;
	}
	memcpy(inode->data + offset, buffer, size);

	if (offset + size > inode->size) {
		inode->size = offset + size;
	}
	inode->last_access = time(NULL);
	inode->last_modified = time(NULL);
	return (int) size;
}

static int fisopfs_mkdir(const char *path, mode_t mode)
{
    printf("[debug] fisopfs_mkdir - path: %s, mode: %o\n", path, mode);
    if (strlen(path) - 1 > MAX_PATH_SIZE) {
        fprintf(stderr, "[debug] fisopfs_mkdir - %s\n", strerror(ENAMETOOLONG));
        return -ENAMETOOLONG;
    }
    int inode_index = get_unused_inode();
    if (inode_index == -1) {
        fprintf(stderr, "[debug] fisopfs_mkdir - %s\n", strerror(ENOMEM));
        return -ENOMEM;
    }

    char parent_path[MAX_PATH_SIZE];
    strncpy(parent_path, path, MAX_PATH_SIZE);
    char *last_slash = strrchr(parent_path, '/');
    if (last_slash != parent_path) {
        *last_slash = '\0';
        if (get_inode_index(parent_path) == -1) {
            fprintf(stderr, "[debug] fisopfs_mkdir - parent path: %s\n", strerror(ENOENT));
            return -ENOENT;
        }
    }
    inode_t *inode = &fs.inodes[inode_index];
    inode->valid = true;
    strncpy(inode->path, path, MAX_PATH_SIZE);
    inode->is_directory = true;
    inode->mode = __S_IFDIR | (mode & 0777);
    inode->uid = getuid();
    inode->gid = getgid();
    inode->creation_time = time(NULL);
    inode->last_access = time(NULL);
    inode->last_modified = time(NULL);
    inode->nlink = 1;
    inode->size = 0;
    return 0;
}

static int fisopfs_utimens(const char *path, const struct timespec tv[2]) {
    printf("[debug] utimens called for path: %s\n", path);

    int inode_index = get_inode_index(path);
    if (inode_index == -1) {
        return -ENOENT;
    }

    inode_t *inode = &fs.inodes[inode_index];

    inode->last_access = tv[0].tv_sec;
    inode->last_modified = tv[1].tv_sec;

    return 0;
}

int fisopfs_truncate(const char *path, off_t length){
	printf("[debug] fisopfs_truncate - path: %s length: %ld\n", path, length);
	int inode_index = get_inode_index(path);
	if (inode_index == -1) {
		fprintf(stderr, "[debug] fisopfs_truncate: %s\n", strerror(ENOENT));
        return -ENOENT;
    }

	inode_t *inode = &fs.inodes[inode_index];

	if (length > inode->size) {
		fprintf(stderr, "[debug] fisopfs_truncate - %s\n", strerror(E2BIG));
		return -E2BIG;
	}

	inode->size = length;
	inode->last_modified = time(NULL);
	inode->last_access = time(NULL);

	return 0;
}

static int fisopfs_unlink(const char *path){
	printf("[debug] fisopfs_unlink - path: %s \n", path);
	int inode_index = get_inode_index(path);
	if (inode_index == -1) {
		fprintf(stderr, "[debug] fisopfs_unlink: %s\n", strerror(ENOENT));
        return -ENOENT;
    }

	inode_t *inode = &(fs.inodes[inode_index]);
	if (inode->is_directory){
		fprintf(stderr, "[debug] fisopfs_unlink - %s\n", strerror(EISDIR));
		return -EISDIR;
	}

	inode->valid = false;
	inode->size = 0;

	return 0;
}

int fisopfs_rmdir(const char *path) {
	printf("[debug] fisopfs_rmdir - path: %s \n", path);
	int inode_index = get_inode_index(path);
	if (inode_index == -1) {
		fprintf(stderr, "[debug] fisopfs_rmdir: %s\n", strerror(ENOENT));
        return -ENOENT;
    }

	inode_t *inode = &(fs.inodes[inode_index]);

	if (!inode->is_directory){
		fprintf(stderr, "[debug] fisopfs_rmdir - %s\n", strerror(ENOTDIR));
		return -ENOTDIR;
	}

	int n_files = get_nfiles(path);
	if (n_files != 0){
		fprintf(stderr, "[debug] fisopfs_rmdir: %s\n", strerror(ENOTEMPTY));
        return -ENOTEMPTY;
	}

	inode->valid = false;

	return 0;
}

int fisopfs_flush(const char *, struct fuse_file_info *) {
	printf("[debug] fisopfs_flush\n");
	fisopfs_destroy(NULL);
	return 0;
}

static struct fuse_operations operations = {
	.getattr = fisopfs_getattr,
	.readdir = fisopfs_readdir,
	.read = fisopfs_read,
	.init = fisopfs_init,
	.create = fisopfs_create,
	.write = fisopfs_write,
	.mkdir = fisopfs_mkdir,
	.utimens = fisopfs_utimens,
	.truncate = fisopfs_truncate,
	.unlink = fisopfs_unlink,
	.rmdir = fisopfs_rmdir,
	.destroy = fisopfs_destroy,
	.flush = fisopfs_flush
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
