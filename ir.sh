#!/bin/bash
# Script simplificado para el sistema IR con opciones

# Función para mostrar el menú
mostrar_menu() {
    echo ""
    echo "╔════════════════════════════════════════════════════════╗"
    echo "║     SISTEMA DE INFORMATION RETRIEVAL                  ║"
    echo "╚════════════════════════════════════════════════════════╝"
    echo ""
    echo "1) Compilar sistema"
    echo "2) Extraer palabras de archivos HTML"
    echo "3) Comparar diccionarios"
    echo "4) Combinar diccionarios"
    echo "5) Generar base de texto"
    echo "6) Indexar (generar índice invertido)"
    echo "7) Realizar búsqueda"
    echo ""
    echo "8) Ejecutar TODO el proceso (pasos 1-6)"
    echo ""
    echo "0) Salir"
    echo ""
}

# Función para compilar
compilar() {
    echo ""
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo "COMPILANDO SISTEMA..."
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    make clean > /dev/null 2>&1
    make
    if [ $? -ne 0 ]; then
        echo "❌ Error en la compilación"
        return 1
    fi
    echo "✓ Compilación completada"
    return 0
}

# Función para extraer palabras
extraer_palabras() {
    echo ""
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo "EXTRAYENDO PALABRAS DE ARCHIVOS HTML..."
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    
    if [ ! -f "bin/extraer_palabras" ]; then
        echo "❌ Error: Primero debes compilar (opción 1)"
        return 1
    fi
    
    ./bin/extraer_palabras tarea1
    if [ $? -ne 0 ]; then
        echo "❌ Error al extraer palabras"
        return 1
    fi
    echo "✓ Palabras extraídas en txt/base_html.txt"
    return 0
}

# Función para comparar diccionarios
comparar_diccionarios() {
    echo ""
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo "COMPARANDO DICCIONARIOS..."
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    
    if [ ! -f "bin/comparar_diccionarios" ]; then
        echo "❌ Error: Primero debes compilar (opción 1)"
        return 1
    fi
    
    if [ ! -f "txt/base_html.txt" ]; then
        echo "❌ Error: Primero debes extraer palabras (opción 2)"
        return 1
    fi
    
    ./bin/comparar_diccionarios txt/base_html.txt txt/es_CL.txt txt/diff.txt
    if [ $? -ne 0 ]; then
        echo "❌ Error al comparar diccionarios"
        return 1
    fi
    echo "✓ Diferencias guardadas en txt/diff.txt"
    return 0
}

# Función para combinar diccionarios
combinar_diccionarios() {
    echo ""
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo "COMBINANDO DICCIONARIOS..."
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    
    if [ ! -f "bin/combinar_diccionarios" ]; then
        echo "❌ Error: Primero debes compilar (opción 1)"
        return 1
    fi
    
    if [ ! -f "txt/diff.txt" ]; then
        echo "❌ Error: Primero debes comparar diccionarios (opción 3)"
        return 1
    fi
    
    ./bin/combinar_diccionarios txt/es_CL.txt txt/diff.txt txt/dict.txt
    if [ $? -ne 0 ]; then
        echo "❌ Error al combinar diccionarios"
        return 1
    fi
    echo "✓ Diccionario combinado guardado en txt/dict.txt"
    return 0
}

# Función para generar base de texto
generar_base_texto() {
    echo ""
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo "GENERANDO BASE DE TEXTO..."
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    
    if [ ! -f "bin/generar_base_texto" ]; then
        echo "❌ Error: Primero debes compilar (opción 1)"
        return 1
    fi
    
    if [ ! -f "txt/base_html.txt" ]; then
        echo "❌ Error: Primero debes extraer palabras (opción 2)"
        return 1
    fi
    
    ./bin/generar_base_texto tarea1 txt/base_html.txt
    if [ $? -ne 0 ]; then
        echo "❌ Error al generar base de texto"
        return 1
    fi
    echo "✓ Base de texto generada en txt/base_texto.txt"
    return 0
}

