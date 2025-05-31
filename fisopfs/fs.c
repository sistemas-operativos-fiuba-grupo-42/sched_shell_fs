#include "fs.h"
#include <stdbool.h>
#include <time.h>

struct fs fs;

// --- AUXILIARES
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

// -- readdir
int fs_is_directory(const char *path) {
    printf("[debug] fs_is_directory - path: %s\n", path);
    int inode_index = get_inode_index(path);
    if (inode_index == -1) {
        fprintf(stderr, "[debug] fs_is_directory - path: %s\n", strerror(ENOENT));
        return -ENOENT;
    }
    inode_t *inode = &fs.inodes[inode_index];
    if (!inode->is_directory) {
        fprintf(stderr, "[debug] fs_is_directory - %s\n", strerror(ENOTDIR));
        return -ENOTDIR;
    }
    inode->last_access = time(NULL);
    return 0;
}

// --- FILESYSTEM
void* fs_init(const char *filedisk) {
	printf("[debug] fs_init\n");

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
			fprintf(stderr, "[debug] fs_init %s\n", strerror(errno));
			fclose(file);
			return NULL;
		}
		fclose(file);
	}
	return NULL;
}

void fs_destroy(const char *filedisk) {
	printf("[debug] fs_destroy\n");

	FILE *file = fopen(filedisk, "w");
	if (file == NULL){
		fprintf(stderr, "[debug] fs_destroy %s\n", strerror(errno));
		return;
	}

	size_t bytes = fwrite(&fs, sizeof(fs), 1, file);
	if (bytes != 1){
		fprintf(stderr, "[debug] fs_destroy %s\n", strerror(errno));
		return;
	}
	fflush(file);
	fclose(file);
}

int
fs_getattr(const char *path, struct stat *st)
{
	printf("[debug] fs_getattr - path: %s\n", path);

	int inode_index = get_inode_index(path);
	if (inode_index == -1){
		fprintf(stderr, "[debug] fs_getattr - path: %s\n", strerror(ENOENT));
		return -ENOENT;
	}
	inode_t *inode = &(fs.inodes[inode_index]);

	st->st_uid = inode->uid;
	st->st_gid = inode->gid;
	st->st_mode = inode->mode;

	st->st_size = inode->size;
	st->st_nlink = inode->nlink;

	st->st_atime = inode->last_access;
	st->st_mtime = inode->last_modified;
	st->st_ctime = inode->creation_time;

	return 0;
}

int get_inode_in_directory(const char *path, int *index, char *entry_name){
	size_t n = strlen(path);

	for (int i = *index; i < MAX_INODES; i++) {
		if (fs.inodes[i].valid) {
    		if (strncmp(path, fs.inodes[i].path, n) == 0 && strcmp(fs.inodes[i].path, path) != 0) {
				const char *entry = fs.inodes[i].path + n;
                if (strchr(entry, '/') == NULL) {
					strncpy(entry_name, entry, MAX_PATH_SIZE);
					*index = i + 1;
					return 1;
				}
            }
		}
	}
	return 0;
}

int
fs_read(const char *path,
             char *buffer,
             size_t size,
             off_t offset)
{
	printf("[debug] fs_read - path: %s, offset: %lu, size: %lu\n",
	       path,
	       offset,
	       size);


	int inode_index = get_inode_index(path);
	if(inode_index == -1){
		fprintf(stderr, "[debug] fs_read - path: %s\n", strerror(ENOENT));
		return -ENOENT;
	}

	inode_t *inode = &(fs.inodes[inode_index]);

	if(inode->is_directory){
		fprintf(stderr, "[debug] fs_read - %s\n", strerror(EISDIR));
		return -EISDIR;
	}

	inode->last_access = time(NULL);
	char *data = inode->data;

	if (offset > inode->size) {
		fprintf(stderr, "[debug] fs_read - %s\n", strerror(EINVAL));
		return -EINVAL;
	}

	size_t bytes = size;
	if (inode->size - offset < bytes){
		bytes = inode->size - offset;
	}

	memcpy(buffer, data + offset, bytes);

	return bytes;
}

