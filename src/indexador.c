// Librería para operaciones de entrada/salida estándar
#include <stdio.h>
// Librería para gestión de memoria dinámica y conversiones
#include <stdlib.h>
// Librería para manipulación de cadenas de caracteres
#include <string.h>
// Librería para funciones de clasificación de caracteres
#include <ctype.h>

// Define la longitud máxima permitida para una palabra (100 caracteres)
#define MAX_WORD_LENGTH 100
// Define la longitud máxima de una línea de texto (2048 caracteres)
#define MAX_LINE_LENGTH 2048
// Define el número máximo de documentos que se pueden procesar (10,000)
#define MAX_DOCS 10000
// Define el número máximo de palabras en el vocabulario (50,000)
#define MAX_WORDS 50000

// Estructura para almacenar información de documentos
typedef struct {
    int doc_id;      // Identificador único del documento
    char url[512];   // URL o ruta del documento
} Documento;

// Estructura para frecuencia de palabra en documento
typedef struct {
    int doc_id;      // Identificador del documento
    int frecuencia;  // Número de veces que aparece la palabra en el documento
} DocFreq;

// Estructura para palabra del vocabulario
typedef struct {
    char palabra[MAX_WORD_LENGTH];  // La palabra en sí
    int palabra_id;                 // Identificador único de la palabra
    int num_docs;                   // Número de documentos donde aparece
    DocFreq *doc_freqs;             // Arreglo de frecuencias por documento
    int capacity;                   // Capacidad del arreglo doc_freqs
} PalabraVocab;

// Estructura para almacenar stopwords (palabras irrelevantes)
typedef struct {
    char **palabras;  // Arreglo dinámico de punteros a stopwords
    int count;        // Número de stopwords cargadas
    int capacity;     // Capacidad del arreglo
} Stopwords;

// Variables globales
Documento documentos[MAX_DOCS];  // Arreglo de todos los documentos procesados
int num_documentos = 0;           // Contador de documentos cargados

PalabraVocab vocabulario[MAX_WORDS];  // Arreglo del vocabulario completo
int num_palabras = 0;                 // Contador de palabras en el vocabulario

int max_freq_por_doc[MAX_DOCS];  // Frecuencia máxima de cualquier palabra en cada documento

// Prototipos de funciones
Stopwords* cargar_stopwords(const char *archivo_stopwords);  // Carga stopwords desde archivo
int es_stopword(Stopwords *stops, const char *palabra);  // Verifica si una palabra es stopword
void liberar_stopwords(Stopwords *stops);  // Libera memoria de stopwords
char* limpiar_palabra(const char *palabra);  // Limpia palabra dejando solo letras en minúscula
int buscar_palabra_vocabulario(const char *palabra);  // Busca palabra en vocabulario
void agregar_palabra_vocabulario(const char *palabra);  // Agrega nueva palabra al vocabulario
void agregar_frecuencia_palabra(int palabra_idx, int doc_id);  // Registra ocurrencia de palabra en documento
void procesar_base_texto(const char *archivo_base, Stopwords *stops);  // Procesa archivo base_texto.txt
void calcular_max_frecuencias();  // Calcula frecuencia máxima por documento
void generar_vocabulario_txt();  // Genera archivo vocabulario.txt
void generar_documentos_txt();  // Genera archivo documentos.txt
void generar_listas_invertidas_txt();  // Genera archivo listas_invertidas.txt
int comparar_palabras(const void *a, const void *b);  // Función de comparación para qsort


