// Bibliotecas estándar de C
#include <stdio.h>      // Para entrada/salida (printf, fopen, etc.)
#include <stdlib.h>     // Para funciones de memoria dinámica (malloc, free, etc.)
#include <string.h>     // Para manipulación de cadenas (strcmp, strcpy, etc.)
#include <ctype.h>      // Para funciones de caracteres (isalpha, tolower, etc.)
#include <math.h>       // Para funciones matemáticas (log10, etc.)

// Definición de constantes para límites del sistema
#define MAX_WORD_LENGTH 100      // Longitud máxima de una palabra
#define MAX_LINE_LENGTH 4096     // Longitud máxima de una línea del archivo
#define MAX_DOCS 10000           // Número máximo de documentos que puede manejar
#define MAX_WORDS 50000          // Número máximo de palabras en el vocabulario
#define MAX_QUERY_WORDS 100      // Número máximo de palabras en una consulta

// Estructura para almacenar información de cada palabra del vocabulario
typedef struct {
    char palabra[MAX_WORD_LENGTH];   // Palabra en formato texto
    int palabra_id;                   // Identificador único de la palabra
    int num_docs_con_palabra;         // Número de documentos que contienen esta palabra
} PalabraVocab;

// Estructura para almacenar información de cada documento
typedef struct {
    int doc_id;        // Identificador único del documento
    char url[512];     // URL o ruta del documento
} Documento;

// Estructura para almacenar la frecuencia de una palabra en un documento específico
typedef struct {
    int doc_id;              // ID del documento
    double frecuencia_norm;  // Frecuencia normalizada de la palabra en el documento
} DocFrec;

// Estructura para representar una lista invertida (posting list) de una palabra
typedef struct {
    int palabra_id;          // ID de la palabra a la que pertenece esta lista
    DocFrec *documentos;     // Array dinámico de documentos que contienen la palabra
    int num_docs;            // Número actual de documentos en la lista
    int capacity;            // Capacidad actual del array (para gestión de memoria)
} ListaInvertida;

// Estructura para almacenar resultados de búsqueda con su ranking
typedef struct {
    int doc_id;        // ID del documento
    double ranking;    // Puntuación de relevancia calculada
} ResultadoBusqueda;

// Variables globales para almacenar los datos del índice invertido
PalabraVocab vocabulario[MAX_WORDS];  // Array de todas las palabras del vocabulario
int num_palabras = 0;                 // Contador de palabras cargadas en el vocabulario

Documento documentos[MAX_DOCS];       // Array de todos los documentos
int num_documentos = 0;               // Contador de documentos cargados

ListaInvertida listas_invertidas[MAX_WORDS];  // Array de listas invertidas (una por palabra)
int num_listas = 0;                           // Contador de listas invertidas cargadas

// Declaración de funciones (prototipos) - permite usar las funciones antes de su definición
void cargar_vocabulario(const char *archivo_vocab);            // Carga el vocabulario desde archivo
void cargar_documentos(const char *archivo_docs);             // Carga la lista de documentos
void cargar_listas_invertidas(const char *archivo_listas);   // Carga las listas invertidas
int buscar_palabra_id(const char *palabra);                  // Busca el ID de una palabra
char* limpiar_palabra(const char *palabra);                  // Limpia y normaliza una palabra
void procesar_consulta(char *consulta);                      // Procesa una consulta de búsqueda
double calcular_w(int palabra_id, int doc_id, double frec_norm);  // Calcula el peso TF-IDF
int comparar_resultados(const void *a, const void *b);       // Compara dos resultados para ordenar
void liberar_memoria();                                       // Libera la memoria dinámica asignada


