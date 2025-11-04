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
// Librería para manejo de códigos de error
#include <errno.h>

// Define la longitud máxima permitida para una palabra (100 caracteres)
#define MAX_WORD_LENGTH 100
// Define el número máximo de palabras que se pueden almacenar (100,000)
#define MAX_WORDS 100000
// Define la longitud máxima para rutas de archivos (512 caracteres)
#define MAX_PATH_LENGTH 512

// Estructura para almacenar palabras únicas
typedef struct {
    char **palabras;    // Arreglo dinámico de punteros a cadenas (palabras)
    int count;          // Contador de palabras actualmente almacenadas
    int capacity;       // Capacidad actual del arreglo
} DiccionarioPalabras;

// Prototipos de funciones
DiccionarioPalabras* inicializar_diccionario();  // Crea e inicializa un nuevo diccionario
void expandir_diccionario(DiccionarioPalabras *dict);  // Duplica la capacidad del diccionario
int palabra_existe(DiccionarioPalabras *dict, const char *palabra);  // Verifica si una palabra existe
void agregar_palabra(DiccionarioPalabras *dict, const char *palabra);  // Agrega una palabra única
void extraer_palabras_texto(const char *texto, DiccionarioPalabras *dict);  // Extrae palabras de texto
void procesar_archivo_html(const char *ruta_archivo, DiccionarioPalabras *dict);  // Procesa un archivo HTML
void recorrer_directorio(const char *ruta_base, DiccionarioPalabras *dict);  // Recorre directorios recursivamente
void guardar_palabras(DiccionarioPalabras *dict, const char *nombre_archivo);  // Guarda palabras en archivo
void liberar_diccionario(DiccionarioPalabras *dict);  // Libera la memoria del diccionario

// Función principal del programa
// Parámetros: argc - número de argumentos, argv - arreglo de argumentos
int main(int argc, char *argv[]) {
    char ruta_base[MAX_PATH_LENGTH];  // Buffer para almacenar la ruta base
    
    // Verifica que se proporcionó exactamente un argumento (el directorio)
    if (argc != 2) {
        printf("Uso: %s <directorio_base>\n", argv[0]);  // Muestra el uso correcto
        printf("Ejemplo: %s /home/diario\n", argv[0]);  // Muestra un ejemplo
        return 1;  // Retorna código de error
    }
    
    strcpy(ruta_base, argv[1]);  // Copia el argumento (directorio) a ruta_base
    
    printf("Iniciando extracción de palabras desde: %s\n", ruta_base);  // Mensaje informativo
    
    // Crea e inicializa el diccionario para almacenar palabras únicas
    DiccionarioPalabras *diccionario = inicializar_diccionario();
    
    // Recorre recursivamente todos los archivos HTML en el directorio
    recorrer_directorio(ruta_base, diccionario);
    
    // Guarda todas las palabras únicas encontradas en el archivo de salida
    guardar_palabras(diccionario, "txt/base_html.txt");
    
    // Libera toda la memoria dinámica utilizada por el diccionario
    liberar_diccionario(diccionario);
    
    printf("Proceso completado exitosamente.\n");  // Mensaje de finalización
    return 0;  // Retorna éxito
}

// Función para inicializar el diccionario
// Retorna: puntero al nuevo diccionario creado
DiccionarioPalabras* inicializar_diccionario() {
    DiccionarioPalabras *dict = malloc(sizeof(DiccionarioPalabras));  // Reserva memoria para la estructura
    dict->capacity = 1000;  // Establece capacidad inicial de 1000 palabras
    dict->count = 0;  // Inicializa el contador de palabras en 0
    dict->palabras = malloc(dict->capacity * sizeof(char*));  // Reserva memoria para 1000 punteros
    return dict;  // Retorna el puntero al diccionario creado
}