int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Uso: %s <base_texto.txt> <stopwords.txt>\n", argv[0]);
        printf("Ejemplo: %s txt/base_texto.txt txt/stopwords.txt\n", argv[0]);
        return 1;
    }
    
    const char *archivo_base = argv[1];
    const char *archivo_stopwords = argv[2];
    
    printf("=== INDEXADOR DE DOCUMENTOS ===\n");
    printf("Base de texto: %s\n", archivo_base);
    printf("Stopwords: %s\n\n", archivo_stopwords);
    
    // Inicializar estructuras
    for (int i = 0; i < MAX_DOCS; i++) {
        max_freq_por_doc[i] = 0;
    }
    
    // Cargar stopwords
    printf("1. Cargando stopwords...\n");
    Stopwords *stops = cargar_stopwords(archivo_stopwords);
    if (stops == NULL) {
        return 1;
    }
    
    // Procesar base de texto
    printf("\n2. Procesando base de texto...\n");
    procesar_base_texto(archivo_base, stops);
    
    // Calcular frecuencias máximas
    printf("\n3. Calculando frecuencias máximas por documento...\n");
    calcular_max_frecuencias();
    
    // Ordenar vocabulario
    printf("\n4. Ordenando vocabulario alfabéticamente...\n");
    qsort(vocabulario, num_palabras, sizeof(PalabraVocab), comparar_palabras);
    
    // Reasignar IDs después de ordenar
    for (int i = 0; i < num_palabras; i++) {
        vocabulario[i].palabra_id = i;
    }
    
    // Genera los tres archivos de salida del índice invertido
    printf("\n5. Generando archivos de índice invertido...\n");
    generar_vocabulario_txt();         // Genera vocabulario.txt
    generar_documentos_txt();          // Genera documentos.txt
    generar_listas_invertidas_txt();   // Genera listas_invertidas.txt
    
    // Muestra estadísticas finales
    printf("\n=== ESTADÍSTICAS ===\n");
    printf("Total de documentos: %d\n", num_documentos);        // Total de docs procesados
    printf("Total de palabras relevantes: %d\n", num_palabras); // Total de palabras únicas
    printf("Stopwords cargadas: %d\n", stops->count);           // Total de stopwords
    
    printf("\n=== ARCHIVOS GENERADOS ===\n");  // Lista de archivos generados
    printf("  - vocabulario.txt\n");
    printf("  - documentos.txt\n");
    printf("  - listas_invertidas.txt\n");
    
    printf("\nProceso completado exitosamente.\n");  // Mensaje de finalización
    
    // Libera toda la memoria utilizada
    liberar_stopwords(stops);  // Libera memoria de stopwords
    for (int i = 0; i < num_palabras; i++) {
        free(vocabulario[i].doc_freqs);  // Libera memoria de frecuencias de cada palabra
    }
    
    return 0;  // Retorna éxito
}

// Función para cargar stopwords desde un archivo
// Parámetro: archivo_stopwords - ruta del archivo con stopwords
// Retorna: puntero a estructura Stopwords, o NULL si hay error
Stopwords* cargar_stopwords(const char *archivo_stopwords) {
    FILE *archivo = fopen(archivo_stopwords, "r");  // Abre el archivo en modo lectura
    if (archivo == NULL) {  // Si no se pudo abrir
        printf("Error: No se pudo abrir %s\n", archivo_stopwords);  // Muestra error
        return NULL;  // Retorna NULL para indicar fallo
    }
    
    Stopwords *stops = malloc(sizeof(Stopwords));  // Reserva memoria para la estructura
    stops->capacity = 500;  // Establece capacidad inicial de 500 palabras
    stops->count = 0;  // Inicializa contador en 0
    stops->palabras = malloc(stops->capacity * sizeof(char*));  // Reserva memoria para punteros
    
    char palabra[MAX_WORD_LENGTH];  // Buffer temporal para cada palabra
    while (fgets(palabra, sizeof(palabra), archivo)) {  // Lee línea por línea
        // Elimina el salto de línea al final
        int len = strlen(palabra);  // Obtiene longitud de la palabra
        if (len > 0 && palabra[len-1] == '\n') {  // Si termina con salto de línea
            palabra[len-1] = '\0';  // Reemplaza '\n' con terminador nulo
        }
        
        if (strlen(palabra) > 0) {  // Si la palabra no está vacía
            stops->palabras[stops->count] = malloc(strlen(palabra) + 1);  // Reserva memoria para la palabra
            strcpy(stops->palabras[stops->count], palabra);  // Copia la palabra
            stops->count++;  // Incrementa contador
            
            // Si se alcanza la capacidad, duplica el tamaño del arreglo
            if (stops->count >= stops->capacity) {
                stops->capacity *= 2;  // Duplica capacidad
                stops->palabras = realloc(stops->palabras, stops->capacity * sizeof(char*));  // Reasigna memoria
            }
        }
    }
    
    fclose(archivo);  // Cierra el archivo
    printf("Stopwords cargadas: %d\n", stops->count);  // Muestra cantidad cargada
    return stops;  // Retorna la estructura de stopwords
}

