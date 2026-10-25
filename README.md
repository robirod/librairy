# 🎧 Librairy — Professional Audio Library Manager & AI Tagger

[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://en.wikipedia.org/wiki/C%2B%2B20)
[![JUCE 8](https://img.shields.io/badge/JUCE-8.0.4-orange.svg)](https://juce.com/)
[![SQLite](https://img.shields.io/badge/SQLite-3.47-lightgrey.svg)](https://www.sqlite.org/)
[![Python](https://img.shields.io/badge/Python-3.10%2B-yellow.svg)](https://www.python.org/)
[![PyTorch](https://img.shields.io/badge/PyTorch-2.1%2B-red.svg)](https://pytorch.org/)
[![ONNX Runtime](https://img.shields.io/badge/Inference-ONNX%20Runtime-green.svg)](https://onnxruntime.ai/)

**Librairy** es un software nativo compilado de alto rendimiento diseñado para diseñadores de sonido, editores de audio y postproductores. Permite catalogar, indexar, buscar, previsualizar y transferir bibliotecas de sonido de gran tamaño directamente a sesiones de **Avid Pro Tools**, **Apple Logic Pro**, **Cockos Reaper**, **Steinberg Nuendo**, entre otros DAWs.

En su segunda fase, incorpora un motor de **Deep Learning** (embeddings multimodales de audio y clasificadores basados en CLAP / PANNs) que auto-clasifica grabaciones de campo sin metadatos y permite búsqueda semántica en lenguaje natural (ej. *"lluvia suave en bosque"*).

---

## 📑 Tabla de Contenidos

- [Características Principales](#-características-principales)
- [Arquitectura del Sistema](#-arquitectura-del-sistema)
- [Estructura del Repositorio](#-estructura-del-repositorio)
- [Requisitos Previos](#-requisitos-previos)
- [Compilación e Instalación](#-compilación-e-instalación)
- [Pipeline de Machine Learning (Etapa 2)](#-pipeline-de-machine-learning-etapa-2)
- [Documentación Técnica Detallada](#-documentación-técnica-detallada)
- [Roadmap de Desarrollo](#-roadmap-de-desarrollo)
- [Licencia y Atribución](#-licencia-y-atribución)

---

## 🚀 Características Principales

### Etapa 1: Core Library & Workflow DAW
- **Escaneo Asíncrono de Ultra Alta Velocidad**: Indexación multihilo recursiva con soporte para BWF (Broadcast Wave), AIFF, FLAC, MP3, OGG, CAF, RF64 y Wave64.
- **Motor de Metadatos Completo**: Lectura y edición in-place de chunks BWF (`bext`), iXML (Universal Category System - UCS) e ID3 sin decodificar ni degradar el audio.
- **Búsqueda Instantánea con SQLite FTS5**: Full-Text Search con tokenización Unicode y Porter Stemming para catálogos con más de 500,000 archivos.
- **Previsualización de Baja Latencia**: Motor de audio JUCE integrado con visualización interactiva de formas de onda (overview, scrub, zoom y cursor sincronizado).
- **Exportación & Transfer a DAW**:
  - Drag & Drop nativo hacia el timeline de Pro Tools, Reaper, Logic, etc.
  - Exportación estructurada por taxonomía/carpetas (Universal Category System).
  - Renombrado paramétrico mediante plantillas (`{category}_{filename}`).

### Etapa 2: Deep Learning & Búsqueda Semántica
- **Auto-Tagging Inteligente**: Inferencia local ultra-rápida con **ONNX Runtime** en C++ para clasificar tomas de campo sin metadatos.
- **Búsqueda Audio-to-Audio (Query by Example)**: Generación de embeddings acústicos L2 normalizados y búsqueda por distancia coseno/FAISS.
- **Búsqueda Text-to-Audio**: Consultas en lenguaje natural sin requerir coincidencias exactas en el nombre del archivo.
- **Active Learning**: El modelo aprende de las confirmaciones o correcciones manuales realizadas por el editor.

---

## 🏗️ Arquitectura del Sistema

```
┌─────────────────────────────────────────────────────────────┐
│                 PRESENTATION LAYER (JUCE 8)                 │
│  ┌──────────────┐ ┌──────────────┐ ┌─────────────────────┐  │
│  │ Library Tree │ │ Search & FTS │ │ Results Table (UCS) │  │
│  └──────────────┘ └──────────────┘ └─────────────────────┘  │
│  ┌───────────────────────────────┐ ┌─────────────────────┐  │
│  │ Interactive Waveform & Scrub  │ │ Metadata Panel      │  │
│  └───────────────────────────────┘ └─────────────────────┘  │
└──────────────────────────────┬──────────────────────────────┘
                               │
┌──────────────────────────────▼──────────────────────────────┐
│                    CORE LOGIC LAYER (C++20)                 │
│  ┌──────────────┐ ┌──────────────┐ ┌─────────────────────┐  │
│  │ AsyncScanner │ │ SearchEngine │ │ Metadata R/W Engine │  │
│  └──────────────┘ └──────────────┘ └─────────────────────┘  │
│  ┌──────────────┐ ┌──────────────┐ ┌─────────────────────┐  │
│  │ Audio Engine │ │ FileExporter │ │ AI Inference Bridge │  │
│  └──────────────┘ └──────────────┘ └─────────────────────┘  │
└──────────────────────────────┬──────────────────────────────┘
                               │
┌──────────────────────────────▼──────────────────────────────┐
│               DATA & STORAGE & EMBEDDINGS LAYER             │
│  ┌───────────────────────────────┐ ┌─────────────────────┐  │
│  │ SQLite 3 (WAL + FTS5 Index)   │ │ ONNX Runtime Models │  │
│  └───────────────────────────────┘ └─────────────────────┘  │
│  ┌───────────────────────────────┐ ┌─────────────────────┐  │
│  │ Waveform Cache / Thumbnails   │ │ Local Audio Files   │  │
│  └───────────────────────────────┘ └─────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
```

---

## 📂 Estructura del Repositorio

```
librairy/
├── CMakeLists.txt              # Configuración principal de CMake
├── README.md                   # Este archivo
├── docs/                       # Documentación técnica de arquitectura y diseño
│   ├── architecture.md         # Diseño arquitectónico y modelos de concurrencia
│   ├── database_schema.md      # Esquema SQLite, índices y triggers FTS5
│   ├── daw_integration.md      # Protocolos de exportación e interoperabilidad DAW
│   └── ml_pipeline.md          # Entrenamiento, taxonomía UCS y exportación a ONNX
├── src/
│   ├── main.cpp                # Punto de entrada de la aplicación JUCE
│   ├── core/                   # Base de datos, escáner, motor de búsqueda
│   ├── metadata/               # Parsers BWF, iXML, Vorbis, ID3
│   ├── audio/                  # Reproducción, generación de forma de onda
│   ├── transfer/               # Exportación, copia estructurada y drag & drop
│   ├── ui/                     # Componentes gráficos (tablas, panels, searchbars)
│   └── ai/                     # Inferencia C++ con ONNX Runtime (Etapa 2)
├── ml/                         # Entorno de Machine Learning en Python
│   ├── pyproject.toml          # Dependencias y configuración de packaging
│   ├── taxonomy/               # Taxonomía jerárquica de categorías en YAML
│   ├── training/               # Datasets, arquitecturas de red, train loops
│   └── notebooks/              # Notebooks exploratorios
├── tests/                      # Suite de pruebas unitarias con Catch2
└── third_party/                # Dependencias embebidas (SQLite Amalgamation)
```

---

## 🛠️ Requisitos Previos

### Entorno C++
- **Compilador**: Clang 15+ (macOS/Linux), GCC 12+ (Linux), o MSVC 2022 v17.4+ (Windows).
- **CMake**: Versión 3.24 o superior.
- **Git**: Para descarga automática de submódulos y dependencias vía `FetchContent`.
- **macOS**: Xcode Command Line Tools (`xcode-select --install`). Se enlazan automáticamente los frameworks del sistema (`CoreAudio`, `AudioToolbox`, `Accelerate`, `Cocoa`).

### Entorno Python (Machine Learning - Etapa 2)
- **Python**: 3.10 o superior.
- **pip** o **uv** para la gestión de paquetes en entornos virtuales.

---

## ⚙️ Compilación e Instalación

### 1. Clonar el repositorio
```bash
git clone https://github.com/tu-usuario/librairy.git
cd librairy
```

### 2. Configurar y compilar con CMake

Para compilar la aplicación base con la suite de pruebas unitarias:
```bash
# Crear directorio de build
cmake -B build -DCMAKE_BUILD_TYPE=Release -DLIBRAIRY_BUILD_TESTS=ON

# Compilar proyecto en paralelo
cmake --build build --config Release -j$(nproc 2>/dev/null || sysctl -n hw.ncpu)
```

> **Nota para macOS**: El binario ejecutable o el bundle `.app` se generará en `build/Librairy_artefacts/Release/`.

### 3. Ejecutar las Pruebas Unitarias
```bash
ctest --test-dir build --output-on-failure
# O ejecutar directamente el ejecutable de Catch2:
./build/librairy_tests
```

### 4. Compilación con Aceleración AI (Opcional - Etapa 2)
Para compilar con soporte activo de inferencia ONNX Runtime:
```bash
cmake -B build -DLIBRAIRY_ENABLE_AI=ON -DCMAKE_PREFIX_PATH="/path/to/onnxruntime"
cmake --build build -j
```

---

## 🧠 Pipeline de Machine Learning (Etapa 2)

El directorio `ml/` aloja el pipeline completo de ingeniería acústica y deep learning:

```bash
cd ml

# 1. Crear entorno virtual
python3 -m venv .venv
source .venv/bin/activate

# 2. Instalar dependencias de entrenamiento
pip install -e ".[train,dev]"

# 3. Entrenar el clasificador sobre una colección categorizada
python -m training.train \
    --data_dir /ruta/a/audios_clasificados \
    --output_dir ./checkpoints \
    --epochs 50 \
    --batch_size 32

# 4. Validar y exportar el modelo a ONNX
python -m training.export_onnx \
    --checkpoint ./checkpoints/best_model.pt \
    --output ../models/librairy_classifier.onnx
```

Para más detalles sobre arquitecturas de red, datasets recomendados (FSD50K, AudioSet) y optimizaciones de latencia, consulta [`docs/ml_pipeline.md`](docs/ml_pipeline.md).

---

## 📚 Documentación Técnica Detallada

Para profundizar en el diseño e implementación de cada subsistema:

1. [**Arquitectura General del Sistema**](docs/architecture.md) — Modelo de hilos, diagramas de secuencia, desacoplamiento GUI/Core.
2. [**Esquema de Base de Datos y Búsqueda**](docs/database_schema.md) — Estructura SQLite, optimizaciones FTS5, integridad referencial y benchmarks.
3. [**Integración e Interoperabilidad con DAWs**](docs/daw_integration.md) — Protocolos Drag & Drop, spot to timeline, estándares de metadatos BWF/iXML.
4. [**Pipeline de Deep Learning & Auto-Tagging**](docs/ml_pipeline.md) — Taxonomía UCS, entrenamiento, embeddings de audio e inferencia en C++.

---

## 🗺️ Roadmap de Desarrollo

- [x] **Módulo 0**: Scaffolding inicial, CMake con JUCE 8, SQLite FTS5 y tests con Catch2.
- [ ] **Módulo 1**: Escaneo asíncrono y parser profundo de chunks BWF/iXML.
- [ ] **Módulo 2**: UI completa (tabla virtualizada, barra de búsqueda con auto-completado y vista de forma de onda).
- [ ] **Módulo 3**: Integración de audio en tiempo real, transporte y scrubbing interactivo.
- [ ] **Módulo 4**: Drag & Drop a Pro Tools y exportador por lotes con reglas de renombrado.
- [ ] **Módulo 5**: Integración de inferencia ONNX Runtime en C++ y auto-etiquetado de grabaciones de campo.

---

## 📄 Licencia y Atribución

Este proyecto utiliza librerías de terceros con sus respectivas licencias:
- **JUCE Framework**: Licencia dual (GPLv3 / Comercial).
- **SQLite**: Dominio Público.
- **Catch2**: Licencia Boost Software License 1.0.
- **ONNX Runtime**: Licencia MIT.

