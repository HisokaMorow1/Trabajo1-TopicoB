// Librería para operaciones de entrada/salida estándar
#include <stdio.h>
// Librería para gestión de memoria dinámica y conversiones
#include <stdlib.h>
// Librería para manipulación de cadenas de caracteres
#include <string.h>
// Librería para funciones de clasificación de caracteres
#include <ctype.h>
// Librería para manipulación de directorios
#include <dirent.h>
// Librería para obtener información de archivos
#include <sys/stat.h>

// Define la longitud máxima permitida para una palabra (100 caracteres)
#define MAX_WORD_LENGTH 100
// Define el número máximo de palabras por línea en el texto filtrado (12)
#define MAX_WORDS_PER_LINE 12
// Define la longitud máxima para rutas de archivos (512 caracteres)
#define MAX_PATH_LENGTH 512
// Define la longitud máxima de una línea de texto (1024 caracteres)
#define MAX_LINE_LENGTH 1024
// Define la longitud máxima del texto procesado (50,000 caracteres)
#define MAX_TEXT_LENGTH 50000

// Estructura para almacenar el diccionario de palabras válidas
typedef struct {
    char **palabras;  // Arreglo dinámico de punteros a palabras
    int count;        // Número de palabras cargadas en el diccionario
    int capacity;     // Capacidad máxima del arreglo
} Diccionario;

// Estructura para información del documento procesado
typedef struct {
    int ano;                                  // Año de publicación del documento
    int mes;                                  // Mes de publicación del documento
    int dia;                                  // Día de publicación del documento
    int documento_id;                         // ID único del documento
    char url_relativa[MAX_PATH_LENGTH];       // Ruta relativa del archivo HTML
    char texto_filtrado[MAX_TEXT_LENGTH];     // Texto filtrado con palabras del diccionario
} DocumentoInfo;

// Prototipos de funciones
Diccionario* cargar_diccionario(const char *archivo_dict);  // Carga el diccionario desde archivo
int buscar_palabra_en_dict(Diccionario *dict, const char *palabra);  // Busca palabra usando búsqueda binaria
void liberar_diccionario(Diccionario *dict);  // Libera memoria del diccionario
char* extraer_contenido_html(const char *contenido_html);  // Extrae texto de tags HTML específicos
char* filtrar_texto_con_diccionario(const char *texto, Diccionario *dict);  // Filtra texto con palabras del diccionario
void procesar_archivo_html(const char *ruta_archivo, Diccionario *dict, FILE *salida, int *contador_doc);  // Procesa un archivo HTML
void recorrer_directorio_html(const char *ruta_base, Diccionario *dict, FILE *salida, int *contador_doc);  // Recorre directorios recursivamente
int extraer_fecha_de_ruta(const char *ruta, int *ano, int *mes, int *dia);  // Extrae fecha de la ruta del archivo
char* limpiar_palabra(const char *palabra);  // Limpia una palabra dejando solo letras en minúscula