// Función para verificar si una palabra es stopword
// Parámetros: stops - estructura de stopwords, palabra - palabra a verificar
// Retorna: 1 si es stopword, 0 si no lo es
int es_stopword(Stopwords *stops, const char *palabra) {
    for (int i = 0; i < stops->count; i++) {  // Recorre todas las stopwords
        if (strcmp(stops->palabras[i], palabra) == 0) {  // Si encuentra coincidencia
            return 1;  // Es stopword
        }
    }
    return 0;  // No es stopword
}

// Función para liberar memoria de stopwords
// Parámetro: stops - puntero a estructura de stopwords
void liberar_stopwords(Stopwords *stops) {
    for (int i = 0; i < stops->count; i++) {  // Recorre todas las stopwords
        free(stops->palabras[i]);  // Libera memoria de cada palabra individual
    }
    free(stops->palabras);  // Libera el arreglo de punteros
    free(stops);  // Libera la estructura
}

// Función para limpiar palabra dejando solo letras en minúscula
// Parámetro: palabra - palabra a limpiar
// Retorna: puntero a buffer estático con palabra limpia
char* limpiar_palabra(const char *palabra) {
    static char limpia[MAX_WORD_LENGTH];  // Buffer estático para la palabra limpia
    int j = 0;  // Índice para construir la palabra limpia
    
    // Recorre cada carácter de la palabra original
    for (int i = 0; palabra[i] != '\0' && j < MAX_WORD_LENGTH - 1; i++) {
        if (isalpha(palabra[i])) {  // Si es una letra
            limpia[j++] = tolower(palabra[i]);  // Convierte a minúscula y guarda
        }
    }
    limpia[j] = '\0';  // Agrega terminador nulo
    return limpia;  // Retorna la palabra limpia
}

// Función para buscar una palabra en el vocabulario
// Parámetro: palabra - palabra a buscar
// Retorna: índice de la palabra en vocabulario, o -1 si no existe
int buscar_palabra_vocabulario(const char *palabra) {
    for (int i = 0; i < num_palabras; i++) {  // Recorre todo el vocabulario
        if (strcmp(vocabulario[i].palabra, palabra) == 0) {  // Si encuentra coincidencia
            return i;  // Retorna el índice
        }
    }
    return -1;  // No encontrada
}

// Función para agregar una nueva palabra al vocabulario
// Parámetro: palabra - palabra a agregar
void agregar_palabra_vocabulario(const char *palabra) {
    if (num_palabras >= MAX_WORDS) {  // Si se alcanzó el límite
        printf("Advertencia: Se alcanzó el límite de palabras\n");  // Muestra advertencia
        return;  // Sale de la función
    }
    
    strcpy(vocabulario[num_palabras].palabra, palabra);  // Copia la palabra
    vocabulario[num_palabras].palabra_id = num_palabras;  // Asigna ID
    vocabulario[num_palabras].num_docs = 0;  // Inicializa contador de documentos
    vocabulario[num_palabras].capacity = 10;  // Capacidad inicial de 10 documentos
    vocabulario[num_palabras].doc_freqs = malloc(vocabulario[num_palabras].capacity * sizeof(DocFreq));  // Reserva memoria
    num_palabras++;  // Incrementa contador de palabras
}

