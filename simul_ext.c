#include "cabecera.h"
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#define NOMBRE_PARTICION "particion.bin" // Aquí hago un define para no tener que repetir constantemente el '.bin'
#define LONGITUD_COMANDO 100

// Funciones dadas para la práctica
void inicializar_particion();
void Printbytemaps(EXT_BYTE_MAPS *ext_bytemaps);
int ComprobarComando(char *strcomando, char *orden, char *argumento1, char *argumento2);
void LeeSuperBloque(EXT_SIMPLE_SUPERBLOCK *psup);
int BuscaFich(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, char *nombre);
void Directorio(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos);
int Renombrar(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, char *nombreantiguo, char *nombrenuevo);
int Imprimir(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, EXT_DATOS *memdatos, char *nombre);
int Borrar(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, EXT_BYTE_MAPS *ext_bytemaps, EXT_SIMPLE_SUPERBLOCK *ext_superblock, char *nombre, FILE *fich);
int Copiar(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, EXT_BYTE_MAPS *ext_bytemaps, EXT_SIMPLE_SUPERBLOCK *ext_superblock, EXT_DATOS *memdatos, char *nombreorigen, char *nombredestino, FILE *fich);
void Grabarinodosydirectorio(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, FILE *fich);
void GrabarByteMaps(EXT_BYTE_MAPS *ext_bytemaps, FILE *fich);
void GrabarSuperBloque(EXT_SIMPLE_SUPERBLOCK *ext_superblock, FILE *fich);
void GrabarDatos(EXT_DATOS *memdatos, FILE *fich);

int main() {
    char comando[LONGITUD_COMANDO], orden[LONGITUD_COMANDO], argumento1[LONGITUD_COMANDO], argumento2[LONGITUD_COMANDO];
    EXT_SIMPLE_SUPERBLOCK ext_superblock;
    EXT_BYTE_MAPS ext_bytemaps;
    EXT_BLQ_INODOS ext_blq_inodos;
    EXT_ENTRADA_DIR directorio[MAX_FICHEROS];
    EXT_DATOS memdatos[MAX_BLOQUES_DATOS];
    int grabar = 0;

    // Se abre el archivo particion.bin
    FILE *fichero = fopen(NOMBRE_PARTICION, "r+b"); // lectura y escritura

    // Para comprobar si se ha abierto el fichero
    if (!fichero) {
        perror("Error: el archivo particion.bin no se ha podido abrir");
        return 1;
    }

    // Se leen las estructuras
    fread(&ext_superblock, sizeof(EXT_SIMPLE_SUPERBLOCK), 1, fichero);
    fread(&ext_bytemaps, sizeof(EXT_BYTE_MAPS), 1, fichero);
    fread(&ext_blq_inodos, sizeof(EXT_BLQ_INODOS), 1, fichero);
    fread(directorio, sizeof(EXT_ENTRADA_DIR), MAX_FICHEROS, fichero);
    fread(memdatos, sizeof(EXT_DATOS), MAX_BLOQUES_DATOS, fichero);
    fseek(fichero, SIZE_BLOQUE * PRIM_BLOQUE_DATOS, SEEK_SET);
    fread(memdatos, sizeof(EXT_DATOS), MAX_BLOQUES_DATOS, fichero);

    // Bucle para que se repita el menu constantemente, también están las funciones de cada opción y el comprobantes de comandos para ver si es correcto el comando insertado
    for (;;) {
        do {
            printf(">> ");
            fflush(stdin);
            fgets(comando, LONGITUD_COMANDO, stdin); // lee el comando
        } while (ComprobarComando(comando, orden, argumento1, argumento2) != 0); // Comprueba si el comando está correcto

        if (strcmp(orden, "info") == 0) { // Muestra la informacion del superbloque
            LeeSuperBloque(&ext_superblock);
        } else if (strcmp(orden, "dir") == 0) { // lista del contenido del directorio
            Directorio(directorio, &ext_blq_inodos);
        }else if (strcmp(orden, "bytemaps") == 0) { // muetra los inodos
            Printbytemaps(&ext_bytemaps);
        } else if (strcmp(orden, "rename") == 0) { // se usa para renombrar el archivo, se pone de la siguiente forma -> HOLA.txt(antiguo nombre) ADIOS.txt (nuevo nombre)
            Renombrar(directorio, &ext_blq_inodos, argumento1, argumento2);
            grabar = 1;
        } else if (strcmp(orden, "remove") == 0) { // se usa para borrar el archivo compleytamente del directorio
            Borrar(directorio, &ext_blq_inodos, &ext_bytemaps, &ext_superblock, argumento1, fichero);
            grabar = 1;
        } else if (strcmp(orden, "copy") == 0) { // se usa para copiar un archivo contoda la informacion que contiene, se pone de la siguiente forma -> HOLA.txt HOLA_COPIADO.txt
            Copiar(directorio, &ext_blq_inodos, &ext_bytemaps, &ext_superblock, memdatos, argumento1, argumento2, fichero);
            grabar = 1;
        } else if (strcmp(orden, "imprimir") == 0) { // se usa para que muestre el contenido del .txt
            Imprimir(directorio, &ext_blq_inodos, memdatos, argumento1);
        } else if (strcmp(orden, "salir") == 0) { // se usa para salir del bucle
            break;
        } else {
            printf("ERROR: Comando erróneo [bytemaps, copy, dir, info, imprimir, rename, remove, salir]\n"); // esto sale si se ha insertado de forma errónea el comando
        }

        // Guarda los cambios en partición si se necesita
        if (grabar) {
            Grabarinodosydirectorio(directorio, &ext_blq_inodos, fichero); // guarda el directorio
            GrabarByteMaps(&ext_bytemaps, fichero); // guarda el bytemaps
            GrabarSuperBloque(&ext_superblock, fichero); // guarda superbloque
            GrabarDatos(memdatos, fichero); // guarda los datos de los archivos
            grabar = 0;
        }
    }

    fclose(fichero); // Se cierra la partición
    return 0;
}