// Función principal del programa
// Parámetros: argc - número de argumentos, argv - arreglo de argumentos
int main(int argc, char *argv[]) {
    if (argc != 3) {  // Verifica que se proporcionen exactamente 2 argumentos
        printf("Uso: %s <directorio_html> <archivo_dict>\n", argv[0]);  // Muestra uso correcto
        printf("Ejemplo: %s tarea1 txt/dict.txt\n", argv[0]);  // Muestra ejemplo
        return 1;  // Retorna código de error
    }
    
    const char *directorio_html = argv[1];  // Primer argumento: directorio con archivos HTML
    const char *archivo_dict = argv[2];     // Segundo argumento: archivo del diccionario
    
    printf("=== GENERADOR DE BASE_TEXTO.TXT ===\n");  // Título del programa
    printf("Directorio HTML: %s\n", directorio_html);  // Muestra directorio a procesar
    printf("Diccionario: %s\n", archivo_dict);         // Muestra archivo de diccionario
    printf("Archivo salida: txt/base_texto.txt\n\n"); // Muestra archivo de salida
    
    // Carga el diccionario de palabras válidas desde el archivo
    printf("1. Cargando diccionario...\n");
    Diccionario *dict = cargar_diccionario(archivo_dict);
    if (dict == NULL) {  // Si falla la carga
        return 1;  // Termina el programa con error
    }
    
    // Abre el archivo de salida en modo escritura
    FILE *salida = fopen("txt/base_texto.txt", "w");
    if (salida == NULL) {  // Si no se pudo crear el archivo
        printf("Error: No se pudo crear txt/base_texto.txt\n");  // Muestra error
        liberar_diccionario(dict);  // Libera memoria del diccionario
        return 1;  // Termina con error
    }
    
    // Procesa todos los archivos HTML del directorio
    printf("\n2. Procesando archivos HTML...\n");
    int contador_doc = 0;  // Inicializa contador de documentos procesados
    recorrer_directorio_html(directorio_html, dict, salida, &contador_doc);
    
    // Cierra el archivo de salida y libera memoria
    fclose(salida);  // Cierra el archivo
    liberar_diccionario(dict);  // Libera toda la memoria del diccionario
    
    printf("\n=== PROCESO COMPLETADO ===\n");  // Mensaje de finalización
    printf("Documentos procesados: %d\n", contador_doc);  // Muestra total de documentos
    printf("Archivo generado: txt/base_texto.txt\n");     // Confirma archivo creado
    
    return 0;  // Retorna éxito
}

// Función para cargar el diccionario desde un archivo
// Parámetro: archivo_dict - ruta del archivo con el diccionario
// Retorna: puntero al diccionario cargado, o NULL si hay error
Diccionario* cargar_diccionario(const char *archivo_dict) {
    FILE *archivo = fopen(archivo_dict, "r");  // Abre el archivo en modo lectura
    if (archivo == NULL) {  // Si no se pudo abrir
        printf("Error: No se pudo abrir %s\n", archivo_dict);  // Muestra error
        return NULL;  // Retorna NULL para indicar fallo
    }
    
    Diccionario *dict = malloc(sizeof(Diccionario));  // Reserva memoria para la estructura
    dict->capacity = 150000;  // Establece capacidad para 150,000 palabras (maneja 117k)
    dict->count = 0;  // Inicializa contador en 0
    dict->palabras = malloc(dict->capacity * sizeof(char*));  // Reserva memoria para los punteros
    
    char palabra[MAX_WORD_LENGTH];  // Buffer temporal para cada palabra
    while (fgets(palabra, sizeof(palabra), archivo)) {  // Lee línea por línea
        // Elimina el salto de línea al final
        int len = strlen(palabra);  // Obtiene longitud de la palabra
        if (len > 0 && palabra[len-1] == '\n') {  // Si termina con salto de línea
            palabra[len-1] = '\0';  // Reemplaza '\n' con terminador nulo
        }
        
        if (strlen(palabra) > 0) {  // Si la palabra no está vacía
            dict->palabras[dict->count] = malloc(strlen(palabra) + 1);  // Reserva memoria para la palabra
            strcpy(dict->palabras[dict->count], palabra);  // Copia la palabra a la memoria
            dict->count++;  // Incrementa el contador
        }
    }
    
    fclose(archivo);  // Cierra el archivo
    printf("Diccionario cargado: %d palabras\n", dict->count);  // Muestra cantidad de palabras cargadas
    return dict;  // Retorna el diccionario cargado
}

// Función de búsqueda binaria en el diccionario (asume que está ordenado alfabéticamente)
// Parámetros: dict - diccionario ordenado, palabra - palabra a buscar
// Retorna: 1 si encuentra la palabra, 0 si no la encuentra
int buscar_palabra_en_dict(Diccionario *dict, const char *palabra) {
    int izquierda = 0;  // Límite izquierdo del rango de búsqueda
    int derecha = dict->count - 1;  // Límite derecho del rango de búsqueda
    
    while (izquierda <= derecha) {  // Mientras el rango sea válido
        int medio = izquierda + (derecha - izquierda) / 2;  // Calcula posición media
        int comparacion = strcmp(dict->palabras[medio], palabra);  // Compara palabra del medio con la buscada
        
        if (comparacion == 0) {  // Si son iguales
            return 1;  // Palabra encontrada
        } else if (comparacion < 0) {  // Si palabra del medio es menor
            izquierda = medio + 1;  // Busca en mitad derecha
        } else {  // Si palabra del medio es mayor
            derecha = medio - 1;  // Busca en mitad izquierda
        }
    }
    
    return 0;  // Palabra no encontrada
}

