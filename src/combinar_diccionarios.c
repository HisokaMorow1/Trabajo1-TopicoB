// Bibliotecas estándar de C
#include <stdio.h>      // Para entrada/salida (printf, fopen, etc.)
#include <stdlib.h>     // Para funciones de memoria dinámica (malloc, free, etc.)
#include <string.h>     // Para manipulación de cadenas (strcmp, strcpy, etc.)

// Definición de constantes
#define MAX_WORD_LENGTH 100  // Longitud máxima de una palabra

// Estructura para almacenar una lista dinámica de palabras
typedef struct {
    char **palabras;  // Array dinámico de punteros a cadenas (palabras)
    int count;        // Número actual de palabras en la lista
    int capacity;     // Capacidad actual del array (para gestión de memoria)
} ListaPalabras;

// Declaración de funciones (prototipos)
ListaPalabras* inicializar_lista();  // Crea y reserva memoria para una nueva lista
void expandir_lista(ListaPalabras *lista);  // Duplica la capacidad de la lista si es necesario
int palabra_existe(ListaPalabras *lista, const char *palabra);  // Verifica si una palabra ya está en la lista
void agregar_palabra_unica(ListaPalabras *lista, const char *palabra);  // Agrega una palabra solo si no existe
int leer_archivo_palabras(const char *nombre_archivo, ListaPalabras *lista);  // Lee palabras de un archivo y las agrega
void ordenar_lista(ListaPalabras *lista);  // Ordena las palabras alfabéticamente
void escribir_archivo_palabras(ListaPalabras *lista, const char *nombre_archivo);  // Escribe las palabras en un archivo
void liberar_lista(ListaPalabras *lista);  // Libera toda la memoria de la lista
int leer_archivo_palabras(const char *nombre_archivo, ListaPalabras *lista);  // Prototipo duplicado (redundante)

// Función principal del programa
int main(int argc, char *argv[]) {
    // Verificar que se recibieron los argumentos correctos
    if (argc != 4) {
        // Mostrar mensaje de uso si faltan argumentos
        printf("Uso: %s <diccionario.txt> <diccionario2.txt> <diccionario_final.txt>\n", argv[0]);
        printf("Ejemplo: %s txt/diccionario.txt txt/diccionario2.txt txt/diccionario_final.txt\n", argv[0]);
        printf("\nEste programa combina dos diccionarios eliminando duplicados y ordena el resultado.\n");
        printf("NOTA: Asegúrate de que diccionario2.txt haya sido limpiado manualmente.\n");
        return 1;  // Retornar código de error
    }
    
    // Obtener los nombres de archivos desde los argumentos
    const char *archivo_diccionario1 = argv[1];  // Primer diccionario (español estándar)
    const char *archivo_diccionario2 = argv[2];  // Segundo diccionario (palabras adicionales)
    const char *archivo_salida = argv[3];        // Diccionario combinado final
    
    // Mostrar encabezado con información de los archivos
    printf("=== COMBINADOR DE DICCIONARIOS ===\n");
    printf("Diccionario español: %s\n", archivo_diccionario1);
    printf("Diccionario adicional: %s\n", archivo_diccionario2);
    printf("Diccionario final: %s\n\n", archivo_salida);
    
    // Inicializar la lista que contendrá todas las palabras combinadas
    ListaPalabras *diccionario_final = inicializar_lista();
    
    // Leer primer diccionario (español estándar)
    printf("1. Leyendo diccionario español estándar...\n");
    if (!leer_archivo_palabras(archivo_diccionario1, diccionario_final)) {
        liberar_lista(diccionario_final);  // Liberar memoria antes de salir
        return 1;  // Retornar error si no se pudo leer
    }
    
    // Leer segundo diccionario (palabras adicionales ya limpiadas)
    printf("\n2. Leyendo diccionario adicional...\n");
    if (!leer_archivo_palabras(archivo_diccionario2, diccionario_final)) {
        liberar_lista(diccionario_final);  // Liberar memoria antes de salir
        return 1;  // Retornar error si no se pudo leer
    }
    
    // Ordenar todas las palabras alfabéticamente
    printf("\n3. Ordenando diccionario final...\n");
    ordenar_lista(diccionario_final);
    
    // Escribir el diccionario combinado en el archivo de salida
    printf("\n4. Escribiendo diccionario final...\n");
    escribir_archivo_palabras(diccionario_final, archivo_salida);
    
    // Mostrar estadísticas finales
    printf("\n=== ESTADÍSTICAS FINALES ===\n");
    printf("Palabras totales en el diccionario final: %d\n", diccionario_final->count);
    
    // Liberar toda la memoria dinámica asignada
    liberar_lista(diccionario_final);
    
    // Mensajes de finalización exitosa
    printf("\nCombinación completada exitosamente.\n");
    printf("RECUERDA: El archivo '%s' está listo para generar base_texto.txt\n", archivo_salida);
    
    return 0;  // Retornar éxito
}

