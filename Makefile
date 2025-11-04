CC=gcc
CXX=g++
CFLAGS=-Wall -Wextra -std=c99 -O2
CXXFLAGS=-Wall -Wextra -std=c++11 -O2
SRCDIR=src
BINDIR=bin
TXTDIR=txt
TARGET1=$(BINDIR)/extraer_palabras
TARGET2=$(BINDIR)/comparar_diccionarios
TARGET3=$(BINDIR)/combinar_diccionarios
TARGET4=$(BINDIR)/generar_base_texto
TARGET5=$(BINDIR)/indexador
TARGET6=$(BINDIR)/buscador
TARGET7=$(BINDIR)/buscador_paralelo
SOURCE1=$(SRCDIR)/extraer_palabras.c
SOURCE2=$(SRCDIR)/comparar_diccionarios.c
SOURCE3=$(SRCDIR)/combinar_diccionarios.c
SOURCE4=$(SRCDIR)/generar_base_texto.c
SOURCE5=$(SRCDIR)/indexador.c
SOURCE6=$(SRCDIR)/buscador.c
SOURCE7=$(SRCDIR)/buscador_paralelo.c

# Archivos de datos
BASE_HTML=$(TXTDIR)/base_html.txt
ES_CL=$(TXTDIR)/es_CL.txt
DIFF_TXT=$(TXTDIR)/diff.txt
DICT_TXT=$(TXTDIR)/dict.txt
BASE_TEXTO=$(TXTDIR)/base_texto.txt

all: $(TARGET1) $(TARGET2) $(TARGET3) $(TARGET4) $(TARGET5) $(TARGET6)

all-parallel: all $(TARGET7)

# Compilar programas
$(TARGET1): $(SOURCE1) | $(BINDIR)
	$(CC) $(CFLAGS) -o $(TARGET1) $(SOURCE1)

$(TARGET2): $(SOURCE2) | $(BINDIR)
	$(CC) $(CFLAGS) -o $(TARGET2) $(SOURCE2)

$(TARGET3): $(SOURCE3) | $(BINDIR)
	$(CC) $(CFLAGS) -o $(TARGET3) $(SOURCE3)

$(TARGET4): $(SOURCE4) | $(BINDIR)
	$(CC) $(CFLAGS) -o $(TARGET4) $(SOURCE4)

$(TARGET5): $(SOURCE5) | $(BINDIR)
	$(CC) $(CFLAGS) -o $(TARGET5) $(SOURCE5)

$(TARGET6): $(SOURCE6) | $(BINDIR)
	$(CC) $(CFLAGS) -o $(TARGET6) $(SOURCE6) -lm

$(TARGET7): $(SOURCE7) | $(BINDIR)
	mpicc $(CFLAGS) -o $(TARGET7) $(SOURCE7) -lm

# Generar código C desde Lex
$(LEX_OUTPUT): $(LEX_SOURCE)
	flex -o $(LEX_OUTPUT) $(LEX_SOURCE)

$(BINDIR):
	mkdir -p $(BINDIR)

# Generar datos para IR
ir-data: $(BASE_TEXTO)

# Extraer palabras de HTML
$(BASE_HTML): $(TARGET1) | $(TXTDIR)
	./$(TARGET1) tarea1

# Crear es_CL.txt a partir de diccionario.txt
$(ES_CL): $(TXTDIR)/diccionario.txt | $(TXTDIR)
	cp $(TXTDIR)/diccionario.txt $(ES_CL)

# Generar diff.txt
$(DIFF_TXT): $(TARGET2) $(BASE_HTML) $(ES_CL)
	./$(TARGET2) $(BASE_HTML) $(ES_CL) $(DIFF_TXT)

# Generar dict.txt (usando comando Unix más eficiente)
$(DICT_TXT): $(ES_CL) $(DIFF_TXT)
	cat $(ES_CL) $(DIFF_TXT) | sort -u > $(DICT_TXT)

# Generar base_texto.txt
$(BASE_TEXTO): $(TARGET4) $(DICT_TXT)
	./$(TARGET4) tarea1 $(DICT_TXT)

$(TXTDIR):
	mkdir -p $(TXTDIR)

clean:
	rm -f $(BINDIR)/*
	rm -f $(LEX_OUTPUT)

# Clean solo archivos generados automáticamente por make (no los archivos base como diccionario.txt)
clean-generated:
	rm -f $(BASE_HTML) $(DIFF_TXT) $(DICT_TXT) $(BASE_TEXTO) $(ES_CL)

# Clean archivos de prueba
clean-tests:
	rm -f tests/test_*.txt

# Clean completo (incluyendo archivos base - usar con cuidado)
clean-all: clean clean-generated clean-tests
	rm -f $(TXTDIR)/diccionario.txt

# Mostrar ayuda
help:
	@echo "Objetivos disponibles:"
	@echo "  all           - Compilar todos los programas (incluyendo verificador)"
	@echo "  ir-data       - Generar todos los archivos de datos para IR"
	@echo "  verificar     - Verificar estructura de base_texto.txt"
	@echo "  clean         - Limpiar ejecutables y archivos generados por lex"
	@echo "  clean-generated - Limpiar archivos generados automáticamente"
	@echo "  clean-tests   - Limpiar archivos de prueba"
	@echo "  clean-all     - Limpiar todo (incluyendo archivos base)"
	@echo ""
	@echo "Archivos generados:"
	@echo "  $(BASE_HTML)  - Palabras extraídas de HTML"
	@echo "  $(ES_CL)       - Copia del diccionario español"
	@echo "  $(DIFF_TXT)    - Palabras faltantes (requiere limpieza manual)"
	@echo "  $(DICT_TXT)    - Diccionario final combinado"
	@echo "  $(BASE_TEXTO)  - Corpus procesado para IR"
	@echo ""
	@echo "Verificador:"
	@echo "  $(TARGET5)     - Verificador de estructura de tags"
	@echo ""
	@echo "Estructura del proyecto:"
	@echo "  src/          - Código fuente"
	@echo "  bin/          - Ejecutables compilados"
	@echo "  txt/          - Archivos de datos"
	@echo "  tests/        - Archivos de prueba"
	@echo "  docs/         - Documentación"
	@echo "  tarea1/       - Corpus HTML de entrada"

# Verificar base_texto.txt
verificar: $(TARGET5)
	./$(TARGET5) $(BASE_TEXTO)

.PHONY: all clean clean-generated clean-tests clean-all help ir-data verificar