// Función para expandir el diccionario si es necesario
// Parámetro: dict - puntero al diccionario a expandir
void expandir_diccionario(DiccionarioPalabras *dict) {
    if (dict->count >= dict->capacity) {  // Si el contador alcanzó la capacidad
        dict->capacity *= 2;  // Duplica la capacidad del diccionario
        dict->palabras = realloc(dict->palabras, dict->capacity * sizeof(char*));  // Reasigna memoria al doble
    }
}

// Función para verificar si una palabra ya existe
// Parámetros: dict - diccionario, palabra - palabra a buscar
// Retorna: 1 si existe, 0 si no existe
int palabra_existe(DiccionarioPalabras *dict, const char *palabra) {
    for (int i = 0; i < dict->count; i++) {  // Recorre todas las palabras del diccionario
        if (strcmp(dict->palabras[i], palabra) == 0) {  // Compara cada palabra con la buscada
            return 1;  // Retorna 1 si encuentra coincidencia
        }
    }
    return 0;  // Retorna 0 si no encuentra la palabra
}

// Función para agregar palabra al diccionario
// Parámetros: dict - diccionario, palabra - palabra a agregar
void agregar_palabra(DiccionarioPalabras *dict, const char *palabra) {
    if (!palabra_existe(dict, palabra)) {  // Solo agrega si la palabra no existe
        expandir_diccionario(dict);  // Expande el diccionario si es necesario
        dict->palabras[dict->count] = malloc(strlen(palabra) + 1);  // Reserva memoria para la palabra
        strcpy(dict->palabras[dict->count], palabra);  // Copia la palabra a la memoria reservada
        dict->count++;  // Incrementa el contador de palabras
    }
}