void inicializar_particion(){
    FILE* fichero = fopen(NOMBRE_PARTICION, "wb");

    if (!fichero) {
        perror("Error: no se puede abrir el archivo de partición");
        exit(1);
    }

    // Se definen las estructuras
    EXT_SIMPLE_SUPERBLOCK superbloque;
    printf("Superbloque inicializado.\n");
    EXT_BYTE_MAPS bytemaps;
    printf("Bytemaps inicializados.\n");
    EXT_SIMPLE_INODE inodos[MAX_INODOS];
    printf("Inodos inicializados.\n");
    EXT_ENTRADA_DIR directorio[MAX_FICHEROS] = {0}; // Inicializa a cero
    EXT_DATOS datos[MAX_BLOQUES_DATOS] = {0};      // Inicializa a cero
    printf("Directorio y datos inicializados.\n");

    // Se inicializa el superbloque
    superbloque.s_inodes_count = 0;
    superbloque.s_blocks_count = MAX_BLOQUES_PARTICION;
    superbloque.s_free_blocks_count = MAX_BLOQUES_PARTICION - 4;
    superbloque.s_free_inodes_count = MAX_INODOS;
    superbloque.s_first_data_block = 4;
    superbloque.s_block_size = SIZE_BLOQUE;
    memset(&superbloque.s_relleno, 0, sizeof(superbloque.s_relleno));
    // Se inicializa los bytemaps
    memset(bytemaps.bmap_bloques, 0, MAX_BLOQUES_PARTICION);
    memset(bytemaps.bmap_inodos, 0, MAX_INODOS);
    memset(bytemaps.bmap_bloques, 1, 4);
    // Se inicializa los inodos
    for (int i = 0; i < MAX_INODOS; i++) {
        inodos[i].size_fichero = 0;
        for (int j = 0; j < MAX_NUMS_BLOQUE_INODO; j++) {
            inodos[i].i_nbloque[j] = NULL_BLOQUE;
        }
    }

    // Escribe en el archivo
    fwrite(&superbloque, sizeof(superbloque), 1, fichero);
    fwrite(&bytemaps, sizeof(bytemaps), 1, fichero);
    fwrite(inodos, sizeof(inodos), MAX_INODOS, fichero);
    fwrite(directorio, sizeof(directorio), 1, fichero);
    fwrite(datos, sizeof(datos), 1, fichero);

    // Bloques vacíos
    unsigned char bloque_vacio[SIZE_BLOQUE] = {0};
    for (int i = 0; i < MAX_BLOQUES_PARTICION; i++) {
        fwrite(bloque_vacio, SIZE_BLOQUE, 1, fichero);
    }
    fclose(fichero);
}

