# 🧠 Librairy ML — Subsistema de Deep Learning

Este módulo contiene el código en Python para el desarrollo, entrenamiento, evaluación y exportación a **ONNX** de los modelos de clasificación acústica y embeddings de audio para **Librairy**.

---

## 📁 Estructura del Módulo

```text
ml/
├── pyproject.toml          # Definición de dependencias (PEP 621)
├── README.md               # Este archivo de guía
├── taxonomy/
│   └── categories.yaml     # Taxonomía jerárquica de clases sonoras (basada en UCS)
├── training/
│   ├── __init__.py
│   ├── dataset.py          # PyTorch AudioDataset con data augmentation y resampling
│   ├── model.py            # Red convolucional MelSpectrogramFrontend + AudioClassifier
│   ├── train.py            # Bucle de entrenamiento y validación
│   └── export_onnx.py      # Exportador y validador de paridad PyTorch <-> ONNX
└── notebooks/              # Análisis de datos y prototipado experimental
```

---

## 🛠️ Instalación y Configuración del Entorno

Recomendamos usar Python 3.10 o superior y un entorno virtual dedicado:

```bash
cd ml

# 1. Crear entorno virtual
python3 -m venv .venv

# 2. Activar entorno virtual
# En macOS / Linux:
source .venv/bin/activate
# En Windows (PowerShell):
# .venv\Scripts\Activate.ps1

# 3. Instalar dependencias con soporte de entrenamiento y desarrollo
pip install --upgrade pip
pip install -e ".[train,dev]"
```

---

## 📊 Preparación de Datasets

El `AudioDataset` espera una estructura de directorios agrupada por categorías (compatible con el estándar de carpetas UCS):

```text
mi_dataset/
├── Ambience/
│   ├── bosque_01.wav
│   ├── ciudad_tarde.wav
│   └── lluvia_techo.flac
├── Foley/
│   ├── pasos_grava_01.wav
│   └── puerta_madera.wav
├── Water/
│   ├── rio_corriente.wav
│   └── cascada.wav
└── Animals/
    └── pajaros_amanecer.wav
```

Formatos de audio soportados: `.wav`, `.flac`, `.mp3`, `.ogg`, `.aiff`, `.aif`.

---

## 🚀 Entrenamiento

Para entrenar el clasificador convolucional:

```bash
python -m training.train \
    --data_dir /ruta/a/mi_dataset \
    --output_dir ./checkpoints \
    --epochs 50 \
    --batch_size 32 \
    --lr 0.001 \
    --sample_rate 22050 \
    --duration 5.0
```

Durante el entrenamiento se generará el archivo `checkpoints/best_model.pt` con los pesos del modelo que obtenga la mejor precisión (`val_acc`) en el subconjunto de validación.

---

## 📦 Exportación a ONNX para la App C++

Para generar los grafos ONNX optimizados requeridos por el ejecutable de Librairy:

```bash
python -m training.export_onnx \
    --checkpoint ./checkpoints/best_model.pt \
    --output ../models/librairy_classifier.onnx
```

Este comando genera:
1. `../models/librairy_classifier.onnx`: Grafo principal para clasificación y estimación de probabilidades.
2. `../models/librairy_classifier_embedding.onnx`: Grafo extractor de vectores latentes $L_2$ normalizados para búsqueda por similitud acústica.
3. `../models/librairy_classifier.classes.txt`: Lista ordenada de etiquetas de texto reconocidas por el modelo.