// Función para agregar o incrementar frecuencia de palabra en documento
// Parámetros: palabra_idx - índice de la palabra en vocabulario, doc_id - ID del documento
void agregar_frecuencia_palabra(int palabra_idx, int doc_id) {
    PalabraVocab *palabra = &vocabulario[palabra_idx];  // Obtiene puntero a la palabra
    
    // Busca si el documento ya está registrado para esta palabra
    int doc_encontrado = -1;
    for (int i = 0; i < palabra->num_docs; i++) {  // Recorre documentos de la palabra
        if (palabra->doc_freqs[i].doc_id == doc_id) {  // Si encuentra el documento
            doc_encontrado = i;  // Guarda el índice
            break;  // Sale del bucle
        }
    }
    
    if (doc_encontrado >= 0) {  // Si el documento ya existe
        // Incrementa la frecuencia de la palabra en ese documento
        palabra->doc_freqs[doc_encontrado].frecuencia++;
    } else {  // Si es la primera vez que aparece en este documento
        // Verifica si necesita expandir el arreglo
        if (palabra->num_docs >= palabra->capacity) {
            palabra->capacity *= 2;  // Duplica la capacidad
            palabra->doc_freqs = realloc(palabra->doc_freqs, palabra->capacity * sizeof(DocFreq));  // Reasigna memoria
        }
        
        // Agrega nueva entrada para el documento
        palabra->doc_freqs[palabra->num_docs].doc_id = doc_id;  // Asigna ID del documento
        palabra->doc_freqs[palabra->num_docs].frecuencia = 1;  // Inicializa frecuencia en 1
        palabra->num_docs++;  // Incrementa contador de documentos
    }
    
    // Actualiza la frecuencia máxima del documento si es necesario
    if (palabra->doc_freqs[doc_encontrado >= 0 ? doc_encontrado : palabra->num_docs - 1].frecuencia > max_freq_por_doc[doc_id]) {
        max_freq_por_doc[doc_id] = palabra->doc_freqs[doc_encontrado >= 0 ? doc_encontrado : palabra->num_docs - 1].frecuencia;
    }
}