int fs_create(const char *path, mode_t mode)
{
	printf("[debug] fs_create - path: %s, mode: %o\n", path, mode);

	if (strlen(path) - 1 > MAX_PATH_SIZE){
		fprintf(stderr, "[debug] fs_create - %s\n", strerror(ENAMETOOLONG));
		return -ENAMETOOLONG;
	}

	int inode_index = get_unused_inode();
	if (inode_index == -1){
		fprintf(stderr, "[debug] fs_create - %s\n", strerror(ENOMEM));
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

int fs_write(const char *path, const char *buffer, size_t size, off_t offset)
{

	printf("[debug] fs_write - path: %s, offset: %lu, size: %lu\n", path, offset, size);

	if(offset + size > MAX_FILE_SIZE){
		fprintf(stderr, "[debug] fs_write - %s\n", strerror(E2BIG));
		return -E2BIG;
	}

	int idx = get_inode_index(path);
	if (idx == -1) {
		fprintf(stderr, "[debug] fs_write - path: %s\n", strerror(ENOENT));
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

int fs_mkdir(const char *path, mode_t mode)
{
    printf("[debug] fs_mkdir - path: %s, mode: %o\n", path, mode);
    if (strlen(path) - 1 > MAX_PATH_SIZE) {
        fprintf(stderr, "[debug] fs_mkdir - %s\n", strerror(ENAMETOOLONG));
        return -ENAMETOOLONG;
    }
    int inode_index = get_unused_inode();
    if (inode_index == -1) {
        fprintf(stderr, "[debug] fs_mkdir - %s\n", strerror(ENOMEM));
        return -ENOMEM;
    }

    char parent_path[MAX_PATH_SIZE];
    strncpy(parent_path, path, MAX_PATH_SIZE);
    char *last_slash = strrchr(parent_path, '/');
    if (last_slash != parent_path) {
        *last_slash = '\0';
        if (get_inode_index(parent_path) == -1) {
            fprintf(stderr, "[debug] fs_mkdir - parent path: %s\n", strerror(ENOENT));
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

int fs_utimens(const char *path, const struct timespec tv[2]) {
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

int fs_truncate(const char *path, off_t length) {
	printf("[debug] fs_truncate - path: %s length: %ld\n", path, length);
	int inode_index = get_inode_index(path);
	if (inode_index == -1) {
		fprintf(stderr, "[debug] fs_truncate: %s\n", strerror(ENOENT));
        return -ENOENT;
    }

	inode_t *inode = &fs.inodes[inode_index];

	if (length > inode->size) {
		fprintf(stderr, "[debug] fs_truncate - %s\n", strerror(E2BIG));
		return -E2BIG;
	}

	inode->size = length;
	inode->last_modified = time(NULL);
	inode->last_access = time(NULL);

	return 0;
}

int fs_unlink(const char *path) {
	printf("[debug] fs_unlink - path: %s \n", path);
	int inode_index = get_inode_index(path);
	if (inode_index == -1) {
		fprintf(stderr, "[debug] fs_unlink: %s\n", strerror(ENOENT));
        return -ENOENT;
    }

	inode_t *inode = &(fs.inodes[inode_index]);
	if (inode->is_directory){
		fprintf(stderr, "[debug] fs_unlink - %s\n", strerror(EISDIR));
		return -EISDIR;
	}

	inode->valid = false;
	inode->size = 0;

	return 0;
}

int fs_rmdir(const char *path) {
	printf("[debug] fs_rmdir - path: %s \n", path);
	int inode_index = get_inode_index(path);
	if (inode_index == -1) {
		fprintf(stderr, "[debug] fs_rmdir: %s\n", strerror(ENOENT));
        return -ENOENT;
    }

	inode_t *inode = &(fs.inodes[inode_index]);

	if (!inode->is_directory){
		fprintf(stderr, "[debug] fs_rmdir - %s\n", strerror(ENOTDIR));
		return -ENOTDIR;
	}

	int n_files = get_nfiles(path);
	if (n_files != 0){
		fprintf(stderr, "[debug] fs_rmdir: %s\n", strerror(ENOTEMPTY));
        return -ENOTEMPTY;
	}

	inode->valid = false;

	return 0;
}

int fs_flush(const char *path, char *filedisk) {
	printf("[debug] fs_flush\n");
	fs_destroy(filedisk);
	return 0;
}
