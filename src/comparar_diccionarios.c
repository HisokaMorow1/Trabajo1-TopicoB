// Bibliotecas estándar de C
#include <stdio.h>      // Para entrada/salida (printf, fopen, etc.)
#include <stdlib.h>     // Para funciones de memoria dinámica (malloc, free, etc.)
#include <string.h>     // Para manipulación de cadenas (strcmp, strcpy, etc.)

// Definición de constantes
#define MAX_WORD_LENGTH 100  // Longitud máxima de una palabra
#define MAX_WORDS 100000     // Número máximo de palabras que puede manejar

// Estructura para almacenar una lista dinámica de palabras
typedef struct {
    char **palabras;  // Array dinámico de punteros a cadenas (palabras)
    int count;        // Número actual de palabras en la lista
    int capacity;     // Capacidad actual del array (para gestión de memoria)
} ListaPalabras;

// Declaración de funciones (prototipos)
ListaPalabras* inicializar_lista();  // Crea y reserva memoria para una nueva lista
void expandir_lista(ListaPalabras *lista);  // Duplica la capacidad si es necesario
void agregar_palabra(ListaPalabras *lista, const char *palabra);  // Agrega una palabra (permite duplicados)
ListaPalabras* leer_archivo_palabras(const char *nombre_archivo);  // Lee todas las palabras de un archivo
int buscar_palabra_binaria(ListaPalabras *lista, const char *palabra);  // Busca una palabra usando búsqueda binaria
void ordenar_lista(ListaPalabras *lista);  // Ordena las palabras alfabéticamente
void escribir_archivo_palabras(ListaPalabras *lista, const char *nombre_archivo);  // Escribe palabras en un archivo
void liberar_lista(ListaPalabras *lista);  // Libera toda la memoria de la lista
ListaPalabras* encontrar_palabras_faltantes(ListaPalabras *base_html, ListaPalabras *diccionario);  // Encuentra palabras que están en base_html pero no en diccionario