// Función principal del programa
int main(int argc, char *argv[]) {
    // Verificar que se recibieron suficientes argumentos
    if (argc < 5) {
        // Mostrar mensaje de uso correcto si faltan argumentos
        printf("Uso: %s <vocabulario.txt> <documentos.txt> <listas_invertidas.txt> <palabra1> [palabra2] ...\n", argv[0]);
        printf("Ejemplo: %s vocabulario.txt documentos.txt listas_invertidas.txt hola mundo\n", argv[0]);
        return 1;  // Retornar código de error
    }
    
    // Obtener los nombres de archivos desde los argumentos de línea de comandos
    const char *archivo_vocab = argv[1];       // Primer argumento: archivo de vocabulario
    const char *archivo_docs = argv[2];        // Segundo argumento: archivo de documentos
    const char *archivo_listas = argv[3];      // Tercer argumento: archivo de listas invertidas
    
    // Cargar el índice invertido completo
    printf("Cargando índice invertido...\n");
    cargar_vocabulario(archivo_vocab);         // Cargar todas las palabras del vocabulario
    cargar_documentos(archivo_docs);           // Cargar la información de todos los documentos
    cargar_listas_invertidas(archivo_listas); // Cargar las listas invertidas (qué documentos contienen cada palabra)
    
    // Mostrar estadísticas del índice cargado
    printf("Índice cargado: %d palabras, %d documentos\n\n", num_palabras, num_documentos);
    
    // Construir la consulta concatenando todos los argumentos restantes
    char consulta[1024] = "";  // Buffer para almacenar la consulta completa
    for (int i = 4; i < argc; i++) {  // Iterar desde el cuarto argumento en adelante
        strcat(consulta, argv[i]);     // Agregar la palabra a la consulta
        if (i < argc - 1) {            // Si no es la última palabra
            strcat(consulta, " ");     // Agregar un espacio entre palabras
        }
    }
    
    // Procesar la consulta y mostrar resultados
    printf("Consulta [ %s ]:\n", consulta);
    procesar_consulta(consulta);  // Ejecutar la búsqueda
    
    // Liberar toda la memoria dinámica asignada
    liberar_memoria();
    
    return 0;  // Retornar éxito
}

// Función para cargar el vocabulario desde un archivo
void cargar_vocabulario(const char *archivo_vocab) {
    FILE *archivo = fopen(archivo_vocab, "r");  // Abrir archivo en modo lectura
    if (archivo == NULL) {  // Verificar si la apertura fue exitosa
        printf("Error: No se pudo abrir %s\n", archivo_vocab);
        exit(1);  // Terminar programa con código de error
    }
    
    char linea[MAX_LINE_LENGTH];  // Buffer para leer cada línea
    // Leer el archivo línea por línea
    while (fgets(linea, sizeof(linea), archivo)) {
        char palabra[MAX_WORD_LENGTH];  // Buffer para la palabra
        int palabra_id, num_docs;       // Variables para ID y número de documentos
        
        // Parsear la línea en formato: palabra,ID,num_docs
        if (sscanf(linea, "%[^,],%d,%d", palabra, &palabra_id, &num_docs) == 3) {
            // Guardar la palabra en el array global de vocabulario
            strcpy(vocabulario[num_palabras].palabra, palabra);
            vocabulario[num_palabras].palabra_id = palabra_id;
            vocabulario[num_palabras].num_docs_con_palabra = num_docs;
            num_palabras++;  // Incrementar contador de palabras
        }
    }
    
    fclose(archivo);  // Cerrar el archivo
}

// Función para cargar la lista de documentos desde un archivo
void cargar_documentos(const char *archivo_docs) {
    FILE *archivo = fopen(archivo_docs, "r");  // Abrir archivo en modo lectura
    if (archivo == NULL) {  // Verificar si hubo error al abrir
        printf("Error: No se pudo abrir %s\n", archivo_docs);
        exit(1);  // Terminar con código de error
    }
    
    char linea[MAX_LINE_LENGTH];  // Buffer para cada línea
    // Leer archivo línea por línea
    while (fgets(linea, sizeof(linea), archivo)) {
        int doc_id;       // ID del documento
        char url[512];    // URL del documento
        
        // Buscar la primera coma en la línea (separa ID de URL)
        char *coma = strchr(linea, ',');
        if (coma != NULL) {  // Si encontró la coma
            *coma = '\0';    // Reemplazar coma con fin de cadena (divide la línea)
            doc_id = atoi(linea);  // Convertir primera parte a entero (ID)
            strcpy(url, coma + 1); // Copiar segunda parte (URL)
            
            // Eliminar salto de línea al final de la URL si existe
            int len = strlen(url);
            if (len > 0 && url[len-1] == '\n') {
                url[len-1] = '\0';  // Reemplazar '\n' con fin de cadena
            }
            
            // Guardar documento en el array global usando el ID como índice
            documentos[doc_id].doc_id = doc_id;
            strcpy(documentos[doc_id].url, url);
            
            // Actualizar contador de documentos si es necesario
            if (doc_id >= num_documentos) {
                num_documentos = doc_id + 1;
            }
        }
    }
    
    fclose(archivo);  // Cerrar archivo
}