void LeeSuperBloque(EXT_SIMPLE_SUPERBLOCK *psup) {
    // Se imprimir la información del superbloque
    printf("Bloque %u Bytes\n", psup->s_block_size);
    printf("Inodos de partición = %u\n", psup->s_inodes_count);
    printf("Inodos libres = %u\n", psup->s_free_inodes_count);
    printf("Bloques de partición = %u\n", psup->s_blocks_count);
    printf("Bloques libres = %u\n", psup->s_free_blocks_count);
    printf("Primer bloque de datos = %u\n", psup->s_first_data_block);
}

void Printbytemaps(EXT_BYTE_MAPS *ext_bytemaps) {
    // Se imprime el estado de cada inodo en los archivos
    printf("Inodos: ");
    for (int i = 0; i < MAX_INODOS; i++) {
        // 1 -> ocupado
        // 0 -> libre
        printf("%d ", ext_bytemaps->bmap_inodos[i]);
    }
    printf("\n");

    // Aquí se imprimr el estado de los primeros 25 bloques de los archivo
    printf("Bloques [0-25]: ");
    for (int i = 0; i < 25; i++) {
        // 1 -> ocupado
        // 0 -> libre
        printf("%d ", ext_bytemaps->bmap_bloques[i]);
    }
    printf("\n");
}

void renombrarArchivo(const char *nombreOriginal, const char *nombreNuevo) {
    // se abre el fichero en lectura binaria
    FILE *fichero = fopen(NOMBRE_PARTICION, "r+b");
    // Aquí se almacenan los datos leídos de particion.bin
    EXT_SIMPLE_SUPERBLOCK superbloque;
    EXT_BYTE_MAPS bytemaps;
    EXT_SIMPLE_INODE inodos[MAX_INODOS];
    EXT_ENTRADA_DIR directorio[MAX_INODOS];

    // Se leen las estructuras de particion.bin
    fread(&superbloque, sizeof(EXT_SIMPLE_SUPERBLOCK), 1, fichero);
    fread(&bytemaps, sizeof(EXT_BYTE_MAPS), 1, fichero);
    fread(inodos, sizeof(EXT_SIMPLE_INODE), MAX_INODOS, fichero);
    fread(directorio, sizeof(EXT_ENTRADA_DIR), MAX_INODOS, fichero);

    // Bucle para encontrar el archivo que hay que renombrar
    for (int i = 0; i < MAX_INODOS; i++) {
        EXT_ENTRADA_DIR entrada = directorio[i];
        if (strcmp(entrada.dir_nfich, nombreOriginal) == 0) { // Comprieba si el nombre puesto coincide con el original
            strcpy(entrada.dir_nfich, nombreNuevo); // cambia el nombre po rel nombre nuevo insertado
            break; // sale del bucle después de renombrarlo
        }
    }

    fseek(fichero, SIZE_BLOQUE * 4, SEEK_SET); // reposiciona el puntero al incio del directorio
    fwrite(directorio, sizeof(EXT_ENTRADA_DIR), MAX_INODOS, fichero); // escribe el directorio actualizado despues de renombramiento
    fclose(fichero); // se cierra el fichero
}

