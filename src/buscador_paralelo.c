// Bibliotecas estándar de C
#include <stdio.h>      // Para entrada/salida (printf, fopen, etc.)
#include <stdlib.h>     // Para funciones de memoria dinámica (malloc, free, etc.)
#include <string.h>     // Para manipulación de cadenas (strcmp, strcpy, etc.)
#include <ctype.h>      // Para funciones de caracteres (isalpha, tolower, etc.)
#include <math.h>       // Para funciones matemáticas (log10, etc.)
#include <mpi.h>        // Para programación paralela con MPI (Message Passing Interface)

// Definición de constantes para límites del sistema
#define MAX_WORD_LENGTH 100      // Longitud máxima de una palabra
#define MAX_LINE_LENGTH 4096     // Longitud máxima de una línea del archivo
#define MAX_DOCS 10000           // Número máximo de documentos que puede manejar
#define MAX_WORDS 50000          // Número máximo de palabras en el vocabulario
#define MAX_QUERY_LENGTH 1024    // Longitud máxima de una consulta
#define MAX_URL_LENGTH 512       // Longitud máxima de una URL

// Estructura para almacenar información de cada palabra del vocabulario
typedef struct {
    char palabra[MAX_WORD_LENGTH];   // Palabra en formato texto
    int palabra_id;                   // Identificador único de la palabra
    int num_docs_con_palabra;         // Número de documentos que contienen esta palabra
} PalabraVocab;