// Función para inicializar una nueva lista de palabras
// Reserva memoria y establece valores iniciales
ListaPalabras* inicializar_lista() {
    // Reservar memoria para la estructura ListaPalabras
    ListaPalabras *lista = malloc(sizeof(ListaPalabras));
    lista->capacity = 1000;  // Capacidad inicial: 1000 palabras
    lista->count = 0;        // Inicialmente no hay palabras
    // Reservar memoria para el array de punteros a palabras
    lista->palabras = malloc(lista->capacity * sizeof(char*));
    return lista;  // Retornar puntero a la lista creada
}

// Función para expandir la capacidad de la lista si está llena
void expandir_lista(ListaPalabras *lista) {
    // Verificar si se alcanzó la capacidad máxima
    if (lista->count >= lista->capacity) {
        lista->capacity *= 2;  // Duplicar la capacidad
        // Reasignar memoria con el nuevo tamaño
        lista->palabras = realloc(lista->palabras, lista->capacity * sizeof(char*));
    }
}

// Función para verificar si una palabra ya existe en la lista (búsqueda binaria - más rápida)
// Retorna 1 si existe, 0 si no existe
// IMPORTANTE: La lista DEBE estar ordenada
int palabra_existe_binaria(ListaPalabras *lista, const char *palabra) {
    int izquierda = 0;
    int derecha = lista->count - 1;
    
    while (izquierda <= derecha) {
        int medio = izquierda + (derecha - izquierda) / 2;
        int comparacion = strcmp(lista->palabras[medio], palabra);
        
        if (comparacion == 0) {
            return 1;  // Encontrada
        } else if (comparacion < 0) {
            izquierda = medio + 1;
        } else {
            derecha = medio - 1;
        }
    }
    
    return 0;  // No encontrada
}

// Función para verificar si una palabra ya existe en la lista (búsqueda lineal - original)
// Retorna 1 si existe, 0 si no existe
int palabra_existe(ListaPalabras *lista, const char *palabra) {
    // Recorrer toda la lista buscando la palabra
    for (int i = 0; i < lista->count; i++) {
        // Comparar la palabra actual con la palabra buscada
        if (strcmp(lista->palabras[i], palabra) == 0) {
            return 1;  // La palabra ya existe
        }
    }
    return 0;  // La palabra no existe
}

// Función para agregar una palabra a la lista (solo si no existe)
// Evita duplicados en la lista
void agregar_palabra_unica(ListaPalabras *lista, const char *palabra) {
    // Solo agregar si la palabra no existe en la lista
    if (!palabra_existe(lista, palabra)) {
        expandir_lista(lista);  // Expandir si es necesario
        // Reservar memoria para la nueva palabra
        lista->palabras[lista->count] = malloc(strlen(palabra) + 1);
        // Copiar la palabra en la memoria reservada
        strcpy(lista->palabras[lista->count], palabra);
        lista->count++;  // Incrementar contador de palabras
    }
}