int BuscaFich(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, char *nombre) {
    // Bucle sobre cada entrada del directorio
    for (int i = 0; i < MAX_INODOS; i++) {
        // comprueba que el nombre del fichero coincide con el nombre que queremos buscar
        if (strcmp(directorio[i].dir_nfich, nombre) == 0 && directorio[i].dir_inodo != NULL_INODO) {
            return i; // Devuelve el i si se ha encontrado
        }
    }
    return -1; // si no encuentra el fichero devuelve -1
}

int ComprobarComando(char *strcomando, char *orden, char *argumento1, char *argumento2) {
    int Num_argumemtos = sscanf(strcomando, "%s %s %s", orden, argumento1, argumento2); // esto lee y seprara los argumentos

    if (Num_argumemtos < 1) { // si no recibe nada devuelve -1
        return -1;
    }

    // Comprobacion de los comandos y del número de las entradas
    if (strcmp(orden, "info") == 0 && Num_argumemtos != 1) {
        return -1;
    } else if (strcmp(orden, "dir") == 0 && Num_argumemtos != 1) {
        return -1;
    } else if (strcmp(orden, "rename") == 0 && Num_argumemtos != 3) {
        return -1;
    } else if ((strcmp(orden, "remove") == 0 || strcmp(orden, "imprimir") == 0) && Num_argumemtos != 2) {
        return -1;
    } else if (strcmp(orden, "copy") == 0 && Num_argumemtos != 3) {
        return -1;
    }

    return 0;
}

void Directorio(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos) {
    // bucle que recorre las entradas del directorio
    for (int i = 0; i < MAX_FICHEROS; i++) {
        // Esto es para quitar la entrada del directorio raíz
        if (strcmp(directorio[i].dir_nfich, ".") == 0) {
            continue;
        }
        if (directorio[i].dir_inodo != NULL_INODO) { // comprueba si la entrada del directorio tiene un inodo asignado
            EXT_SIMPLE_INODE inodo = inodos->blq_inodos[directorio[i].dir_inodo]; // el inodo que corresponde con la entrada del directorio
            printf("%s tamaño: %u inodo: %d bloques:", directorio[i].dir_nfich, inodo.size_fichero, directorio[i].dir_inodo);
            // bucle que recorre los bloques de datos del inodo
            for (int j = 0; j < MAX_NUMS_BLOQUE_INODO; j++) {
                if (inodo.i_nbloque[j] != NULL_BLOQUE) { // si no es nulo se imprime su número
                    printf(" %d", inodo.i_nbloque[j]);
                } else {
                    break; // si es nulo, se termina el bucle
                }
            }
            printf("\n");
        }
    }
}

int Renombrar(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, char *nombreantiguo, char *nombrenuevo) {
    for (int i = 0; i < MAX_FICHEROS; i++) { // con un bucle s ecomprueba si el nombre insertado ya está en uso
        if (strcmp(directorio[i].dir_nfich, nombrenuevo) == 0) {
            printf("Error: El nombre '%s' ya está en uso.\n", nombrenuevo); // Te sale si el nombre ya existe en dir
            return -1;
        }
    }

    int comprobar = 0; // variable para ver si se ha encontrado
    for (int i = 0; i < MAX_FICHEROS; i++) { // bucl epara buscar el fichero con el nombre antiguo
        if (strcmp(directorio[i].dir_nfich, nombreantiguo) == 0) {
            strcpy(directorio[i].dir_nfich, nombrenuevo);
            comprobar = 1; // se ha encontrado
            break; // se sale del bucle
        }
    }

    if (!comprobar) { // si no se ha encontrado entonces te daldrá el siguiente mensaje
        printf("Error: Fichero '%s' no encontrado.\n", nombreantiguo);
        return -1;
    }

    return 0;
}