// Función para liberar memoria del diccionario
// Parámetro: dict - puntero al diccionario a liberar
void liberar_diccionario(Diccionario *dict) {
    for (int i = 0; i < dict->count; i++) {  // Recorre todas las palabras
        free(dict->palabras[i]);  // Libera memoria de cada palabra individual
    }
    free(dict->palabras);  // Libera el arreglo de punteros
    free(dict);  // Libera la estructura del diccionario
}

// Función para extraer contenido de tags HTML específicos
// Parámetro: contenido_html - contenido completo del archivo HTML
// Retorna: cadena con el texto extraído de los tags
char* extraer_contenido_html(const char *contenido_html) {
    char *resultado = malloc(MAX_TEXT_LENGTH);  // Reserva memoria para el resultado
    resultado[0] = '\0';  // Inicializa como cadena vacía
    
    // Define los tags HTML que se van a procesar para extraer texto
    const char *tags[] = {"<title>", "<h1>", "<h2>", "<h3>", "<p>", "<div>", "<span>"};
    const char *tags_cierre[] = {"</title>", "</h1>", "</h2>", "</h3>", "</p>", "</div>", "</span>"};
    int num_tags = 7;  // Número total de tags a procesar
    
    for (int t = 0; t < num_tags; t++) {  // Recorre cada tipo de tag
        const char *inicio = contenido_html;  // Comienza desde el inicio del HTML
        char *pos_inicio, *pos_fin;  // Punteros para marcar inicio y fin del contenido
        
        while ((pos_inicio = strstr(inicio, tags[t])) != NULL) {  // Busca el tag de apertura
            pos_inicio += strlen(tags[t]);  // Avanza al contenido (después del tag de apertura)
            pos_fin = strstr(pos_inicio, tags_cierre[t]);  // Busca el tag de cierre
            
            if (pos_fin != NULL) {  // Si encontró el tag de cierre
                // Calcula la longitud del contenido entre los tags
                int longitud = pos_fin - pos_inicio;
                char *fragmento = malloc(longitud + 1);  // Reserva memoria para el fragmento
                strncpy(fragmento, pos_inicio, longitud);  // Copia el contenido
                fragmento[longitud] = '\0';  // Agrega terminador nulo
                
                // Agrega el fragmento al resultado si hay espacio
                if (strlen(resultado) + strlen(fragmento) + 2 < MAX_TEXT_LENGTH) {
                    strcat(resultado, fragmento);  // Concatena el fragmento
                    strcat(resultado, " ");  // Agrega un espacio separador
                }
                
                free(fragmento);  // Libera el fragmento temporal
                inicio = pos_fin + strlen(tags_cierre[t]);  // Continúa después del tag de cierre
            } else {
                break;  // No hay más tags de este tipo
            }
        }
    }
    
    return resultado;  // Retorna el texto extraído
}