// Función para leer palabras de un archivo y agregarlas a la lista
// Solo agrega palabras que no existan ya (elimina duplicados automáticamente)
// Retorna 1 si tiene éxito, 0 si hay error
int leer_archivo_palabras(const char *nombre_archivo, ListaPalabras *lista) {
    FILE *archivo = fopen(nombre_archivo, "r");  // Abrir archivo en modo lectura
    if (archivo == NULL) {  // Verificar si hubo error al abrir
        printf("Error: No se pudo abrir el archivo %s\n", nombre_archivo);
        return 0;  // Retornar error
    }
    
    char palabra[MAX_WORD_LENGTH];  // Buffer para leer cada palabra
    int palabras_agregadas = 0;     // Contador de palabras nuevas agregadas
    int palabras_leidas = 0;        // Contador total de palabras leídas
    int palabras_duplicadas = 0;    // Contador de palabras duplicadas
    
    // Leer el archivo línea por línea
    while (fgets(palabra, sizeof(palabra), archivo)) {
        palabras_leidas++;  // Incrementar contador de palabras leídas
        
        // Eliminar salto de línea al final si existe
        int len = strlen(palabra);
        if (len > 0 && palabra[len-1] == '\n') {
            palabra[len-1] = '\0';  // Reemplazar '\n' con fin de cadena
            len--;  // Actualizar longitud
        }
        
        // Ignorar líneas vacías
        if (len == 0) {
            continue;  // Saltar a la siguiente iteración
        }
        
        // Agregar palabra si no existe (evita duplicados)
        if (!palabra_existe(lista, palabra)) {
            agregar_palabra_unica(lista, palabra);
            palabras_agregadas++;  // Incrementar contador de palabras nuevas
        } else {
            palabras_duplicadas++;  // Incrementar contador de duplicados
        }
    }
    
    fclose(archivo);  // Cerrar archivo
    // Mostrar estadísticas de lectura detalladas
    printf("Archivo '%s': %d leídas, %d nuevas, %d duplicadas.\n", 
           nombre_archivo, palabras_leidas, palabras_agregadas, palabras_duplicadas);
    return 1;  // Retornar éxito
}

// Función de comparación para qsort
// Compara dos cadenas alfabéticamente
int comparar_palabras(const void *a, const void *b) {
    // Convertir los punteros void a punteros a cadena
    // qsort pasa punteros a los elementos del array (que son char**)
    return strcmp(*(const char**)a, *(const char**)b);
}

// Función para ordenar la lista de palabras alfabéticamente
void ordenar_lista(ListaPalabras *lista) {
    // Usar qsort de la biblioteca estándar
    qsort(lista->palabras,           // Array a ordenar
          lista->count,               // Número de elementos
          sizeof(char*),              // Tamaño de cada elemento
          comparar_palabras);         // Función de comparación
}

// Función para escribir todas las palabras de la lista en un archivo
// Cada palabra se escribe en una línea separada
void escribir_archivo_palabras(ListaPalabras *lista, const char *nombre_archivo) {
    FILE *archivo = fopen(nombre_archivo, "w");  // Abrir archivo en modo escritura
    if (archivo == NULL) {  // Verificar si hubo error al crear
        printf("Error: No se pudo crear el archivo %s\n", nombre_archivo);
        return;  // Salir de la función
    }
    
    // Escribir cada palabra en una línea del archivo
    for (int i = 0; i < lista->count; i++) {
        fprintf(archivo, "%s\n", lista->palabras[i]);
    }
    
    fclose(archivo);  // Cerrar archivo
    // Mostrar mensaje de confirmación
    printf("Archivo '%s' creado con %d palabras únicas.\n", nombre_archivo, lista->count);
}

// Función para liberar toda la memoria dinámica de la lista
void liberar_lista(ListaPalabras *lista) {
    // Primero liberar cada palabra individual
    for (int i = 0; i < lista->count; i++) {
        free(lista->palabras[i]);  // Liberar memoria de la palabra
    }
    // Luego liberar el array de punteros
    free(lista->palabras);
    // Finalmente liberar la estructura ListaPalabras
    free(lista);
}