// Función para procesar el archivo base_texto.txt y construir el índice invertido
// Parámetros: archivo_base - ruta del archivo base_texto.txt, stops - stopwords a filtrar
void procesar_base_texto(const char *archivo_base, Stopwords *stops) {
    FILE *archivo = fopen(archivo_base, "r");  // Abre el archivo en modo lectura
    if (archivo == NULL) {  // Si no se pudo abrir
        printf("Error: No se pudo abrir %s\n", archivo_base);  // Muestra error
        return;  // Sale de la función
    }
    
    char linea[MAX_LINE_LENGTH];  // Buffer para cada línea
    int doc_actual = -1;  // ID del documento actual (-1 = ninguno)
    int en_texto = 0;  // Bandera: 1 si está dentro de <TEXTO>, 0 si no
    
    while (fgets(linea, sizeof(linea), archivo)) {  // Lee línea por línea
        // Elimina el salto de línea al final
        int len = strlen(linea);  // Obtiene longitud de la línea
        if (len > 0 && linea[len-1] == '\n') {  // Si termina con '\n'
            linea[len-1] = '\0';  // Lo reemplaza con terminador nulo
        }
        
        // Detecta etiqueta de documento: <DOCUMENTO [ID]>
        if (strstr(linea, "<DOCUMENTO") != NULL) {
            char *inicio = strchr(linea, '[');  // Busca '['
            char *fin = strchr(linea, ']');  // Busca ']'
            if (inicio && fin) {  // Si encontró ambos
                char num_str[20];  // Buffer para el número
                int tam = fin - inicio - 1;  // Calcula tamaño del número
                strncpy(num_str, inicio + 1, tam);  // Copia el número
                num_str[tam] = '\0';  // Agrega terminador
                doc_actual = atoi(num_str) - 1;  // Convierte a entero y ajusta a base 0
            }
        }
        // Detecta etiqueta de URL: <URL ruta>
        else if (strstr(linea, "<URL") != NULL) {
            char *inicio = strstr(linea, "<URL");  // Busca "<URL"
            if (inicio) {
                inicio += 5;  // Salta "<URL " (5 caracteres)
                while (*inicio == ' ') inicio++;  // Salta espacios
                
                // Busca el final (puede ser '>' o fin de línea)
                char *fin = strchr(inicio, '>');
                if (fin) {  // Si encuentra '>'
                    int tam = fin - inicio;  // Calcula tamaño de la URL
                    strncpy(documentos[doc_actual].url, inicio, tam);  // Copia la URL
                    documentos[doc_actual].url[tam] = '\0';  // Agrega terminador
                } else {
                    strcpy(documentos[doc_actual].url, inicio);  // Copia hasta fin de línea
                }
                
                documentos[doc_actual].doc_id = doc_actual;  // Asigna ID del documento
                if (doc_actual >= num_documentos) {  // Si es un nuevo documento
                    num_documentos = doc_actual + 1;  // Actualiza contador
                }
            }
        }
        // Detecta inicio del texto: <TEXTO>
        else if (strstr(linea, "<TEXTO>") != NULL) {
            en_texto = 1;  // Activa bandera de texto
        }
        // Detecta fin del texto: </TEXTO>
        else if (strstr(linea, "</TEXTO>") != NULL) {
            en_texto = 0;  // Desactiva bandera de texto
        }
        // Procesa contenido de texto (palabras)
        else if (en_texto && doc_actual >= 0) {  // Si está dentro de <TEXTO> y hay documento
            char *token = strtok(linea, " \t\n");  // Tokeniza por espacios y tabs
            while (token != NULL) {  // Mientras haya tokens
                char *palabra_limpia = limpiar_palabra(token);  // Limpia la palabra
                
                // Si la palabra no está vacía y no es stopword
                if (strlen(palabra_limpia) > 0 && !es_stopword(stops, palabra_limpia)) {
                    int idx = buscar_palabra_vocabulario(palabra_limpia);  // Busca en vocabulario
                    if (idx < 0) {  // Si no existe
                        agregar_palabra_vocabulario(palabra_limpia);  // Agrega al vocabulario
                        idx = num_palabras - 1;  // Obtiene el nuevo índice
                    }
                    agregar_frecuencia_palabra(idx, doc_actual);  // Registra ocurrencia
                }
                
                token = strtok(NULL, " \t\n");  // Obtiene siguiente token
            }
        }
    }
    
    fclose(archivo);  // Cierra el archivo
    printf("Documentos procesados: %d\n", num_documentos);  // Muestra total procesado
}

// Función para calcular frecuencias máximas por documento
void calcular_max_frecuencias() {
    // Recorre todas las palabras del vocabulario
    for (int i = 0; i < num_palabras; i++) {
        // Recorre todos los documentos donde aparece cada palabra
        for (int j = 0; j < vocabulario[i].num_docs; j++) {
            int doc_id = vocabulario[i].doc_freqs[j].doc_id;  // Obtiene ID del documento
            int freq = vocabulario[i].doc_freqs[j].frecuencia;  // Obtiene frecuencia
            
            // Si esta frecuencia es mayor que la máxima registrada
            if (freq > max_freq_por_doc[doc_id]) {
                max_freq_por_doc[doc_id] = freq;  // Actualiza la frecuencia máxima
            }
        }
    }
    printf("Frecuencias máximas calculadas\n");  // Confirma finalización
}

// Función de comparación para ordenar palabras alfabéticamente (para qsort)
// Parámetros: a, b - punteros a elementos PalabraVocab a comparar
// Retorna: <0 si a<b, 0 si a==b, >0 si a>b
int comparar_palabras(const void *a, const void *b) {
    PalabraVocab *pa = (PalabraVocab*)a;  // Convierte a puntero PalabraVocab
    PalabraVocab *pb = (PalabraVocab*)b;  // Convierte a puntero PalabraVocab
    return strcmp(pa->palabra, pb->palabra);  // Compara las palabras alfabéticamente
}