// Función para convertir entidades HTML a caracteres normales
// Parámetro: texto - cadena de texto a convertir (se modifica directamente)
void convertir_acentos_html(char *texto) {
    // Arreglo con las entidades HTML que se van a reemplazar
    char *entidades[] = {"&aacute;", "&eacute;", "&iacute;", "&oacute;", "&uacute;",
                         "&Aacute;", "&Eacute;", "&Iacute;", "&Oacute;", "&Uacute;",
                         "&ntilde;", "&Ntilde;", "&amp;", "&lt;", "&gt;", "&quot;", 
                         "&nbsp;", "&#243;", "&#233;", "&#237;", "&#225;", "&#250;"};
    // Arreglo con los caracteres de reemplazo correspondientes
    char *reemplazos[] = {"a", "e", "i", "o", "u", "A", "E", "I", "O", "U", 
                          "n", "N", "&", "<", ">", "\"", " ", "o", "e", "i", "a", "u"};
    int num_entidades = 22;  // Número total de entidades a procesar
    
    for (int i = 0; i < num_entidades; i++) {  // Recorre cada entidad HTML
        char *pos = strstr(texto, entidades[i]);  // Busca la primera ocurrencia de la entidad
        while (pos != NULL) {  // Mientras encuentre ocurrencias
            int len_entidad = strlen(entidades[i]);  // Obtiene longitud de la entidad HTML
            int len_reemplazo = strlen(reemplazos[i]);  // Obtiene longitud del reemplazo
            
            // Mueve el resto del texto para hacer espacio o reducir espacio
            memmove(pos + len_reemplazo, pos + len_entidad, strlen(pos + len_entidad) + 1);
            // Copia el carácter de reemplazo en la posición actual
            memcpy(pos, reemplazos[i], len_reemplazo);
            
            pos = strstr(pos + len_reemplazo, entidades[i]);  // Busca la siguiente ocurrencia
        }
    }
}

// Función para limpiar una palabra dejando solo letras en minúscula
// Parámetro: palabra - palabra a limpiar
// Retorna: nueva cadena con solo letras en minúscula
char* limpiar_palabra(const char *palabra) {
    char *limpia = malloc(strlen(palabra) + 1);  // Reserva memoria para la palabra limpia
    int j = 0;  // Índice para construir la palabra limpia
    
    for (int i = 0; palabra[i] != '\0'; i++) {  // Recorre cada carácter de la palabra
        if (isalpha(palabra[i])) {  // Si es una letra
            limpia[j++] = tolower(palabra[i]);  // Convierte a minúscula y guarda
        }
    }
    limpia[j] = '\0';  // Agrega terminador nulo
    
    return limpia;  // Retorna la palabra limpia
}

// Función para filtrar texto conservando solo palabras del diccionario
// Parámetros: texto - texto a filtrar, dict - diccionario de palabras válidas
// Retorna: cadena con solo las palabras que están en el diccionario (máx 12 por línea)
char* filtrar_texto_con_diccionario(const char *texto, Diccionario *dict) {
    char *resultado = malloc(MAX_TEXT_LENGTH);  // Reserva memoria para el resultado
    resultado[0] = '\0';  // Inicializa como cadena vacía
    
    char *texto_copia = malloc(strlen(texto) + 1);  // Crea copia del texto
    strcpy(texto_copia, texto);  // Copia el texto original
    
    // Convierte todas las entidades HTML a caracteres normales
    convertir_acentos_html(texto_copia);
    
    // Tokeniza el texto usando espacios y signos de puntuación como delimitadores
    char *token = strtok(texto_copia, " \t\n\r\f.,;:!?()[]{}\"'<>");
    int palabras_en_linea = 0;  // Contador de palabras en la línea actual
    
    while (token != NULL) {  // Mientras haya tokens
        char *palabra_limpia = limpiar_palabra(token);  // Limpia la palabra
        
        // Si la palabra tiene más de 1 letra y está en el diccionario
        if (strlen(palabra_limpia) > 1 && buscar_palabra_en_dict(dict, palabra_limpia)) {
            // Si ya hay 12 palabras en la línea, crea una nueva línea
            if (palabras_en_linea >= MAX_WORDS_PER_LINE) {
                strcat(resultado, "\n");  // Agrega salto de línea
                palabras_en_linea = 0;  // Reinicia contador
            }
            
            if (palabras_en_linea > 0) {  // Si no es la primera palabra de la línea
                strcat(resultado, " ");  // Agrega espacio separador
            }
            strcat(resultado, palabra_limpia);  // Agrega la palabra al resultado
            palabras_en_linea++;  // Incrementa contador de palabras
        }
        
        free(palabra_limpia);  // Libera la palabra limpia
        token = strtok(NULL, " \t\n\r\f.,;:!?()[]{}\"'<>");  // Obtiene siguiente token
    }
    
    free(texto_copia);  // Libera la copia del texto
    return resultado;  // Retorna el texto filtrado
}

