# Sistema de Information Retrieval

Sistema completo de búsqueda y ranking de documentos en C.

## 🚀 Inicio Rápido

```bash
# 1. Hacer ejecutable
chmod +x ir.sh

# 2. Ejecutar menú interactivo
./ir.sh
```

## 📦 Componentes

### 1. Indexador
Genera índice invertido de una colección de documentos.

**Uso directo:**
```bash
./bin/indexador <base_texto.txt> <stopwords.txt>
```

**Genera:**
- `vocabulario.txt` - Palabras con IDs y frecuencia documental
- `documentos.txt` - Lista de documentos
- `listas_invertidas.txt` - Listas invertidas con frecuencias

**Fórmula:**
```
Frec(t,i) = F(t,i) / Fmx(i)
```

### 2. Buscador
Busca documentos usando ranking TF-IDF.

**Uso directo:**
```bash
./bin/buscador vocabulario.txt documentos.txt listas_invertidas.txt <consulta>
```

**Ejemplos:**
```bash
./bin/buscador vocabulario.txt documentos.txt listas_invertidas.txt pinochet
./bin/buscador vocabulario.txt documentos.txt listas_invertidas.txt chile gobierno
```

**Fórmula de ranking:**
```
W(t,i) = log₁₀(N / D(t)) × Frec(t,i)
R(i,Q) = Σ W(t,i) para todo t en Q
```

### 3. Buscador Paralelo
Versión distribuida con MPI (o simulación).

**Con MPI:**
```bash
mpirun -np 4 ./bin/buscador_paralelo 10 10
```

**Simulación (sin MPI):**
```bash
./ir.sh  # Opción 4 y 5
```

## 🔧 Compilación Manual

```bash
# Todos los programas
make

# Solo buscador paralelo (requiere MPI)
make all-parallel

# Limpiar
make clean
```

## 📁 Archivos

```
src/
  ├── indexador.c           - Genera índice invertido
  ├── buscador.c            - Búsqueda con ranking
  └── buscador_paralelo.c   - Búsqueda paralela MPI

txt/
  ├── base_texto.txt        - Corpus completo (1318 docs)
  ├── base_texto_ejemplo.txt - Ejemplo pequeño (3 docs)
  ├── stopwords.txt         - Palabras irrelevantes
  └── consultas_ejemplo.txt - Consultas de prueba

ir.sh                       - Script único interactivo
Makefile                    - Sistema de compilación
```

## 📊 Ejemplo Completo

### Entrada (base_texto_ejemplo.txt):
```
<DOCUMENTO 1>
<URL ./test/doc1.html>
<TEXTO>
hola mundo mundo chao mundo
</TEXTO>
</DOCUMENTO>

<DOCUMENTO 2>
<URL ./test/doc2.html>
<TEXTO>
hola chao hola
</TEXTO>
</DOCUMENTO>

<DOCUMENTO 3>
<URL ./test/doc3.html>
<TEXTO>
casa arbol casa
</TEXTO>
</DOCUMENTO>
```

### Salida:
```bash
$ ./bin/indexador txt/base_texto_ejemplo.txt txt/stopwords.txt
# Genera: vocabulario.txt, documentos.txt, listas_invertidas.txt

$ ./bin/buscador vocabulario.txt documentos.txt listas_invertidas.txt arbol
Consulta [ arbol ]:
(3,0.239)

$ ./bin/buscador vocabulario.txt documentos.txt listas_invertidas.txt arbol casa
Consulta [ arbol casa ]:
(3,0.716)
```

## 🎯 Casos de Uso

### Búsqueda Simple
```bash
./ir.sh
# Opción 1: Compilar
# Opción 2: Generar índice (seleccionar base_texto.txt)
# Opción 3: Realizar búsqueda (escribir: pinochet chile)
```

### Búsqueda Paralela
```bash
./ir.sh
# Opción 4: Particionar base (seleccionar 4 procesadores)
# Opción 5: Ejecutar paralelo (Q=10, K=10)
# Ver resultados en salida_0.txt, salida_1.txt, etc.
```

### Demo Rápida
```bash
./ir.sh
# Opción 7: Ejecutar demo completa
```

## 🔍 Estadísticas

**Base completa (base_texto.txt):**
- Documentos: 1,318
- Palabras relevantes: ~13,000
- Período: 1998-2001
- Fuente: Noticias chilenas

**Base ejemplo (base_texto_ejemplo.txt):**
- Documentos: 3
- Palabras relevantes: 5
- Ideal para pruebas rápidas

## ⚙️ Requisitos

- GCC (C99)
- Make
- Python 3 (para búsqueda paralela simulada)
- MPI (opcional, solo para buscador paralelo real)

**Instalar MPI (opcional):**
```bash
sudo apt-get install mpich libmpich-dev
```

## 🐛 Solución de Problemas

**Error: "No such file"**
→ Ejecuta primero la opción 1 (Compilar)

**Error: "Cannot open vocabulario.txt"**
→ Ejecuta primero la opción 2 (Generar índice)

**Resultados vacíos**
→ Verifica que las palabras de búsqueda existan en el vocabulario

## 📝 Notas

- Los IDs de documentos empiezan en 1 (en salida)
- Se usa log₁₀ (no ln) para el cálculo de W(t,i)
- Las stopwords se filtran automáticamente
- El sistema es case-insensitive

## 🎓 Autor

Sistema de Information Retrieval - 2025
