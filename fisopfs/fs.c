#include "fs.h"
#include <stdbool.h>
#include <time.h>


// En caso de error o funcionalidad no implementada, el sistema de archivos debe escribirlo por pantalla (basta con un printf a stderr, o con el prefijo [debug]) y devolver el error apropiado. Pueden tomar los errores definidos en errno.h (ver errno(3)) como inspiración, viendo qué errores arrojan otros sistemas de archivos. Algunas opciones útiles son: ENOENT, ENOTDIR, EIO, EINVAL, EBIG y ENOMEM, entre otros.

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

	// Solo tenemos un archivo hardcodeado!
	if (strcmp(path, "/fisop") != 0)
		return -ENOENT;

	if (offset + size > strlen(fisop_file_contenidos))
		size = strlen(fisop_file_contenidos) - offset;

	size = size > 0 ? size : 0;

	memcpy(buffer, fisop_file_contenidos + offset, size);

	return size;
}