// Función para generar el archivo vocabulario.txt
// Formato: palabra,id,num_documentos
void generar_vocabulario_txt() {
    FILE *archivo = fopen("vocabulario.txt", "w");  // Abre archivo en modo escritura
    if (archivo == NULL) {  // Si no se pudo crear
        printf("Error: No se pudo crear vocabulario.txt\n");  // Muestra error
        return;  // Sale de la función
    }
    
    // Escribe cada palabra del vocabulario
    for (int i = 0; i < num_palabras; i++) {
        fprintf(archivo, "%s,%d,%d\n",  // Formato: palabra,id,num_docs
                vocabulario[i].palabra,  // La palabra
                vocabulario[i].palabra_id,  // ID de la palabra
                vocabulario[i].num_docs);  // Número de documentos donde aparece
    }
    
    fclose(archivo);  // Cierra el archivo
    printf("vocabulario.txt generado\n");  // Confirma generación
}

// Función para generar el archivo documentos.txt
// Formato: id,url
void generar_documentos_txt() {
    FILE *archivo = fopen("documentos.txt", "w");  // Abre archivo en modo escritura
    if (archivo == NULL) {  // Si no se pudo crear
        printf("Error: No se pudo crear documentos.txt\n");  // Muestra error
        return;  // Sale de la función
    }
    
    // Escribe cada documento
    for (int i = 0; i < num_documentos; i++) {
        fprintf(archivo, "%d,%s\n", documentos[i].doc_id, documentos[i].url);  // Formato: id,url
    }
    
    fclose(archivo);  // Cierra el archivo
    printf("documentos.txt generado\n");  // Confirma generación
}

// Función de comparación para ordenar documentos por ID (para qsort)
// Parámetros: a, b - punteros a elementos DocFreq a comparar
// Retorna: diferencia entre IDs (ordena ascendente)
int comparar_doc_ids(const void *a, const void *b) {
    DocFreq *da = (DocFreq*)a;  // Convierte a puntero DocFreq
    DocFreq *db = (DocFreq*)b;  // Convierte a puntero DocFreq
    return da->doc_id - db->doc_id;  // Retorna diferencia (ordena por ID)
}

// Función para generar el archivo listas_invertidas.txt
// Formato: palabra_id,doc_id1,freq_norm1,doc_id2,freq_norm2,...
void generar_listas_invertidas_txt() {
    FILE *archivo = fopen("listas_invertidas.txt", "w");  // Abre archivo en modo escritura
    if (archivo == NULL) {  // Si no se pudo crear
        printf("Error: No se pudo crear listas_invertidas.txt\n");  // Muestra error
        return;  // Sale de la función
    }
    
    // Procesa cada palabra del vocabulario
    for (int i = 0; i < num_palabras; i++) {
        PalabraVocab *palabra = &vocabulario[i];  // Obtiene puntero a la palabra
        
        // Ordena los documentos de esta palabra por ID
        qsort(palabra->doc_freqs, palabra->num_docs, sizeof(DocFreq), comparar_doc_ids);
        
        fprintf(archivo, "%d", palabra->palabra_id);  // Escribe el ID de la palabra
        
        // Escribe cada documento donde aparece la palabra
        for (int j = 0; j < palabra->num_docs; j++) {
            int doc_id = palabra->doc_freqs[j].doc_id;  // ID del documento
            int freq = palabra->doc_freqs[j].frecuencia;  // Frecuencia cruda
            // Calcula frecuencia normalizada (freq / max_freq_del_doc)
            double freq_norm = (max_freq_por_doc[doc_id] > 0) ? 
                              (double)freq / max_freq_por_doc[doc_id] : 0.0;
            
            fprintf(archivo, ",%d,%.3f", doc_id, freq_norm);  // Escribe doc_id y frecuencia normalizada
        }
        
        fprintf(archivo, "\n");  // Nueva línea al final de cada palabra
    }
    
    fclose(archivo);  // Cierra el archivo
    printf("listas_invertidas.txt generado\n");  // Confirma generación
}
