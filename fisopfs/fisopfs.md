# fisop-fs

## Estructuras en memoria

Para la representación del file system en general usamos un struct fs, compuesto por un array de 1024 inodes. Dichos inodes se utilizan ya sea para representar archivos o directorios.

Todas las constantes (MAX_INODES, MAX_PATH_SIZE, MAX_FILE_SIZE) fueron definidas para utilizar memoria estatica con el fin de facilitar el menejo de la misma.

```c
struct fs
{
    inode_t inodes[MAX_INODES];
};
```

### Inode

Cada Inode contiene la metadata necesaria de cada archivo/directorio y los datos que contiene el mismo (los directorios no contienen data):

``` c
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
```

**Metadata:**

|       Clave          |                              Valor                             |
|----------------------|----------------------------------------------------------------|
| is_directory         | flag que indica si es archivo o directorio                     |
| valid                | flag que indica si el inodo es válido, o sea está en uso       |
| mode                 | flag que indica el modo del inodo (lectura, escritura, etc)    |
| gid                  |                           group id                             |
| uid                  |                           user id                              |
| size                 |           tamaño de los datos que contiene                     |
| creation_time        |                   fecha en que se creo                         |
| last_access          |     indica la última vez que se accedió al archivo accedió     |
| last_modified        |         indica la ultima fecha en que se modificó              |
| path                 |           ruta absoluta del archivo o directorio               |


## Busqueda de un archivo

Para encontrar un archivo en específico mediante su path se implementó la función get_inode_index, la cual recorre todos los inodos del file system en busqueda del que sea válido y coincida su path con la que se busca. En caso de existir se devuelve el índice de dicho inodo, en caso contrario -1.


## Persistencia del file system en disco


    El formato de serialización del sistema de archivos en disco (ver siguiente sección)