// Estructura para almacenar información de cada documento
typedef struct {
    int doc_id;                  // Identificador único del documento
    char url[MAX_URL_LENGTH];    // URL o ruta del documento
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

// Estructura para almacenar resultados de búsqueda con su ranking y URL
typedef struct {
    int doc_id;                  // ID del documento
    double ranking;              // Puntuación de relevancia calculada
    char url[MAX_URL_LENGTH];    // URL del documento (para mostrar en resultados)
} ResultadoBusqueda;

// Variables globales para almacenar los datos del índice invertido local de cada procesador
PalabraVocab vocabulario[MAX_WORDS];  // Array de palabras del vocabulario local
int num_palabras = 0;                 // Contador de palabras cargadas

Documento documentos[MAX_DOCS];       // Array de documentos locales
int num_documentos = 0;               // Contador de documentos cargados

ListaInvertida listas_invertidas[MAX_WORDS];  // Array de listas invertidas locales
int num_listas = 0;                           // Contador de listas invertidas cargadas

// Declaración de funciones (prototipos)
void cargar_indice_local(int rank);  // Carga el índice invertido específico de un procesador
int buscar_palabra_id(const char *palabra);  // Busca el ID de una palabra
char* limpiar_palabra(const char *palabra);  // Limpia y normaliza una palabra
void procesar_consulta_local(char *consulta, int K, ResultadoBusqueda *resultados_locales, int *num_resultados);  // Procesa una consulta localmente
double calcular_w(int palabra_id, int doc_id, double frec_norm);  // Calcula el peso TF-IDF
int comparar_resultados(const void *a, const void *b);  // Compara dos resultados para ordenar
void liberar_memoria();  // Libera la memoria dinámica asignada
void leer_consultas(const char *archivo_entrada, char consultas[][MAX_QUERY_LENGTH], int *num_consultas, int Q);  // Lee consultas de un archivo
void escribir_resultados(const char *archivo_salida, int rank, int consulta_id, ResultadoBusqueda *resultados, int num_resultados);  // Escribe resultados en archivo


// Función principal del programa paralelo
int main(int argc, char *argv[]) {
    int rank, size;                // rank = ID del procesador, size = total de procesadores
    int Q = 10;  // Número de consultas por lote (valor por defecto)
    int K = 10;  // Número de mejores documentos a retornar (valor por defecto)
    
    // Inicializar el entorno MPI
    MPI_Init(&argc, &argv);                    // Inicializar MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);      // Obtener el ID de este procesador
    MPI_Comm_size(MPI_COMM_WORLD, &size);      // Obtener el número total de procesadores
    
    // Verificar que se proporcionaron los argumentos necesarios
    if (argc < 2) {
        if (rank == 0) {  // Solo el procesador 0 imprime el mensaje de ayuda
            printf("Uso: mpirun -np <P> %s <Q> [K]\n", argv[0]);
            printf("  P = número de procesadores\n");
            printf("  Q = consultas por lote (default: 10)\n");
            printf("  K = top K documentos por respuesta (default: 10)\n");
            printf("\nAsume que existen archivos:\n");
            printf("  - entrada_<rank>.txt : consultas para cada procesador\n");
            printf("  - vocabulario_<rank>.txt : vocabulario local\n");
            printf("  - documentos_<rank>.txt : documentos locales\n");
            printf("  - listas_invertidas_<rank>.txt : listas invertidas locales\n");
        }
        MPI_Finalize();  // Finalizar MPI antes de salir
        return 1;        // Retornar código de error
    }
    
    // Leer parámetros de línea de comandos
    Q = atoi(argv[1]);  // Convertir primer argumento a entero (consultas por lote)
    if (argc >= 3) {
        K = atoi(argv[2]);  // Convertir segundo argumento a entero si existe (top K)
    }
    
    // Mostrar información del sistema (solo procesador 0)
    if (rank == 0) {
        printf("=== BUSCADOR PARALELO ===\n");
        printf("Procesadores: %d\n", size);
        printf("Consultas por lote (Q): %d\n", Q);
        printf("Top K documentos: %d\n\n", K);
    }
    
    // Cargar el índice invertido local de cada procesador
    if (rank == 0) printf("Procesador %d: Cargando índice local...\n", rank);
    cargar_indice_local(rank);  // Cada procesador carga su porción del índice
    MPI_Barrier(MPI_COMM_WORLD);  // Sincronizar todos los procesadores (esperar a que todos terminen de cargar)
    
    // Mensaje de confirmación (solo procesador 0)
    if (rank == 0) {
        printf("\nTodos los procesadores han cargado sus índices.\n");
        printf("Iniciando procesamiento de consultas...\n\n");
    }
    
    // Leer las consultas desde el archivo de entrada específico de este procesador
    char consultas[1000][MAX_QUERY_LENGTH];  // Array para almacenar las consultas
    int num_consultas_totales = 0;           // Contador de consultas leídas
    char archivo_entrada[256];
    sprintf(archivo_entrada, "entrada_%d.txt", rank);  // Nombre del archivo: entrada_0.txt, entrada_1.txt, etc.
    leer_consultas(archivo_entrada, consultas, &num_consultas_totales, Q);
    
    // Verificar si hay consultas
    if (num_consultas_totales == 0) {
        if (rank == 0) printf("Procesador %d: No hay consultas en %s\n", rank, archivo_entrada);
    }
    
    // Preparar archivo de salida para este procesador
    char archivo_salida[256];
    sprintf(archivo_salida, "salida_%d.txt", rank);  // Nombre del archivo: salida_0.txt, salida_1.txt, etc.
    FILE *salida = fopen(archivo_salida, "w");
    if (salida) {
        fprintf(salida, "=== RESULTADOS PROCESADOR %d ===\n\n", rank);
        fclose(salida);
    }
    
    // Procesar las consultas en lotes de tamaño Q
    int consulta_id = 0;  // Contador global de consultas procesadas
    for (int lote = 0; lote < num_consultas_totales; lote += Q) {
        // Calcular cuántas consultas hay en este lote
        int consultas_en_lote = (lote + Q <= num_consultas_totales) ? Q : (num_consultas_totales - lote);
        
        // Procesar cada consulta del lote actual
        for (int i = 0; i < consultas_en_lote; i++) {
            char *consulta = consultas[lote + i];  // Obtener la consulta actual
            
            // Broadcast: enviar la consulta a todos los procesadores desde este procesador
            // Todos los procesadores ejecutan MPI_Bcast, pero solo el que tiene rank actual envía
            MPI_Bcast(consulta, MAX_QUERY_LENGTH, MPI_CHAR, rank, MPI_COMM_WORLD);
            
            // Cada procesador busca en su índice local
            ResultadoBusqueda resultados_locales[MAX_DOCS];  // Array para resultados locales
            int num_resultados_locales = 0;                   // Contador de resultados locales
            procesar_consulta_local(consulta, K, resultados_locales, &num_resultados_locales);
            
            // Determinar cuántos resultados enviar (máximo K)
            int resultados_a_enviar = (num_resultados_locales < K) ? num_resultados_locales : K;
            
            // Recopilar resultados de todos los procesadores en el procesador que hizo la consulta
            ResultadoBusqueda todos_resultados[MAX_DOCS];  // Array para todos los resultados
            int num_todos_resultados = 0;                   // Contador de resultados totales
            
            // Recibir resultados de cada procesador
            for (int p = 0; p < size; p++) {
                if (p == rank) {
                    // Si es este procesador, copiar sus resultados locales directamente
                    for (int j = 0; j < resultados_a_enviar; j++) {
                        todos_resultados[num_todos_resultados++] = resultados_locales[j];
                    }
                } else {
                    // Si es otro procesador, recibir sus resultados por MPI
                    int num_recibidos;  // Número de resultados que enviará el otro procesador
                    MPI_Status status;  // Estado de la recepción MPI
                    
                    // Recibir primero el número de resultados
                    MPI_Recv(&num_recibidos, 1, MPI_INT, p, 0, MPI_COMM_WORLD, &status);
                    
                    if (num_recibidos > 0) {  // Si hay resultados que recibir
                        ResultadoBusqueda temp[MAX_DOCS];  // Buffer temporal
                        // Recibir el array de resultados como bytes
                        MPI_Recv(temp, num_recibidos * sizeof(ResultadoBusqueda), MPI_BYTE, 
                                p, 1, MPI_COMM_WORLD, &status);
                        
                        // Copiar resultados recibidos al array total
                        for (int j = 0; j < num_recibidos; j++) {
                            todos_resultados[num_todos_resultados++] = temp[j];
                        }
                    }
                }
            }
            
            // También este procesador debe enviar sus resultados a todos los demás
            for (int p = 0; p < size; p++) {
                if (p != rank) {  // No enviarse a sí mismo
                    // Enviar primero el número de resultados
                    MPI_Send(&resultados_a_enviar, 1, MPI_INT, p, 0, MPI_COMM_WORLD);
                    if (resultados_a_enviar > 0) {  // Si hay resultados que enviar
                        // Enviar el array de resultados como bytes
                        MPI_Send(resultados_locales, resultados_a_enviar * sizeof(ResultadoBusqueda), 
                                MPI_BYTE, p, 1, MPI_COMM_WORLD);
                    }
                }
            }
            
            // Ordenar todos los resultados recopilados por ranking (mayor a menor)
            qsort(todos_resultados, num_todos_resultados, sizeof(ResultadoBusqueda), comparar_resultados);
            
            // Tomar solo los mejores K resultados finales
            int resultados_finales = (num_todos_resultados < K) ? num_todos_resultados : K;
            
            // Escribir los resultados en el archivo de salida
            escribir_resultados(archivo_salida, rank, consulta_id, todos_resultados, resultados_finales);
            
            consulta_id++;  // Incrementar contador de consultas procesadas
        }
    }
    
    // Mensaje de finalización (solo procesador 0)
    if (rank == 0) {
        printf("\nProcesamiento completado.\n");
        printf("Resultados en archivos: salida_0.txt, salida_1.txt, ..., salida_%d.txt\n", size-1);
    }
    
    // Liberar toda la memoria dinámica asignada
    liberar_memoria();
    
    // Finalizar el entorno MPI
    MPI_Finalize();
    return 0;  // Retornar éxito
}

// Función para cargar el índice invertido local de un procesador específico
// Cada procesador tiene su propia copia parcial del índice (particionado)
void cargar_indice_local(int rank) {
    char archivo[256];  // Buffer para el nombre del archivo
    
    // ========== CARGAR VOCABULARIO ==========
    sprintf(archivo, "vocabulario_%d.txt", rank);  // Nombre: vocabulario_0.txt, vocabulario_1.txt, etc.
    FILE *f = fopen(archivo, "r");  // Abrir archivo en modo lectura
    if (!f) {  // Si no se pudo abrir
        printf("Error: Procesador %d no puede abrir %s\n", rank, archivo);
        MPI_Abort(MPI_COMM_WORLD, 1);  // Abortar todos los procesos MPI con código de error
    }
    
    char linea[MAX_LINE_LENGTH];  // Buffer para cada línea
    // Leer vocabulario línea por línea
    while (fgets(linea, sizeof(linea), f)) {
        char palabra[MAX_WORD_LENGTH];  // Buffer para la palabra
        int palabra_id, num_docs;       // ID de la palabra y número de documentos
        
        // Parsear línea en formato: palabra,ID,num_docs
        if (sscanf(linea, "%[^,],%d,%d", palabra, &palabra_id, &num_docs) == 3) {
            // Guardar en el vocabulario local
            strcpy(vocabulario[num_palabras].palabra, palabra);
            vocabulario[num_palabras].palabra_id = palabra_id;
            vocabulario[num_palabras].num_docs_con_palabra = num_docs;
            num_palabras++;  // Incrementar contador
        }
    }
    fclose(f);  // Cerrar archivo
    
    // ========== CARGAR DOCUMENTOS ==========
    sprintf(archivo, "documentos_%d.txt", rank);  // Nombre: documentos_0.txt, documentos_1.txt, etc.
    f = fopen(archivo, "r");  // Abrir archivo
    if (!f) {  // Si no se pudo abrir
        printf("Error: Procesador %d no puede abrir %s\n", rank, archivo);
        MPI_Abort(MPI_COMM_WORLD, 1);  // Abortar todos los procesos MPI
    }
    
    // Leer documentos línea por línea
    while (fgets(linea, sizeof(linea), f)) {
        int doc_id;                  // ID del documento
        char url[MAX_URL_LENGTH];    // URL del documento
        
        // Buscar la primera coma (separa ID de URL)
        char *coma = strchr(linea, ',');
        if (coma != NULL) {  // Si se encontró la coma
            *coma = '\0';    // Reemplazarla con fin de cadena (divide la línea)
            doc_id = atoi(linea);  // Convertir primera parte a entero (ID)
            strcpy(url, coma + 1); // Copiar segunda parte (URL)
            
            // Eliminar salto de línea al final de la URL si existe
            int len = strlen(url);
            if (len > 0 && url[len-1] == '\n') {
                url[len-1] = '\0';
            }
            
            // Guardar documento en el array usando ID como índice
            documentos[doc_id].doc_id = doc_id;
            strcpy(documentos[doc_id].url, url);
            
            // Actualizar contador si es necesario
            if (doc_id >= num_documentos) {
                num_documentos = doc_id + 1;
            }
        }
    }
    fclose(f);  // Cerrar archivo
    
    // ========== CARGAR LISTAS INVERTIDAS ==========
    sprintf(archivo, "listas_invertidas_%d.txt", rank);  // Nombre: listas_invertidas_0.txt, etc.
    f = fopen(archivo, "r");  // Abrir archivo
    if (!f) {  // Si no se pudo abrir
        printf("Error: Procesador %d no puede abrir %s\n", rank, archivo);
        MPI_Abort(MPI_COMM_WORLD, 1);  // Abortar todos los procesos MPI
    }
    
    // Leer listas invertidas línea por línea
    while (fgets(linea, sizeof(linea), f)) {
        int palabra_id;  // ID de la palabra
        if (sscanf(linea, "%d", &palabra_id) == 1) {  // Leer el ID de la palabra
            // Inicializar la lista invertida para esta palabra
            listas_invertidas[num_listas].palabra_id = palabra_id;
            listas_invertidas[num_listas].capacity = 100;  // Capacidad inicial
            listas_invertidas[num_listas].num_docs = 0;    // Sin documentos aún
            // Reservar memoria dinámica para el array de documentos
            listas_invertidas[num_listas].documentos = malloc(
                listas_invertidas[num_listas].capacity * sizeof(DocFrec)
            );
            
            // Parsear los pares (doc_id, frecuencia) de esta palabra
            char *ptr = strchr(linea, ',');  // Buscar la primera coma
            while (ptr != NULL) {  // Mientras haya más datos
                ptr++;  // Saltar la coma
                
                int doc_id;      // ID del documento
                double frec;     // Frecuencia normalizada
                if (sscanf(ptr, "%d,%lf", &doc_id, &frec) == 2) {  // Leer par doc_id,frecuencia
                    // Expandir el array si se alcanzó la capacidad
                    if (listas_invertidas[num_listas].num_docs >= listas_invertidas[num_listas].capacity) {
                        listas_invertidas[num_listas].capacity *= 2;  // Duplicar capacidad
                        // Reasignar memoria con nuevo tamaño
                        listas_invertidas[num_listas].documentos = realloc(
                            listas_invertidas[num_listas].documentos,
                            listas_invertidas[num_listas].capacity * sizeof(DocFrec)
                        );
                    }
                    
                    // Guardar doc_id y frecuencia en el array
                    listas_invertidas[num_listas].documentos[listas_invertidas[num_listas].num_docs].doc_id = doc_id;
                    listas_invertidas[num_listas].documentos[listas_invertidas[num_listas].num_docs].frecuencia_norm = frec;
                    listas_invertidas[num_listas].num_docs++;  // Incrementar contador
                    
                    // Avanzar al siguiente par
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
    fclose(f);  // Cerrar archivo
    
    // Mostrar estadísticas del índice cargado
    printf("Procesador %d: Índice cargado - %d palabras, %d documentos, %d listas\n", 
           rank, num_palabras, num_documentos, num_listas);
}

// Función para buscar el ID de una palabra en el vocabulario local
// Retorna el palabra_id si la encuentra, o -1 si no existe
int buscar_palabra_id(const char *palabra) {
    // Recorrer todo el vocabulario local
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
// W(t,i) = log10(N / D(t)) * Frec(t,i)
double calcular_w(int palabra_id, int doc_id, double frec_norm) {
    (void)doc_id;  // Evitar warning de parámetro no usado
    
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
    
    // N = número total de documentos en la partición local
    int n = num_documentos;
    // Calcular peso TF-IDF: W(t,i) = log10(N/D(t)) * Frec(t,i)
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

// Función para procesar una consulta de búsqueda en el índice local
// Cada procesador ejecuta esta función sobre su partición de datos
void procesar_consulta_local(char *consulta, int K, ResultadoBusqueda *resultados_locales, int *num_resultados) {
    (void)K;  // Evitar warning de parámetro no usado
    
    char *palabras_consulta[100];  // Array de punteros a palabras de la consulta
    int num_palabras_consulta = 0;  // Contador de palabras
    
    // Crear copia de la consulta (strtok modifica la cadena original)
    char consulta_copia[MAX_QUERY_LENGTH];
    strcpy(consulta_copia, consulta);
    
    // Tokenizar (dividir) la consulta en palabras
    char *token = strtok(consulta_copia, " \t\n");  // Usar espacios, tabs y newlines como delimitadores
    while (token != NULL && num_palabras_consulta < 100) {
        char *palabra_limpia = limpiar_palabra(token);  // Limpiar cada palabra
        if (strlen(palabra_limpia) > 0) {  // Si no está vacía
            // Reservar memoria y copiar la palabra
            palabras_consulta[num_palabras_consulta] = malloc(strlen(palabra_limpia) + 1);
            strcpy(palabras_consulta[num_palabras_consulta], palabra_limpia);
            num_palabras_consulta++;  // Incrementar contador
        }
        token = strtok(NULL, " \t\n");  // Obtener siguiente palabra
    }
    
    // Si no hay palabras válidas, retornar sin resultados
    if (num_palabras_consulta == 0) {
        *num_resultados = 0;
        return;
    }
    
    // Obtener los IDs de las palabras que existen en el vocabulario local
    int palabra_ids[100];    // Array para almacenar IDs
    int palabras_validas = 0;  // Contador de palabras encontradas
    
    for (int i = 0; i < num_palabras_consulta; i++) {
        int palabra_id = buscar_palabra_id(palabras_consulta[i]);  // Buscar palabra
        if (palabra_id >= 0) {  // Si existe en el vocabulario local
            palabra_ids[palabras_validas++] = palabra_id;  // Guardar su ID
        }
    }
    
    // Si ninguna palabra está en el vocabulario local, retornar sin resultados
    if (palabras_validas == 0) {
        *num_resultados = 0;
        // Liberar memoria de las palabras
        for (int i = 0; i < num_palabras_consulta; i++) {
            free(palabras_consulta[i]);
        }
        return;
    }
    
    // Inicializar array de rankings para cada documento local
    double rankings[MAX_DOCS];
    for (int i = 0; i < num_documentos; i++) {
        rankings[i] = 0.0;  // Inicializar todos en 0
    }
    
    // Para cada palabra de la consulta, calcular su contribución al ranking
    for (int i = 0; i < palabras_validas; i++) {
        int palabra_id = palabra_ids[i];  // Obtener ID de la palabra
        
        // Buscar la lista invertida de esta palabra en el índice local
        for (int j = 0; j < num_listas; j++) {
            if (listas_invertidas[j].palabra_id == palabra_id) {  // Si encontramos la lista
                // Para cada documento en la lista invertida
                for (int k = 0; k < listas_invertidas[j].num_docs; k++) {
                    int doc_id = listas_invertidas[j].documentos[k].doc_id;  // ID del documento
                    double frec_norm = listas_invertidas[j].documentos[k].frecuencia_norm;  // Frecuencia normalizada
                    
                    // Calcular peso TF-IDF y sumarlo al ranking
                    double w = calcular_w(palabra_id, doc_id, frec_norm);
                    rankings[doc_id] += w;  // Acumular peso
                }
                break;  // Salir del bucle, ya encontramos la lista
            }
        }
    }
    
    // Recopilar todos los documentos con ranking > 0 (documentos relevantes)
    *num_resultados = 0;
    for (int i = 0; i < num_documentos; i++) {
        if (rankings[i] > 0.0) {  // Si el documento es relevante
            resultados_locales[*num_resultados].doc_id = i;  // Guardar ID
            resultados_locales[*num_resultados].ranking = rankings[i];  // Guardar ranking
            strcpy(resultados_locales[*num_resultados].url, documentos[i].url);  // Guardar URL
            (*num_resultados)++;  // Incrementar contador
        }
    }
    
    // Ordenar los resultados locales por ranking (de mayor a menor)
    qsort(resultados_locales, *num_resultados, sizeof(ResultadoBusqueda), comparar_resultados);
    
    // Liberar memoria de las palabras de la consulta
    for (int i = 0; i < num_palabras_consulta; i++) {
        free(palabras_consulta[i]);
    }
}

// Función para leer consultas desde un archivo de texto
// Cada línea del archivo es una consulta
void leer_consultas(const char *archivo_entrada, char consultas[][MAX_QUERY_LENGTH], int *num_consultas, int Q) {
    (void)Q;  // Evitar warning de parámetro no usado
    
    FILE *f = fopen(archivo_entrada, "r");  // Abrir archivo en modo lectura
    if (!f) {  // Si no se pudo abrir
        *num_consultas = 0;  // No hay consultas
        return;
    }
    
    *num_consultas = 0;  // Inicializar contador
    // Leer el archivo línea por línea
    while (fgets(consultas[*num_consultas], MAX_QUERY_LENGTH, f)) {
        // Eliminar salto de línea al final si existe
        int len = strlen(consultas[*num_consultas]);
        if (len > 0 && consultas[*num_consultas][len-1] == '\n') {
            consultas[*num_consultas][len-1] = '\0';
        }
        
        // Solo contar líneas no vacías
        if (strlen(consultas[*num_consultas]) > 0) {
            (*num_consultas)++;  // Incrementar contador
        }
    }
    
    fclose(f);  // Cerrar archivo
}

// Función para escribir los resultados de una consulta en un archivo
void escribir_resultados(const char *archivo_salida, int rank, int consulta_id, 
                        ResultadoBusqueda *resultados, int num_resultados) {
    (void)rank;  // Evitar warning de parámetro no usado
    
    FILE *f = fopen(archivo_salida, "a");  // Abrir en modo append (agregar al final)
    if (!f) return;  // Si no se pudo abrir, salir
    
    // Escribir encabezado de la consulta
    fprintf(f, "Consulta %d:\n", consulta_id + 1);  // ID de consulta (empezando en 1)
    
    // Verificar si hay resultados
    if (num_resultados == 0) {
        fprintf(f, "  No se encontraron resultados\n");
    } else {
        // Escribir cada resultado
        for (int i = 0; i < num_resultados; i++) {
            fprintf(f, "  %d. (doc=%d, ranking=%.3f) %s\n", 
                   i+1,                          // Número de resultado (empezando en 1)
                   resultados[i].doc_id + 1,     // ID del documento (empezando en 1)
                   resultados[i].ranking,        // Puntuación de relevancia
                   resultados[i].url);           // URL del documento
        }
    }
    fprintf(f, "\n");  // Línea en blanco para separar consultas
    
    fclose(f);  // Cerrar archivo
}

// Función para liberar toda la memoria dinámica asignada
void liberar_memoria() {
    // Recorrer todas las listas invertidas
    for (int i = 0; i < num_listas; i++) {
        // Liberar el array de documentos de cada lista invertida
        free(listas_invertidas[i].documentos);
    }
}