// Lista de palabras HTML que NO son contenido (tags y atributos a filtrar)
const char *palabras_html[] = {
    // Tags HTML estándar
    "html", "head", "body", "div", "span", "p", "br", "hr", "img", "a", "link",
    "script", "style", "title", "meta", "form", "input", "button", "table", "tr",
    "td", "th", "thead", "tbody", "tfoot", "ul", "ol", "li", "h1", "h2", "h3",
    "h4", "h5", "h6", "h7", "h8", "h9", "h10", "font", "b", "i", "u", "strong", "em",
    "center", "left", "right", "justify", "middle", "bottom", "top",
    
    // Atributos comunes
    "align", "width", "height", "color", "size", "border", "cellpadding", "cellspacing",
    "bgcolor", "class", "id", "style", "href", "src", "alt", "name", "value", "type",
    "method", "action", "target", "onclick", "onload", "onmouseover", "onmouseout",
    "onchange", "onsubmit", "onkeydown", "onfocus", "onblur", "ondblclick", "onmousedown",
    "onmouseup", "colspan", "rowspan", "valign", "frame", "frameborder", "scrolling",
    "marginwidth", "marginheight", "content", "http", "equiv", "charset", "rel",
    "itemprop", "itemscope", "itemtype", "property", "attribute", "generator",
    "data", "naturalsizeflag", "background", "face", "arial", "helvetica",
    
    // Tags especiales
    "noscript", "object", "embed", "applet", "param", "blink", "marquee", "textarea",
    "select", "option", "label", "fieldset", "legend", "caption", "col", "colgroup",
    "iframe", "frameset", "frame", "noframe", "area", "base", "basefont", "isindex",
    "map", "pre", "tt", "code", "kbd", "samp", "var", "big", "small", "sub", "sup",
    "strike", "del", "ins", "cite", "dfn", "abbr", "acronym", "address", "blockquote",
    "q", "dl", "dt", "dd", "bdo", "wbr", "nobr", "comment", "xml", "version",
    "encoding", "standalone", "dtd", "doctype", "cdata", "section", "article", "nav",
    "aside", "header", "footer", "main", "figure", "figcaption", "time", "mark",
    "ruby", "rt", "rp", "bdi", "details", "summary", "dialog", "canvas", "svg",
    "video", "audio", "source", "track", "meter", "progress", "datalist", "keygen",
    "output", "li",
    
    // Palabras de navegación y sitio específico
    "navegacion", "portada", "volver", "inicio", "home", "menu", "submenu", "link",
    "sitio", "web", "internet", "email", "mailto", "ftp", "https",
    "barra", "regresar", "atras", "adelante", "siguiente", "anterior", "pagina",
    "seccion", "categoria", "tags", "noticias", "articulo", "blog", "post",
    "comentarios", "opiniones", "lectores", "usuario", "login", "logout", "registro",
    "buscar", "search", "generado", "automaticamente", "derechos",
    "reservados", "copyright", "consorcio", "periodistico", "copesa",
    "agencias", "epigrafe", "bajada", "autor",
    "ultimas", "informaciones", "especial", "detenido", "londres",
    "cgibin", "tnbanner", "destino", "openchile", "tnimage", "imagen", "publicidad",
    "sponsors", "icn", "lineaazul", "gif", "logo", "win",
    
    // Palabras técnicas, formatos y atributos HTML nuevas
    "quot", "amp", "lt", "gt", "nbsp", "aacute", "eacute", "iacute", "oacute",
    "uacute", "agrave", "egrave", "igrave", "ograve", "ugrave", "ntilde", "rsac",
    "pics", "rating", "rsaci", "north", "america", "server", "jccamus",
    "tnpress", "tecnonautica", "tnautica", "javascript", "history", "back",
    "genero", "rotativa", "texto", "automagicamente",
    "dddddd", "ffffff", "1a77b0", "1c2f72",
    "news", "latercera", "htm", "html", "color", "ab", "verdana", "ad", "terra",
    "abr", "www", "click", "aqui", "aqu", "superior", "extras",
    
    // Palabras de estructura de artículos
    "titulo", "epigrafe", "fin", "navegacion", "reacciones", "history",
    
    // Palabras cortas no significativas (stopwords en español)
    "el", "la", "de", "y", "o", "a", "en", "por", "para", "con", "sin", "es", "son",
    "ser", "al", "fue", "ha", "han", "haya", "hayan", "habria", "habrian", "hay",
    "que", "te", "ti", "tu", "tus", "mi", "mis", "su", "sus", "nos",
    "nosotros", "vosotros", "los", "las", "les", "le", "me", "os",
    "nuestro", "vuestro", "mio", "tuyo", "suyo", "eso", "este", "ese", "aquel",
    "esto", "esa", "esos", "esas", "aquello", "algo", "nada", "alguien", "nadie",
    "uno", "unos", "una", "unas", "dos", "tres", "cuatro", "cinco", "seis",
    "siete", "ocho", "nueve", "diez", "veinte", "treinta", "cuarenta",
    "no", "nos", "muy", "solo", "mas", "bien", "tal", "cierto", "otro",
    "alguno", "ninguno", "cada",
    
    // Preposiciones, conjunciones, conectores
    "donde", "como", "cuando", "porque", "cual", "cuales", "quien", "quienes",
    "sino", "pero", "pues", "luego", "entonces", "aunque", "si",
    "entre", "sobre", "bajo", "tras", "ante", "desde", "hasta", "durante", "mediante",
    "cabe", "cerca", "junto", "versus", "via", "segun",
    
    // Palabras comunes de baja relevancia
    "caso", "parte", "vez", "forma", "momento", "numero", "tipo", "nivel",
    "hecho", "punto", "modo", "grado", "sentido", "termino", "dato",
    
    // Palabras técnicas de valores numéricos en HTML
    "year", "date", "day", "month", "time", "cifra", "cantidad",
    "zero", "false", "true", "null", "undefined", "hex", "rgb", "url", "file",
    NULL
};