// Función para cargar las listas invertidas desde un archivo
// Una lista invertida contiene todos los documentos donde aparece cada palabra
void cargar_listas_invertidas(const char *archivo_listas) {
    FILE *archivo = fopen(archivo_listas, "r");  // Abrir archivo en modo lectura
    if (archivo == NULL) {  // Verificar si hubo error
        printf("Error: No se pudo abrir %s\n", archivo_listas);
        exit(1);  // Terminar con código de error
    }
    
    char linea[MAX_LINE_LENGTH];  // Buffer para cada línea
    // Leer el archivo línea por línea
    while (fgets(linea, sizeof(linea), archivo)) {
        // Parsear línea en formato: palabra_id,doc_id,frec,doc_id,frec,...
        int palabra_id;
        if (sscanf(linea, "%d", &palabra_id) == 1) {  // Leer el ID de la palabra
            // Inicializar la lista invertida para esta palabra
            listas_invertidas[num_listas].palabra_id = palabra_id;
            listas_invertidas[num_listas].capacity = 100;  // Capacidad inicial
            listas_invertidas[num_listas].num_docs = 0;    // Sin documentos aún
            // Reservar memoria dinámica para el array de documentos
            listas_invertidas[num_listas].documentos = malloc(
                listas_invertidas[num_listas].capacity * sizeof(DocFrec)
            );
            
            // Buscar la primera coma (después del palabra_id)
            char *ptr = strchr(linea, ',');
            while (ptr != NULL) {  // Mientras haya más pares doc_id,frecuencia
                ptr++;  // Saltar la coma para leer el siguiente valor
                
                int doc_id;      // ID del documento
                double frec;     // Frecuencia normalizada
                if (sscanf(ptr, "%d,%lf", &doc_id, &frec) == 2) {  // Leer par doc_id,frecuencia
                    // Expandir el array si se alcanzó la capacidad máxima
                    if (listas_invertidas[num_listas].num_docs >= listas_invertidas[num_listas].capacity) {
                        listas_invertidas[num_listas].capacity *= 2;  // Duplicar capacidad
                        // Reasignar memoria con nuevo tamaño
                        listas_invertidas[num_listas].documentos = realloc(
                            listas_invertidas[num_listas].documentos,
                            listas_invertidas[num_listas].capacity * sizeof(DocFrec)
                        );
                    }
                    
                    // Guardar el doc_id y frecuencia en el array
                    listas_invertidas[num_listas].documentos[listas_invertidas[num_listas].num_docs].doc_id = doc_id;
                    listas_invertidas[num_listas].documentos[listas_invertidas[num_listas].num_docs].frecuencia_norm = frec;
                    listas_invertidas[num_listas].num_docs++;  // Incrementar contador
                    
                    // Avanzar al siguiente par doc_id,frecuencia
                    ptr = strchr(ptr, ',');  // Buscar siguiente coma (después de doc_id)
                    if (ptr != NULL) {
                        ptr = strchr(ptr + 1, ',');  // Buscar coma después de frecuencia
                    }
                } else {
                    break;  // Si no se pudo leer el par, salir del bucle
                }
            }
            
            num_listas++;  // Incrementar contador de listas invertidas
        }
    }
    
    fclose(archivo);  // Cerrar archivo
}

// Función para buscar el ID de una palabra en el vocabulario
// Retorna el palabra_id si la encuentra, o -1 si no existe
int buscar_palabra_id(const char *palabra) {
    // Recorrer todo el vocabulario
    for (int i = 0; i < num_palabras; i++) {
        // Comparar palabra buscada con cada palabra del vocabulario
        if (strcmp(vocabulario[i].palabra, palabra) == 0) {
            return vocabulario[i].palabra_id;  // Retornar ID si coincide
        }
    }
    return -1;  // Retornar -1 si no se encontró
}