int Imprimir(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, EXT_DATOS *memdatos, char *nombre) {
    // busca el fichero por su nombre y obtiene el inodo
    int fichero_existe = BuscaFich(directorio, inodos, nombre);
    if (fichero_existe == -1) {
        printf("Error: Fichero '%s' no encontrado.\n", nombre);
        return -1;
    }

    EXT_SIMPLE_INODE inodo = inodos->blq_inodos[directorio[fichero_existe].dir_inodo]; // coge el inodo del fichero que se ha encontrado

    for (int i = 0; i < MAX_NUMS_BLOQUE_INODO; i++) { // bucle que recorre los bloques de datos del inodo
        // Comprueba que el bloque no sea nulo y que esté dentro del ranfo
        if (inodo.i_nbloque[i] != NULL_BLOQUE && inodo.i_nbloque[i] < MAX_BLOQUES_DATOS + PRIM_BLOQUE_DATOS) {
            int bloque_real = inodo.i_nbloque[i] - PRIM_BLOQUE_DATOS; // indice real del bloque
            printf("%s\n", memdatos[bloque_real].dato);
        } else if (inodo.i_nbloque[i] == NULL_BLOQUE) {
            break; // si encuentra un bloque nulo sale del bucle
        }
    }
    return 0;
}

int Borrar(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, EXT_BYTE_MAPS *ext_bytemaps, EXT_SIMPLE_SUPERBLOCK *ext_superblock, char *nombre, FILE *fich) {
    // Se busca el fichero en el directorio con un bucle
    int fichero_existe = 0;
    for (int i = 0; i < MAX_INODOS; i++){
        if (ext_bytemaps->bmap_inodos[i]!=1){
            continue; // continua si el inodo no se usa
        }
        if (strcmp(directorio[i].dir_nfich, nombre) == 0) {
            fichero_existe = i; // se encuentra el índice del fichero
            break;
        }
    }

    // da error si el fichero no existe
    if (fichero_existe == 0){
        printf("ERROR: fichero %s no se encuentra.\n", nombre);
        return -1;
    }

    // Aquí se libera los bloques de datos usados
    for(int i = 0;i < MAX_NUMS_BLOQUE_INODO; i++){
        if (inodos->blq_inodos[fichero_existe].i_nbloque[i] != NULL_BLOQUE) {
            ext_bytemaps->bmap_bloques[inodos->blq_inodos[fichero_existe].i_nbloque[i]] = 0; // se marca como libre
            ext_superblock->s_free_blocks_count++; // aumenta el numero de bloques libres
        }
        ext_bytemaps->bmap_inodos[fichero_existe] = 0; // se marca como libre
        ext_superblock->s_free_inodes_count++; // aumenta el numero de inodos libres
    }
    directorio[fichero_existe].dir_inodo = NULL_INODO; // se elimina la entrada del directorio
    return 0;
}