// Función para verificar si una palabra es un residuo HTML
// Parámetro: palabra - palabra a verificar
// Retorna: 1 si es residuo HTML, 0 si es contenido válido
int es_residuo_html(const char *palabra) {
    for (int i = 0; palabras_html[i] != NULL; i++) {
        if (strcmp(palabra, palabras_html[i]) == 0) {
            return 1;  // Es un residuo HTML
        }
    }
    return 0;  // Es contenido válido
}

// Función para limpiar el texto de tags HTML y sus atributos
// Parámetro: texto - cadena de texto a limpiar (se modifica directamente)
void limpiar_html(char *texto) {
    int i = 0;
    int j = 0;
    int en_tag = 0;
    
    // Primera pasada: eliminar TODOS los tags HTML, comentarios y su contenido
    while (texto[i] != '\0') {
        // Detectar comentarios HTML <!-- -->
        if (texto[i] == '<' && texto[i+1] == '!' && texto[i+2] == '-' && texto[i+3] == '-') {
            // Saltar el comentario completo: <!-- ... -->
            i += 4;  // Saltar <!--
            while (texto[i] != '\0') {
                if (texto[i] == '-' && texto[i+1] == '-' && texto[i+2] == '>') {
                    i += 3;  // Saltar -->
                    break;
                }
                i++;
            }
            // Agregar un espacio para separar palabras
            if (j > 0 && texto[j-1] != ' ' && texto[j-1] != '\n') {
                texto[j++] = ' ';
            }
        }
        // Detectar tags HTML normales < ... >
        else if (texto[i] == '<') {
            en_tag = 1;
            i++;
            // Saltarse todo hasta encontrar '>'
            while (texto[i] != '\0' && texto[i] != '>') {
                i++;
            }
            // Si encontramos '>', saltar también ese carácter
            if (texto[i] == '>') {
                i++;
            }
            en_tag = 0;
            // Agregar un espacio para separar palabras que estaban entre tags
            if (j > 0 && texto[j-1] != ' ' && texto[j-1] != '\n') {
                texto[j++] = ' ';
            }
        } else if (texto[i] == '\n' || texto[i] == '\r' || texto[i] == '\t') {
            // Convertir espacios en blanco a espacios normales
            if (j > 0 && texto[j-1] != ' ') {
                texto[j++] = ' ';
            }
            i++;
        } else {
            // Copiar caracteres normales
            texto[j++] = texto[i++];
        }
    }
    texto[j] = '\0';
    
    // Segunda pasada: limpiar espacios múltiples
    i = 0;
    j = 0;
    int espacios_seguidos = 0;
    
    while (texto[i] != '\0') {
        if (texto[i] == ' ') {
            espacios_seguidos++;
            if (espacios_seguidos == 1) {
                texto[j++] = ' ';  // Mantener solo un espacio
            }
        } else {
            espacios_seguidos = 0;
            texto[j++] = texto[i];
        }
        i++;
    }
    texto[j] = '\0';
}