// Función principal del programa
int main(int argc, char *argv[]) {
    // Verificar que se recibieron los argumentos correctos
    if (argc != 4) {
        // Mostrar mensaje de uso si faltan argumentos
        printf("Uso: %s <base_html.txt> <diccionario.txt> <diccionario2.txt>\n", argv[0]);
        printf("Ejemplo: %s base_html.txt diccionario.txt diccionario2.txt\n", argv[0]);
        return 1;  // Retornar código de error
    }
    
    // Obtener los nombres de archivos desde los argumentos
    const char *archivo_base_html = argv[1];     // Archivo con palabras extraídas de HTML
    const char *archivo_diccionario = argv[2];   // Diccionario español estándar
    const char *archivo_diccionario2 = argv[3];  // Archivo de salida con palabras faltantes
    
    // Mostrar encabezado con información de los archivos
    printf("=== COMPARADOR DE DICCIONARIOS ===\n");
    printf("Archivo base HTML: %s\n", archivo_base_html);
    printf("Archivo diccionario: %s\n", archivo_diccionario);
    printf("Archivo salida: %s\n\n", archivo_diccionario2);
    
    // Leer archivo base_html.txt (palabras extraídas de archivos HTML)
    printf("1. Leyendo archivo base HTML...\n");
    ListaPalabras *base_html = leer_archivo_palabras(archivo_base_html);
    if (base_html == NULL) {  // Verificar si hubo error
        return 1;  // Retornar error
    }
    
    // Leer archivo diccionario.txt (diccionario español de referencia)
    printf("\n2. Leyendo archivo diccionario...\n");
    ListaPalabras *diccionario = leer_archivo_palabras(archivo_diccionario);
    if (diccionario == NULL) {  // Verificar si hubo error
        liberar_lista(base_html);  // Liberar memoria antes de salir
        return 1;  // Retornar error
    }
    
    // Verificar si el diccionario está ordenado (necesario para búsqueda binaria)
    printf("\n3. Verificando orden del diccionario...\n");
    int ordenado = 1;  // Asumir que está ordenado
    // Recorrer el diccionario verificando el orden
    for (int i = 0; i < diccionario->count - 1; i++) {
        // Si una palabra es mayor que la siguiente, no está ordenado
        if (strcmp(diccionario->palabras[i], diccionario->palabras[i+1]) > 0) {
            ordenado = 0;  // Marcar como no ordenado
            break;         // Salir del bucle
        }
    }
    
    // Ordenar el diccionario si no está ordenado
    if (!ordenado) {
        printf("El diccionario no está ordenado. Ordenando...\n");
        ordenar_lista(diccionario);  // Ordenar alfabéticamente
    } else {
        printf("El diccionario ya está ordenado.\n");
    }
    
    // Encontrar palabras que están en base_html pero no en diccionario
    printf("\n4. Buscando palabras faltantes...\n");
    ListaPalabras *faltantes = encontrar_palabras_faltantes(base_html, diccionario);
    
    // Ordenar las palabras faltantes alfabéticamente
    printf("\n5. Ordenando palabras faltantes...\n");
    ordenar_lista(faltantes);
    
    // Escribir las palabras faltantes en el archivo de salida
    printf("\n6. Escribiendo archivo diccionario2...\n");
    escribir_archivo_palabras(faltantes, archivo_diccionario2);
    
    // Mostrar estadísticas del análisis
    printf("\n=== ESTADÍSTICAS ===\n");
    printf("Palabras en base_html.txt: %d\n", base_html->count);
    printf("Palabras en diccionario.txt: %d\n", diccionario->count);
    printf("Palabras faltantes: %d\n", faltantes->count);
    // Calcular porcentaje de cobertura del diccionario
    printf("Porcentaje de cobertura del diccionario: %.2f%%\n", 
           ((float)(base_html->count - faltantes->count) / base_html->count) * 100);
    
    // Liberar toda la memoria dinámica asignada
    liberar_lista(base_html);
    liberar_lista(diccionario);
    liberar_lista(faltantes);
    
    printf("\nProceso completado exitosamente.\n");
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

// Función para agregar una palabra a la lista
// A diferencia de combinar_diccionarios, esta función NO verifica duplicados
void agregar_palabra(ListaPalabras *lista, const char *palabra) {
    expandir_lista(lista);  // Expandir si es necesario
    // Reservar memoria para la nueva palabra
    lista->palabras[lista->count] = malloc(strlen(palabra) + 1);
    // Copiar la palabra en la memoria reservada
    strcpy(lista->palabras[lista->count], palabra);
    lista->count++;  // Incrementar contador de palabras
}

// Función para leer todas las palabras de un archivo
// Lee palabra por palabra (separadas por espacios o saltos de línea)
// Retorna un puntero a la lista creada, o NULL si hay error
ListaPalabras* leer_archivo_palabras(const char *nombre_archivo) {
    FILE *archivo = fopen(nombre_archivo, "r");  // Abrir archivo en modo lectura
    if (archivo == NULL) {  // Verificar si hubo error al abrir
        printf("Error: No se pudo abrir el archivo %s\n", nombre_archivo);
        return NULL;  // Retornar NULL en caso de error
    }
    
    // Crear una nueva lista para almacenar las palabras
    ListaPalabras *lista = inicializar_lista();
    char palabra[MAX_WORD_LENGTH];  // Buffer para leer cada palabra
    
    // Leer palabras del archivo (fscanf lee hasta encontrar espacio o salto de línea)
    while (fscanf(archivo, "%s", palabra) == 1) {
        // Eliminar salto de línea al final si existe
        int len = strlen(palabra);
        if (len > 0 && palabra[len-1] == '\n') {
            palabra[len-1] = '\0';  // Reemplazar '\n' con fin de cadena
        }
        agregar_palabra(lista, palabra);  // Agregar palabra a la lista
    }
    
    fclose(archivo);  // Cerrar archivo
    // Mostrar mensaje informativo
    printf("Archivo '%s' leído: %d palabras cargadas.\n", nombre_archivo, lista->count);
    return lista;  // Retornar puntero a la lista
}

// Función de búsqueda binaria en una lista ordenada
// IMPORTANTE: La lista DEBE estar ordenada alfabéticamente para que funcione correctamente
// Retorna 1 si encuentra la palabra, 0 si no la encuentra
int buscar_palabra_binaria(ListaPalabras *lista, const char *palabra) {
    int izquierda = 0;              // Índice del extremo izquierdo
    int derecha = lista->count - 1; // Índice del extremo derecho
    
    // Bucle mientras el rango de búsqueda sea válido
    while (izquierda <= derecha) {
        // Calcular el índice del elemento medio
        int medio = izquierda + (derecha - izquierda) / 2;
        // Comparar la palabra buscada con la palabra en la posición media
        int comparacion = strcmp(lista->palabras[medio], palabra);
        
        if (comparacion == 0) {
            // Si son iguales, se encontró la palabra
            return 1;
        } else if (comparacion < 0) {
            // Si la palabra del medio es menor, buscar en la mitad derecha
            izquierda = medio + 1;
        } else {
            // Si la palabra del medio es mayor, buscar en la mitad izquierda
            derecha = medio - 1;
        }
    }
    
    return 0;  // Palabra no encontrada
}

// Función de comparación para qsort
// Compara dos cadenas alfabéticamente
int comparar_palabras(const void *a, const void *b) {
    // Convertir los punteros void a punteros a cadena
    return strcmp(*(const char**)a, *(const char**)b);
}

// Función para ordenar la lista de palabras alfabéticamente
void ordenar_lista(ListaPalabras *lista) {
    // Usar qsort de la biblioteca estándar (algoritmo rápido de ordenamiento)
    qsort(lista->palabras,      // Array a ordenar
          lista->count,          // Número de elementos
          sizeof(char*),         // Tamaño de cada elemento
          comparar_palabras);    // Función de comparación
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
    printf("Archivo '%s' creado con %d palabras.\n", nombre_archivo, lista->count);
}

// Función para liberar toda la memoria dinámica de la lista
void liberar_lista(ListaPalabras *lista) {
    // Primero liberar cada palabra individual
    for (int i = 0; i < lista->count; i++) {
        free(lista->palabras[i]);  // Liberar memoria de cada palabra
    }
    // Luego liberar el array de punteros
    free(lista->palabras);
    // Finalmente liberar la estructura ListaPalabras
    free(lista);
}

// Función para encontrar palabras faltantes
ListaPalabras* encontrar_palabras_faltantes(ListaPalabras *base_html, ListaPalabras *diccionario) {
    ListaPalabras *faltantes = inicializar_lista();
    int contador_faltantes = 0;
    
    printf("Comparando palabras...\n");
    
    for (int i = 0; i < base_html->count; i++) {
        if (i % 1000 == 0) {
            printf("Procesadas %d de %d palabras...\n", i, base_html->count);
        }
        
        if (!buscar_palabra_binaria(diccionario, base_html->palabras[i])) {
            agregar_palabra(faltantes, base_html->palabras[i]);
            contador_faltantes++;
        }
    }
    
    printf("Encontradas %d palabras faltantes.\n", contador_faltantes);
    return faltantes;
}