// Función para limpiar y normalizar una palabra
// Elimina caracteres no alfabéticos y convierte a minúsculas
char* limpiar_palabra(const char *palabra) {
    static char limpia[MAX_WORD_LENGTH];  // Buffer estático para el resultado
    int j = 0;  // Índice para la palabra limpia
    
    // Recorrer cada carácter de la palabra original
    for (int i = 0; palabra[i] != '\0' && j < MAX_WORD_LENGTH - 1; i++) {
        if (isalpha(palabra[i])) {  // Si el carácter es alfabético
            limpia[j++] = tolower(palabra[i]);  // Convertir a minúscula y agregar
        }
    }
    limpia[j] = '\0';  // Agregar terminador de cadena
    return limpia;     // Retornar puntero a la palabra limpia
}

// Función para calcular el peso W(t,i) usando TF-IDF
// W(t,i) = log(N / D(t)) * Frec(t,i)
// Donde: N = total de documentos, D(t) = docs con la palabra, Frec(t,i) = frecuencia normalizada
double calcular_w(int palabra_id, int doc_id, double frec_norm) {
    // Buscar D(t) - número de documentos donde aparece la palabra
    int d_t = 0;  // Inicializar contador
    for (int i = 0; i < num_palabras; i++) {
        // Buscar la palabra en el vocabulario por su ID
        if (vocabulario[i].palabra_id == palabra_id) {
            d_t = vocabulario[i].num_docs_con_palabra;  // Obtener número de docs
            break;  // Salir del bucle al encontrarla
        }
    }
    
    // Si no hay documentos con esta palabra, retornar 0
    if (d_t == 0) {
        return 0.0;
    }
    
    // N = número total de documentos en la colección
    int n = num_documentos;
    
    // Calcular W(t,i) = log10(N / D(t)) * Frec(t,i)
    // IDF = log(N/D(t)) - mide qué tan rara/importante es la palabra
    // TF = Frec(t,i) - frecuencia normalizada de la palabra en el documento
    double w = log10((double)n / (double)d_t) * frec_norm;
    
    return w;  // Retornar el peso calculado
}

// Función de comparación para ordenar resultados de búsqueda
// Se usa con qsort() para ordenar por ranking descendente (mayor primero)
int comparar_resultados(const void *a, const void *b) {
    ResultadoBusqueda *ra = (ResultadoBusqueda*)a;  // Convertir primer puntero
    ResultadoBusqueda *rb = (ResultadoBusqueda*)b;  // Convertir segundo puntero
    
    // Comparar rankings (orden descendente: mayor ranking primero)
    if (rb->ranking > ra->ranking) return 1;   // b es mayor que a
    if (rb->ranking < ra->ranking) return -1;  // b es menor que a
    return 0;  // Son iguales
}