// Función para extraer fecha de la ruta del archivo (formato: .../AAAA/MM/DD/archivo.html)
// Parámetros: ruta - ruta del archivo, ano/mes/dia - punteros donde guardar la fecha
// Retorna: 1 si extrajo la fecha exitosamente, 0 si hubo error
int extraer_fecha_de_ruta(const char *ruta, int *ano, int *mes, int *dia) {
    // Crea una copia de la ruta para tokenizarla sin modificar la original
    char *ruta_copia = malloc(strlen(ruta) + 1);
    strcpy(ruta_copia, ruta);  // Copia la ruta
    
    char *token;  // Puntero para cada parte de la ruta
    char *partes[10];  // Arreglo para almacenar las partes de la ruta
    int num_partes = 0;  // Contador de partes
    
    // Tokeniza la ruta usando / y \ como separadores
    token = strtok(ruta_copia, "/\\");
    while (token != NULL && num_partes < 10) {  // Mientras haya tokens y espacio
        partes[num_partes++] = token;  // Guarda el token
        token = strtok(NULL, "/\\");  // Obtiene siguiente token
    }
    
    // Busca el patrón año (4 dígitos), mes y día en las partes de la ruta
    for (int i = 0; i < num_partes - 2; i++) {  // Recorre las partes (dejando espacio para mes y día)
        if (strlen(partes[i]) == 4 && isdigit(partes[i][0])) {  // Si tiene 4 dígitos (año)
            *ano = atoi(partes[i]);  // Convierte a entero y guarda el año
            if (i + 1 < num_partes && strlen(partes[i+1]) <= 2) {  // Si hay siguiente parte (mes)
                *mes = atoi(partes[i+1]);  // Convierte y guarda el mes
                if (i + 2 < num_partes && strlen(partes[i+2]) <= 2) {  // Si hay siguiente parte (día)
                    *dia = atoi(partes[i+2]);  // Convierte y guarda el día
                    free(ruta_copia);  // Libera la copia de la ruta
                    return 1;  // Retorna éxito
                }
            }
        }
    }
    
    free(ruta_copia);  // Libera la copia de la ruta
    return 0;  // Retorna error (no se encontró fecha válida)
}