int Copiar(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, EXT_BYTE_MAPS *ext_bytemaps, EXT_SIMPLE_SUPERBLOCK *ext_superblock, EXT_DATOS *memdatos, char *nombreorigen, char *nombredestino, FILE *fich) {
    // Se busca el fichero de origen en dir
    int indice_origen = BuscaFich(directorio, inodos, nombreorigen);
    if (indice_origen == -1) {
        printf("Error: Fichero origen '%s' no encontrado.\n", nombreorigen);
        return -1;
    }

    // Comprueba si el fichero ya existe
    int indice_destino = BuscaFich(directorio, inodos, nombredestino);
    if (indice_destino != -1) {
        printf("Error: El fichero destino '%s' ya existe.\n", nombredestino);
        return -1;
    }

    // Se busca un inodo libre
    int inodo_libre = -1;
    for (int i = 0; i < MAX_INODOS; i++) {
        if (ext_bytemaps->bmap_inodos[i] == 0) {
            indice_destino = i;
            break;
        }
    }
    if (indice_destino == -1) {
        printf("Error: No hay inodos libres.\n");
        return -1;
    }

    // Se busca una entrada libre en dir para el fichero de destino
    int entrada_libre = -1;
    for (int i = 0; i < MAX_FICHEROS; i++) {
        if (directorio[i].dir_inodo == NULL_INODO) {
            entrada_libre = i;
            break;
        }
    }
    if (entrada_libre == -1) {
        printf("Error: No hay espacio en el directorio.\n");
        return -1;
    }

    // Se copio los datos del inodo origen al inodo destino
    ext_bytemaps->bmap_inodos[indice_destino] = 1;
    ext_superblock->s_free_inodes_count--;
    EXT_SIMPLE_INODE inodo_origen = inodos->blq_inodos[directorio[indice_origen].dir_inodo];
    inodos->blq_inodos[indice_destino] = inodo_origen;

    // Se crea una nueva entrada de dir para el nuevo fichero
    strcpy(directorio[entrada_libre].dir_nfich, nombredestino);
    directorio[entrada_libre].dir_inodo = indice_destino;

    // Se copian los bloques de datos
    for (int i = 0; i < MAX_NUMS_BLOQUE_INODO; i++) {
        if (inodo_origen.i_nbloque[i] != NULL_BLOQUE) {
            // Aquí se busca un bloque libre para la copia
            int bloque_libre = -1;
            for (int j = PRIM_BLOQUE_DATOS; j < MAX_BLOQUES_PARTICION; j++) {
                if (ext_bytemaps->bmap_bloques[j] == 0) {
                    bloque_libre = j;
                    break;
                }
            }
            if (bloque_libre == -1) {
                printf("Error: No hay bloques libres para la copia.\n");
                return -1;
            }
            // Se copian los datos y se actualizan los bytemaps y superbloque
            ext_bytemaps->bmap_bloques[bloque_libre] = 1;
            ext_superblock->s_free_blocks_count--;
            inodos->blq_inodos[indice_destino].i_nbloque[i] = bloque_libre;
            memdatos[bloque_libre - PRIM_BLOQUE_DATOS] = memdatos[inodo_origen.i_nbloque[i] - PRIM_BLOQUE_DATOS];
        }
    }

    return 0;
}

void Grabarinodosydirectorio(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, FILE *fich) {
    // Comprueba si el archivo está abierto
    if (fich == NULL) {
        printf("Error: El fichero no está abierto.\n");
        return;
    }

    // posiciona en el bloque de inodos y se escriben los inodos
    fseek(fich, SIZE_BLOQUE * 2, SEEK_SET);
    fwrite(inodos, sizeof(EXT_BLQ_INODOS), 1, fich);

    // Se posiciona en el bloque de directorio y se escribe el directorio
    fseek(fich, SIZE_BLOQUE * 3, SEEK_SET);
    fwrite(directorio, sizeof(EXT_ENTRADA_DIR), MAX_FICHEROS, fich);
}

void GrabarByteMaps(EXT_BYTE_MAPS *ext_bytemaps, FILE *fich) {
    // Comprueba si el archivo está abierto
    if (fich == NULL) {
        printf("Error: El fichero no está abierto.\n");
        return;
    }

    // Se posiciona en el bloque de bytemaps y se escriben los bytemaps
    fseek(fich, SIZE_BLOQUE, SEEK_SET);
    fwrite(ext_bytemaps, sizeof(EXT_BYTE_MAPS), 1, fich);
}

void GrabarSuperBloque(EXT_SIMPLE_SUPERBLOCK *ext_superblock, FILE *fich) {
    // Comprueba si el archivo está abierto
    if (fich == NULL) {
        printf("Error: El fichero no está abierto.\n");
        return;
    }

    // Se posiciona en el inicio del archivo y se escribe el superbloque
    fseek(fich, 0, SEEK_SET);
    fwrite(ext_superblock, sizeof(EXT_SIMPLE_SUPERBLOCK), 1, fich);
}

void GrabarDatos(EXT_DATOS *memdatos, FILE *fich) {
    // Comprueba si el archivo está abierto
    if (fich == NULL) {
        printf("Error: El fichero no está abierto.\n");
        return;
    }

    // Se posiciona en el bloque donde empiezan los datos y se escriben los datos
    fseek(fich, SIZE_BLOQUE * PRIM_BLOQUE_DATOS, SEEK_SET);
    for (int i = 0; i < MAX_BLOQUES_DATOS; i++) {
        fwrite(&memdatos[i], sizeof(EXT_DATOS), 1, fich);
    }
}