// Función para convertir entidades HTML a caracteres normales
// Parámetro: texto - cadena de texto a convertir (se modifica directamente)
void convertir_acentos_html(char *texto) {
    // Arreglo con las entidades HTML que se van a reemplazar
    char *entidades[] = {"&aacute;", "&eacute;", "&iacute;", "&oacute;", "&uacute;",
                         "&Aacute;", "&Eacute;", "&Iacute;", "&Oacute;", "&Uacute;",
                         "&ntilde;", "&Ntilde;", "&amp;", "&lt;", "&gt;", "&quot;"};
    // Arreglo con los caracteres de reemplazo correspondientes
    char *reemplazos[] = {"a", "e", "i", "o", "u", "A", "E", "I", "O", "U", "n", "N", "&", "<", ">", "\""};
    int num_entidades = 16;  // Número total de entidades a procesar
    
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

// Función para extraer palabras de un texto
// Parámetros: texto - cadena de texto a procesar, dict - diccionario donde agregar palabras
void extraer_palabras_texto(const char *texto, DiccionarioPalabras *dict) {
    char *texto_copia = malloc(strlen(texto) + 1);  // Reserva memoria para copia del texto
    strcpy(texto_copia, texto);  // Copia el texto original
    
    // Limpia todos los tags HTML de una sola pasada
    limpiar_html(texto_copia);
    
    // Convierte todas las entidades HTML a caracteres normales
    convertir_acentos_html(texto_copia);
    
    char palabra[MAX_WORD_LENGTH];  // Buffer para construir cada palabra
    int pos_palabra = 0;  // Posición actual en el buffer de palabra
    int i = 0;  // Índice para recorrer el texto
    int palabra_tiene_letra = 0;  // Flag para verificar si la palabra tiene al menos una letra
    
    while (texto_copia[i] != '\0') {
        // Procesar caracteres
        if (isalpha(texto_copia[i])) {
            if (pos_palabra < MAX_WORD_LENGTH - 1) {
                palabra[pos_palabra] = tolower(texto_copia[i]);
                pos_palabra++;
                palabra_tiene_letra = 1;
            }
        } else {
            // Fin de palabra
            if (pos_palabra > 0 && palabra_tiene_letra) {
                palabra[pos_palabra] = '\0';
                // Solo agregar si: 
                // - tiene más de 2 caracteres
                // - NO es residuo HTML
                if (strlen(palabra) > 2 && !es_residuo_html(palabra)) {
                    agregar_palabra(dict, palabra);
                }
                pos_palabra = 0;
                palabra_tiene_letra = 0;
            }
        }
        i++;  // Avanza al siguiente carácter
    }
    
    // Procesa la última palabra si el texto no termina con separador
    if (pos_palabra > 0 && palabra_tiene_letra) {  // Si quedó una palabra sin procesar
        palabra[pos_palabra] = '\0';  // Termina la cadena
        // Solo agregar si: tiene más de 2 caracteres Y NO es residuo HTML
        if (strlen(palabra) > 2 && !es_residuo_html(palabra)) {
            agregar_palabra(dict, palabra);  // Agrega la palabra al diccionario
        }
    }
    
    free(texto_copia);  // Libera la memoria de la copia del texto
}

// Función para procesar un archivo HTML completo
// Parámetros: ruta_archivo - ruta del archivo HTML, dict - diccionario donde agregar palabras
void procesar_archivo_html(const char *ruta_archivo, DiccionarioPalabras *dict) {
    FILE *archivo = fopen(ruta_archivo, "r");  // Abre el archivo en modo lectura
    if (archivo == NULL) {  // Si no se pudo abrir
        printf("No se pudo abrir el archivo: %s\n", ruta_archivo);  // Muestra error
        return;  // Sale de la función
    }
    
    // Determina el tamaño total del archivo
    fseek(archivo, 0, SEEK_END);  // Va al final del archivo
    long tamano = ftell(archivo);  // Obtiene la posición (tamaño en bytes)
    fseek(archivo, 0, SEEK_SET);  // Regresa al inicio del archivo
    
    // Lee todo el contenido del archivo en memoria
    char *contenido = malloc(tamano + 1);  // Reserva memoria para el contenido (+1 para '\0')
    size_t bytes_leidos = fread(contenido, 1, tamano, archivo);  // Lee el archivo completo
    if (bytes_leidos != (size_t)tamano) {  // Si no se leyó todo
        printf("Advertencia: No se pudieron leer todos los bytes de %s\n", ruta_archivo);
    }
    contenido[bytes_leidos] = '\0';  // Agrega terminador nulo
    
    fclose(archivo);  // Cierra el archivo
    
    // Procesa el contenido para extraer todas las palabras
    extraer_palabras_texto(contenido, dict);
    
    free(contenido);  // Libera la memoria del contenido
    printf("Procesado: %s\n", ruta_archivo);  // Mensaje de confirmación
}

// Función para recorrer directorios recursivamente
// Parámetros: ruta_base - directorio a recorrer, dict - diccionario donde agregar palabras
void recorrer_directorio(const char *ruta_base, DiccionarioPalabras *dict) {
    DIR *dir;  // Puntero al directorio
    struct dirent *entrada;  // Estructura para cada entrada del directorio
    struct stat info;  // Estructura para información de archivos
    char ruta_completa[MAX_PATH_LENGTH];  // Buffer para construir rutas completas
    
    dir = opendir(ruta_base);  // Abre el directorio
    if (dir == NULL) {  // Si no se pudo abrir
        printf("No se pudo abrir el directorio: %s\n", ruta_base);  // Muestra error
        return;  // Sale de la función
    }
    
    while ((entrada = readdir(dir)) != NULL) {
        // Saltar . y ..
        if (strcmp(entrada->d_name, ".") == 0 || strcmp(entrada->d_name, "..") == 0) {
            continue;
        }
        
        // Construir ruta completa
        snprintf(ruta_completa, sizeof(ruta_completa), "%s/%s", ruta_base, entrada->d_name);
        
        // Verificar si es archivo o directorio
        if (stat(ruta_completa, &info) == 0) {
            if (S_ISDIR(info.st_mode)) {
                // Es un directorio, recorrer recursivamente
                recorrer_directorio(ruta_completa, dict);
            } else if (S_ISREG(info.st_mode)) {
                // Es un archivo regular, verificar si es HTML
                char *extension = strrchr(entrada->d_name, '.');
                if (extension != NULL && 
                    (strcmp(extension, ".html") == 0 || strcmp(extension, ".htm") == 0)) {
                    procesar_archivo_html(ruta_completa, dict);
                }
            }
        }
    }
    
    closedir(dir);
}

// Función de comparación para qsort (ordenamiento alfabético)
// Parámetros: a, b - punteros a los elementos a comparar
// Retorna: <0 si a<b, 0 si a==b, >0 si a>b
int comparar_palabras(const void *a, const void *b) {
    return strcmp(*(const char**)a, *(const char**)b);  // Compara las cadenas apuntadas
}

// Función para guardar palabras en archivo
// Parámetros: dict - diccionario con las palabras, nombre_archivo - ruta del archivo de salida
void guardar_palabras(DiccionarioPalabras *dict, const char *nombre_archivo) {
    // Intenta crear el directorio txt/ si no existe
    struct stat st = {0};  // Estructura para verificar existencia
    if (stat("txt", &st) == -1) {  // Si el directorio no existe
        if (mkdir("txt", 0755) != 0) {  // Intenta crear con permisos 755
            printf("Advertencia: No se pudo crear el directorio txt/\n");
        }
    }
    
    // Ordena todas las palabras alfabéticamente usando quicksort
    qsort(dict->palabras, dict->count, sizeof(char*), comparar_palabras);
    
    FILE *archivo = fopen(nombre_archivo, "w");  // Abre el archivo en modo escritura
    if (archivo == NULL) {  // Si no se pudo crear
        printf("Error al crear el archivo: %s\n", nombre_archivo);  // Muestra error
        return;  // Sale de la función
    }
    
    for (int i = 0; i < dict->count; i++) {  // Recorre todas las palabras
        fprintf(archivo, "%s\n", dict->palabras[i]);  // Escribe cada palabra en una línea
    }
    
    fclose(archivo);  // Cierra el archivo
    printf("Archivo '%s' creado con %d palabras únicas.\n", nombre_archivo, dict->count);  // Mensaje de éxito
}

// Función para liberar memoria del diccionario
// Parámetro: dict - puntero al diccionario a liberar
void liberar_diccionario(DiccionarioPalabras *dict) {
    for (int i = 0; i < dict->count; i++) {  // Recorre todas las palabras
        free(dict->palabras[i]);  // Libera la memoria de cada palabra individual
    }
    free(dict->palabras);  // Libera el arreglo de punteros
    free(dict);  // Libera la estructura del diccionario
}