// Función principal para procesar una consulta de búsqueda
void procesar_consulta(char *consulta) {
    char *palabras_consulta[MAX_QUERY_WORDS];  // Array de punteros a palabras de la consulta
    int num_palabras_consulta = 0;             // Contador de palabras en la consulta
    
    // Tokenizar (dividir) la consulta en palabras individuales
    char consulta_copia[1024];  // Crear copia porque strtok modifica la cadena
    strcpy(consulta_copia, consulta);
    
    // Dividir la consulta usando espacios, tabuladores y saltos de línea como delimitadores
    char *token = strtok(consulta_copia, " \t\n");
    while (token != NULL && num_palabras_consulta < MAX_QUERY_WORDS) {
        char *palabra_limpia = limpiar_palabra(token);  // Limpiar cada palabra
        if (strlen(palabra_limpia) > 0) {  // Si la palabra no está vacía
            // Reservar memoria y copiar la palabra limpia
            palabras_consulta[num_palabras_consulta] = malloc(strlen(palabra_limpia) + 1);
            strcpy(palabras_consulta[num_palabras_consulta], palabra_limpia);
            num_palabras_consulta++;  // Incrementar contador
        }
        token = strtok(NULL, " \t\n");  // Obtener siguiente palabra
    }
    
    // Verificar si hay palabras válidas en la consulta
    if (num_palabras_consulta == 0) {
        printf("No se encontraron palabras válidas en la consulta.\n");
        return;
    }
    
    // Obtener los IDs de las palabras que existen en el vocabulario
    int palabra_ids[MAX_QUERY_WORDS];  // Array para almacenar IDs
    int palabras_validas = 0;          // Contador de palabras encontradas
    
    for (int i = 0; i < num_palabras_consulta; i++) {
        int palabra_id = buscar_palabra_id(palabras_consulta[i]);  // Buscar palabra
        if (palabra_id >= 0) {  // Si la palabra existe en el vocabulario
            palabra_ids[palabras_validas++] = palabra_id;  // Guardar su ID
        } else {
            // Advertir sobre palabras no encontradas
            printf("Advertencia: palabra '%s' no encontrada en vocabulario\n", palabras_consulta[i]);
        }
    }
    
    // Si ninguna palabra está en el vocabulario, no hay resultados
    if (palabras_validas == 0) {
        printf("Ninguna palabra de la consulta está en el vocabulario.\n");
        // Liberar memoria de las palabras de la consulta
        for (int i = 0; i < num_palabras_consulta; i++) {
            free(palabras_consulta[i]);
        }
        return;
    }
    
    // Inicializar array de rankings para cada documento
    double rankings[MAX_DOCS];
    for (int i = 0; i < num_documentos; i++) {
        rankings[i] = 0.0;  // Inicializar todos los rankings en 0
    }
    
    // Para cada palabra en la consulta, calcular su contribución al ranking
    for (int i = 0; i < palabras_validas; i++) {
        int palabra_id = palabra_ids[i];  // Obtener ID de la palabra actual
        
        // Buscar la lista invertida correspondiente a esta palabra
        for (int j = 0; j < num_listas; j++) {
            if (listas_invertidas[j].palabra_id == palabra_id) {  // Encontramos la lista
                // Para cada documento en la lista invertida
                for (int k = 0; k < listas_invertidas[j].num_docs; k++) {
                    int doc_id = listas_invertidas[j].documentos[k].doc_id;  // ID del documento
                    double frec_norm = listas_invertidas[j].documentos[k].frecuencia_norm;  // Frecuencia normalizada
                    
                    // Calcular el peso W(t,i) y sumarlo al ranking del documento
                    double w = calcular_w(palabra_id, doc_id, frec_norm);
                    rankings[doc_id] += w;  // Acumular peso en el ranking
                }
                break;  // Salir del bucle, ya encontramos la lista
            }
        }
    }
    
    // Recopilar todos los documentos que tienen ranking mayor a 0
    ResultadoBusqueda resultados[MAX_DOCS];  // Array para almacenar resultados
    int num_resultados = 0;                   // Contador de resultados
    
    for (int i = 0; i < num_documentos; i++) {
        if (rankings[i] > 0.0) {  // Si el documento es relevante
            resultados[num_resultados].doc_id = i;           // Guardar ID
            resultados[num_resultados].ranking = rankings[i]; // Guardar ranking
            num_resultados++;  // Incrementar contador
        }
    }
    
    // Ordenar los resultados por ranking (de mayor a menor)
    qsort(resultados, num_resultados, sizeof(ResultadoBusqueda), comparar_resultados);
    
    // Mostrar los resultados al usuario
    if (num_resultados == 0) {
        printf("No se encontraron documentos relevantes.\n");
    } else {
        // Imprimir cada resultado en formato: (doc_id, ranking)
        for (int i = 0; i < num_resultados; i++) {
            // Mostrar doc_id+1 para que coincida con numeración desde 1
            printf("(%d,%.3f) ", resultados[i].doc_id + 1, resultados[i].ranking);
        }
        printf("\n");  // Salto de línea al final
    }
    
    // Liberar la memoria asignada para las palabras de la consulta
    for (int i = 0; i < num_palabras_consulta; i++) {
        free(palabras_consulta[i]);
    }
}

// Función para liberar toda la memoria dinámica asignada
void liberar_memoria() {
    // Recorrer todas las listas invertidas
    for (int i = 0; i < num_listas; i++) {
        // Liberar el array de documentos de cada lista invertida
        free(listas_invertidas[i].documentos);
    }
}