# Función para indexar
indexar() {
    echo ""
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo "GENERANDO ÍNDICE INVERTIDO..."
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    
    if [ ! -f "bin/indexador" ]; then
        echo "❌ Error: Primero debes compilar (opción 1)"
        return 1
    fi
    
    if [ ! -f "txt/base_texto.txt" ]; then
        echo "❌ Error: Primero debes generar base de texto (opción 5)"
        return 1
    fi
    
    ./bin/indexador txt/base_texto.txt txt/stopwords.txt
    if [ $? -ne 0 ]; then
        echo "❌ Error al generar índice"
        return 1
    fi
    echo ""
    echo "✓ Archivos generados:"
    echo "  - vocabulario.txt"
    echo "  - documentos.txt"
    echo "  - listas_invertidas.txt"
    return 0
}

# Función para realizar búsqueda
realizar_busqueda() {
    echo ""
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo "SISTEMA DE BÚSQUEDA"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    
    if [ ! -f "bin/buscador" ]; then
        echo "❌ Error: Primero debes compilar (opción 1)"
        return 1
    fi
    
    if [ ! -f "vocabulario.txt" ] || [ ! -f "documentos.txt" ] || [ ! -f "listas_invertidas.txt" ]; then
        echo "❌ Error: Primero debes indexar (opción 6)"
        return 1
    fi
    
    echo ""
    echo "Sistema listo. Escribe 'salir' para volver al menú."
    echo ""
    
    while true; do
        read -p "Consulta: " consulta
        
        if [ "$consulta" = "salir" ] || [ "$consulta" = "exit" ] || [ "$consulta" = "q" ]; then
            echo "Volviendo al menú principal..."
            break
        fi
        
        if [ -z "$consulta" ]; then
            echo "Consulta vacía. Intenta de nuevo."
            continue
        fi
        
        echo ""
        ./bin/buscador vocabulario.txt documentos.txt listas_invertidas.txt $consulta
        echo ""
        echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
        echo ""
    done
}

# Función para ejecutar todo el proceso
ejecutar_todo() {
    echo ""
    echo "╔════════════════════════════════════════════════════════╗"
    echo "║     EJECUTANDO PROCESO COMPLETO                       ║"
    echo "╚════════════════════════════════════════════════════════╝"
    echo ""
    echo "NOTA: Usando base_html.txt (palabras extraídas limpiamente)"
    echo ""
    
    compilar
    [ $? -ne 0 ] && return 1
    
    extraer_palabras
    [ $? -ne 0 ] && return 1
    
    echo ""
    echo "⚠️  PASOS OPCIONALES (comparar y combinar diccionarios)"
    echo "   Saltando porque base_html.txt ya está limpio..."
    echo ""
    
    generar_base_texto
    [ $? -ne 0 ] && return 1
    
    indexar
    [ $? -ne 0 ] && return 1
    
    echo ""
    echo "╔════════════════════════════════════════════════════════╗"
    echo "║     ✓ PROCESO COMPLETO FINALIZADO                     ║"
    echo "╚════════════════════════════════════════════════════════╝"
    echo ""
    echo "El sistema está listo para realizar búsquedas (opción 7)"
}

# Bucle principal del menú
while true; do
    mostrar_menu
    read -p "Selecciona una opción: " opcion
    
    case $opcion in
        1) compilar ;;
        2) extraer_palabras ;;
        3) comparar_diccionarios ;;
        4) combinar_diccionarios ;;
        5) generar_base_texto ;;
        6) indexar ;;
        7) realizar_busqueda ;;
        8) ejecutar_todo ;;
        0) 
            echo ""
            echo "¡Gracias por usar el sistema IR!"
            echo ""
            exit 0
            ;;
        *) 
            echo ""
            echo "❌ Opción inválida. Por favor selecciona una opción del 0 al 8."
            ;;
    esac
    
    echo ""
    read -p "Presiona Enter para continuar..."
done