// Función para procesar un archivo HTML individual
// Parámetros: ruta_archivo - ruta del HTML, dict - diccionario, salida - archivo de salida, contador_doc - contador de documentos
void procesar_archivo_html(const char *ruta_archivo, Diccionario *dict, FILE *salida, int *contador_doc) {
    FILE *archivo = fopen(ruta_archivo, "r");  // Abre el archivo en modo lectura
    if (archivo == NULL) {  // Si no se pudo abrir
        printf("Advertencia: No se pudo abrir %s\n", ruta_archivo);  // Muestra advertencia
        return;  // Sale de la función
    }
    
    // Determina el tamaño del archivo
    fseek(archivo, 0, SEEK_END);  // Va al final del archivo
    long tamano = ftell(archivo);  // Obtiene la posición (tamaño)
    fseek(archivo, 0, SEEK_SET);  // Regresa al inicio
    
    // Lee todo el contenido del archivo en memoria
    char *contenido = malloc(tamano + 1);  // Reserva memoria para el contenido
    size_t bytes_leidos = fread(contenido, 1, tamano, archivo);  // Lee todo el archivo
    contenido[bytes_leidos] = '\0';  // Agrega terminador nulo
    fclose(archivo);  // Cierra el archivo
    
    // Extrae la fecha (año/mes/día) de la ruta del archivo
    int ano, mes, dia;
    if (!extraer_fecha_de_ruta(ruta_archivo, &ano, &mes, &dia)) {  // Si falla la extracción
        printf("Advertencia: No se pudo extraer fecha de %s\n", ruta_archivo);  // Muestra advertencia
        free(contenido);  // Libera memoria
        return;  // Sale de la función
    }
    
    // Extrae el texto de los tags HTML relevantes
    char *texto_html = extraer_contenido_html(contenido);
    
    // Filtra el texto conservando solo palabras del diccionario
    char *texto_filtrado = filtrar_texto_con_diccionario(texto_html, dict);
    
    // Solo procesa el documento si tiene texto filtrado
    if (strlen(texto_filtrado) > 0) {  // Si hay contenido
        (*contador_doc)++;  // Incrementa el contador de documentos
        
        // Genera la URL relativa del archivo
        char url_relativa[MAX_PATH_LENGTH];
        char *inicio_relativo = strstr(ruta_archivo, "tarea1");  // Busca "tarea1" en la ruta
        if (inicio_relativo != NULL) {  // Si lo encuentra
            snprintf(url_relativa, sizeof(url_relativa), "./%s", inicio_relativo);  // Crea ruta relativa
        } else {
            strcpy(url_relativa, ruta_archivo);  // Usa ruta completa
        }
        
        // Escribe el documento en el formato requerido
        fprintf(salida, "<EDICION [%d]>\n", ano);  // Escribe el año
        fprintf(salida, "<MES [%d]>\n", mes);  // Escribe el mes
        fprintf(salida, "<DIA [%d]>\n", dia);  // Escribe el día
        fprintf(salida, "<DOCUMENTO [%d]>\n", *contador_doc);  // Escribe el ID del documento
        fprintf(salida, "<URL %s>\n", url_relativa);  // Escribe la URL
        fprintf(salida, "<TEXTO>\n");  // Marca inicio del texto
        fprintf(salida, "%s\n", texto_filtrado);  // Escribe el texto filtrado
        fprintf(salida, "</TEXTO>\n\n");  // Marca fin del texto
        
        printf("Procesado [%d]: %s (%d/%d/%d)\n", *contador_doc, url_relativa, ano, mes, dia);  // Mensaje de progreso
    }
    
    // Libera toda la memoria utilizada
    free(contenido);
    free(texto_html);
    free(texto_filtrado);
}

// Función para recorrer directorios recursivamente buscando archivos HTML
// Parámetros: ruta_base - directorio a recorrer, dict - diccionario, salida - archivo de salida, contador_doc - contador de documentos
void recorrer_directorio_html(const char *ruta_base, Diccionario *dict, FILE *salida, int *contador_doc) {
    DIR *dir = opendir(ruta_base);  // Abre el directorio
    if (dir == NULL) {  // Si no se pudo abrir
        printf("No se pudo abrir directorio: %s\n", ruta_base);  // Muestra error
        return;  // Sale de la función
    }
    
    struct dirent *entrada;  // Estructura para cada entrada del directorio
    while ((entrada = readdir(dir)) != NULL) {  // Lee cada entrada del directorio
        // Salta los directorios especiales "." (actual) y ".." (padre)
        if (strcmp(entrada->d_name, ".") == 0 || strcmp(entrada->d_name, "..") == 0) {
            continue;  // Salta a la siguiente entrada
        }
        
        // Construye la ruta completa del archivo o subdirectorio
        char ruta_completa[MAX_PATH_LENGTH];
        snprintf(ruta_completa, sizeof(ruta_completa), "%s/%s", ruta_base, entrada->d_name);
        
        struct stat info;  // Estructura para información del archivo
        if (stat(ruta_completa, &info) == 0) {  // Si se obtuvo información correctamente
            if (S_ISDIR(info.st_mode)) {  // Si es un directorio
                // Llama recursivamente para procesar el subdirectorio
                recorrer_directorio_html(ruta_completa, dict, salida, contador_doc);
            } else if (S_ISREG(info.st_mode)) {  // Si es un archivo regular
                // Busca la extensión del archivo (último punto en el nombre)
                char *extension = strrchr(entrada->d_name, '.');
                if (extension != NULL && strcmp(extension, ".html") == 0) {  // Si es archivo HTML
                    procesar_archivo_html(ruta_completa, dict, salida, contador_doc);  // Procesa el archivo
                }
            }
        }
    }
    
    closedir(dir);  // Cierra el directorio
}