<div align="center">
  <h1>💾 FisopFS</h1>

  <p>
    <strong>Sistemas Operativos (7508) - FIUBA</strong><br>
    Implementación de un Sistema de Archivos en espacio de usuario utilizando <strong>FUSE</strong>.
  </p>

  <img src="https://img.shields.io/badge/Language-C-00599C?style=flat-square&logo=c&logoColor=white" alt="Lenguaje C">
  <img src="https://img.shields.io/badge/Platform-Linux-FCC624?style=flat-square&logo=linux&logoColor=black" alt="Plataforma Linux">
  <img src="https://img.shields.io/badge/Tools-Docker-2496ED?style=flat-square&logo=docker&logoColor=white" alt="Docker">
  <img src="https://img.shields.io/badge/Build-Makefile-000000?style=flat-square&logo=gnu-make&logoColor=white" alt="Makefile">

  <br>

  <img src="./fisopfs/Terminal.png" alt="Ejecución en Terminal" width="80%" style="border-radius: 10px; box-shadow: 0px 4px 10px rgba(0,0,0,0.2);">
  </div>

---

## 📋 Descripción

**FisopFS** es un sistema de archivos implementado sobre FUSE (Filesystem in Userspace) que simula un disco en memoria con capacidad de persistencia. El proyecto permite realizar operaciones estándar de archivos (crear, leer, escribir, borrar) y directorios, manteniendo la estructura de datos en memoria y serializándola a disco al finalizar.

### Características principales
* **Estructura en Memoria:** Utiliza un array estático de inodos para representar archivos y directorios.
* **Persistencia:** Capacidad de guardar el estado del sistema de archivos en un archivo binario (`.fisopfs`) y recuperarlo en el siguiente montaje.
* **Interfaz POSIX:** Compatible con comandos estándar de terminal (`ls`, `touch`, `mkdir`, `cat`, etc.).

## 📝 Respuestas teóricas

El desarrollo teórico, incluyendo las estructuras de datos elegidas y las decisiones de diseño, se encuentra detallado en el archivo:
* [fisopfs.md](./fisopfs.md)

## 🚀 Compilación y Ejecución

### Compilar

Para generar el binario del sistema de archivos:

```bash
$ make
```

### Ejecutar

#### Setup

Primero hay que crear un directorio de prueba:

```bash
$ mkdir prueba
```

#### Iniciar el servidor FUSE

En el mismo directorio que se utilizó para compilar la solución, ejectuar:

```bash
$ ./fisopfs prueba/
```

**Opciones de Persistencia:** Por defecto, el sistema guarda los datos en persistence_file.fisopfs. Puedes especificar un archivo personalizado con el flag --filedisk:

```bash
$ ./fisopfs prueba/ --filedisk nuevo_disco.fisopfs
```

#### Verificar directorio

```bash
$ mount | grep fisopfs
```

### Utilizar el directorio de "pruebas"

El repositorio cuenta con un script de pruebas automatizado (test_fisopfs.sh) que verifica las funcionalidades críticas: creación de archivos, lectura/escritura, persistencia y manejo de directorios.

En otra terminal, ejecutar:

```bash
$ cd prueba
$ ls -al
```

### Limpieza

Para desmontar el sistema de archivos y asegurar que los datos se guarden en el archivo de persistencia:

```bash
$ sudo umount prueba
```

## 🐳 Docker

Existen tres _targets_ en el archivo `Makefile` para utilizar _docker_.

- `docker-build`: genera la imagen basada en "Ubuntu 20.04" con las dependencias de FUSE
- `docker-run`: crea un _container_ basado en la imagen anterior ejecutando `bash`
   - acá se puede ejecutar `make` y luego `./fisopfs -f ./prueba`
- `docker-exec`: permite vincularse al mismo _container_ anterior para poder realizar pruebas
   - acá se puede ingresar al directorio `prueba`

## Linter

```bash
$ make format
```

Para efectivamente subir los cambios producidos por el `format`, hay que `git add .` y `git commit`.

## 👥 Integrantes 

| Integrante | Padrón | Contacto |
| :--- | :---: | :---: |
| **Calderón Vasil, Máximo Augusto** | 111810 | [![GitHub](https://img.shields.io/badge/GitHub-black?style=flat-square&logo=github)](https://github.com/maxivasil) [![Email](https://img.shields.io/badge/Email-red?style=flat-square&logo=gmail&logoColor=white)](mailto:mcalderonv@fi.uba.ar) |
| **Molina Buitrago, Steven Marlon** | 112018 | [![GitHub](https://img.shields.io/badge/GitHub-black?style=flat-square&logo=github)](https://github.com/StevenMolina22) [![Email](https://img.shields.io/badge/Email-red?style=flat-square&logo=gmail&logoColor=white)](mailto:mmolinab@fi.uba.ar) |
| **Moore, Juan Ignacio** | 112479 | [![GitHub](https://img.shields.io/badge/GitHub-black?style=flat-square&logo=github)](https://github.com/JuaniMoore) [![Email](https://img.shields.io/badge/Email-red?style=flat-square&logo=gmail&logoColor=white)](mailto:jmoore@fi.uba.ar) |
| **Tripaldi, Ulises Valentín** | 111919 | [![GitHub](https://img.shields.io/badge/GitHub-black?style=flat-square&logo=github)](https://github.com/utripaldi) [![Email](https://img.shields.io/badge/Email-red?style=flat-square&logo=gmail&logoColor=white)](mailto:utripaldi@fi.uba.ar) |
