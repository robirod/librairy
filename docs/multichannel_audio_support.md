# Mejora: Soporte Multicanal (Mono, Estéreo, 3.0, 5.1, 7.1, Atmos)

**Estado:** Propuesta  
**Épica:** `EP-01` - Escaneo, Indexado y Metadatos  
**Prioridad:** Alta  
**Tipo:** Mejora arquitectónica  
**Afecta a:** `src/core/`, `src/metadata/`, `src/audio/`, `src/ui/`, `schema SQLite`

## 1. Objetivo

Ampliar el soporte actual de Librairy para identificar, almacenar, visualizar y previsualizar correctamente archivos de audio con cualquier configuración de canales (Mono, Estéreo, LCR, 3.0, 4.0, 5.0, 5.1, 6.1, 7.1, 7.1.4), manteniendo el principio de **preservación estricta del PCM original** y garantizando una experiencia fluida para el usuario.

## 2. Estado Actual

| Componente | Estado | Observaciones |
|---|---|---|
| **Base de datos (`files.num_channels`)** | Parcial | Solo se guarda la cantidad numérica de canales. No se distingue entre un archivo 5.1 y otro con 6 canales con un layout distinto. |
| **Motor de Audio (`AudioEngine`)** | Parcial | Basado en JUCE, el cual sí soporta dispositivos de salida multicanal. Falta definir la política de *preview*. |
| **`MetadataReader`** | Parcial | Previsto para leer BWF/iXML, pero no se extrae ni persiste el `channel layout`. |
| **UI (`ResultsTable`)** | Parcial | Muestra la columna `CH`, pero solo con el número de canales (`2`, `6`). No muestra `5.1`, `7.1`, etc. |
| **Previsualización / Forma de onda** | Pendiente | No se ha definido cómo audicionar o generar thumbnails para archivos multicanal. |
| **IA / Auto-Tagging** | Pendiente | Modelos como CLAP/PANNs esperan audio mono o estéreo. Requiere una política de conversión. |

## 3. Gaps Identificados

- **Falta de `channel_layout`**: No existe forma de distinguir entre distintos layouts con el mismo número de canales.
- **Dependencia exclusiva de `bext`**: El chunk `bext` de BWF no define el orden o distribución de canales. Esta información suele encontrarse en el chunk `iXML`.
- **Audición sin downmix**: Si se intenta reproducir un 5.1 directamente con auriculares o con una salida estéreo, el resultado será incorrecto o inaudible.
- **Generación de forma de onda**: Crear un único waveform para todos los canales simultáneamente resulta poco legible para el usuario.
- **Inferencia IA**: Pasar un archivo multicanal completo al preprocesador de los modelos puede generar resultados errórones o innecesariamente costosos.
- **Exportación/Transferencia**: Falta definir si se mantiene, reconfigura o hace *downmix* al transferir a una sesión with distinto layout.

## 4. Propuesta de Solución

### 4.1. Modelo de Datos (SQLite)

Se modificaría la tabla `files` para almacenar información semántica del layout.

```sql
ALTER TABLE files ADD COLUMN channel_layout TEXT DEFAULT '';
ALTER TABLE files ADD COLUMN channel_mask    INTEGER DEFAULT 0;
ALTER TABLE files ADD COLUMN channel_order   TEXT DEFAULT '';
```

| Campo | Tipo | Descripción |
|---|---|---|
| `num_channels` | `INTEGER` | Cantidad total de canales (existente). Mantener por compatibilidad y filtrado rápido. |
| `channel_layout` | `TEXT` | Layout legible para UI. Valores esperados: `mono`, `stereo`, `3.0`, `4.0`, `5.0`, `5.1`, `6.1`, `7.1`, `7.1.4`. |
| `channel_mask` | `INTEGER` | Máscara de canales estilo Windows/WAVEFORMATEXTENSIBLE. Permite mayor portabilidad e interoperabilidad con otras herramientas. |
| `channel_order` | `TEXT` | (Opcional) JSON con el orden real extraído desde `iXML`. Ej: `["L","R","C","LFE","Ls","Rs"]`. Útil para depuración o layouts no estándar. |

También se añadirá un índice para facilitar filtrado por layout cuando sea necesario:

```sql
CREATE INDEX IF NOT EXISTS idx_files_channel_layout ON files(channel_layout);
```

### 4.2. Parsing de Metadatos (`src/metadata/`)

El `MetadataReader` deberá enriquecer la extracción con las siguientes fuentes:

| Fuente | Acción |
|---|---|
| **Chunk `FMT` (WAV)** | Obtener `nChannels`, `wBitsPerSample`, `nSamplesPerSec`. Base obligatoria. |
| **Chunk `iXML`** | Parsear `<TRACK_LIST>` para obtener nombres, índices y asignación de canales. Principal fuente para deducir `5.1`, `7.1`, etc. |
| **Chunk `bext` (BWF)** | Mantener su lectura actual, pero **no** utilizarlo como única fuente para el layout de canales. |
| **Heurística** | Si no existe información en `iXML` y el layout no es estándar, se aplicará una inferencia conservadora (ej. `2 -> stereo`, `6 -> 5.1`), marcando el resultado con menor confianza en logs internos. |

### 4.3. Motor de Audio y Previsualización (`src/audio/`)

Se define una política explícita para la audición dentro de Librairy:

| Aspecto | Propuesta | Justificación |
|---|---|---|
| **Preview Downmix** | Por defecto, **downmix a estéreo** para previsualización. | Garantiza compatibilidad con auriculares, altavoces estéreo y la gran mayoría de setups de edición. |
| **Algoritmo de Downmix** | Implementar un downmix ponderado (basado en coeficientes ITU-R BS.775) configurable. | Más fiel perceptivamente que una simple suma/mezcla. |
| **Routing a salida física** | Respetar el dispositivo multicanal seleccionado. Si el usuario reproduce desde el DAW o con interfaz 5.1 activa, JUCE deberá enrutar correctamente al hardware. | Mantener flexibilidad sin romper la previsualización. |
| **Sin modificación al archivo** | El downmix **únicamente** se realizará en tiempo real para la previsualización. Nunca se reescribirá ni copiará el archivo fuente. | Cumple con el requisito de "preservación estricta de datos". |

### 4.4. Forma de Onda (`src/ui/WaveformView`)

- **Base para thumbnail**: Generar el `AudioThumbnail` a partir de la **señal downmix a estéreo** utilizada para la previsualización.
- **Consistencia**: Lo que el usuario ve en la forma de onda debe coincidir con lo que escucha al hacer *scrub* o *play*.
- **Rendimiento**: Seguir utilizando el sistema de caché existente (`AudioThumbnailCache`) para evitar regeneraciones costosas.
- **Futuro**: Se deja abierta la posibilidad de añadir una vista "por canales" en una iteración posterior, pero **no** será la vista por defecto.

### 4.5. Interfaz de Usuario (`src/ui/`)

| Componente | Cambio | Detalle |
|---|---|---|
| **`ResultsTable`** | Mostrar layout legible | Sustituir/acompañar `CH` por `LAYOUT` o mostrar `5.1` directamente cuando esté disponible. Mantener número de canales como fallback. |
| **`MetadataPanel`** | Nueva información | Añadir fila `Canales` mostrando `Layout: 5.1 (6 canales)` y, si existe `channel_order`, poder visualizarlo en la pestaña `BWF / iXML`. |
| **Filtros** | Ampliar filtros | Añadir filtro por `Formato de canales` (`Mono`, `Estéreo`, `Multicanal`) para agilizar la búsqueda. |

### 4.6. IA / Auto-Tagging (`src/ai/`)

Para mantener compatibilidad con los modelos definidos en `ml_pipeline.md`:

| Acción | Descripción |
|---|---|
| **Normalización** | Antes de la inferencia, convertir cualquier archivo multicanal a **mono downmix** o, preferentemente, a un **mono mezclado ponderado** coherente con la percepción. |
| **Consistencia** | Utilizar la misma lógica de *downmix* que para previsualización siempre que sea posible para evitar sesgos entre "lo que se escucha" y "lo que se etiqueta". |
| **Documentación** | Añadir esta política explícita a `docs/ml_pipeline.md` al momento de implementar la integración ONNX. |
| **Configuración** | Permitir ajustar el método de downmix desde configuración interna (pensado para futuras pruebas con CLAP). |

### 4.7. Exportación / Transferencia a DAW (`src/transfer/`)

- **Preservación por defecto**: Al arrastrar al DAW, **mantener el archivo original sin remezclar**. El DAW será responsable de enrutarlo según su propia configuración.
- **Conversión opcional (futuro)**: Si en el futuro se añade conversión entre layouts, deberá ser **explícito y opt-in** por parte del usuario, nunca implícito.
- **Estructuración UCS**: No se ve afectada por el layout de canales. Seguirá aplicándose según `Category/Subcategory`.

## 5. Plan de Implementación

| Orden | Tarea | Épica | Notas |
|---|---|---|---|
| **1.** | Actualizar `docs/database_schema.md` con los nuevos campos y índice. | EP-01 | Documentar el cambio antes de tocar código. |
| **2.** | Ampliar `MetadataReader` para parsear `iXML` y deducir `channel_layout`. | EP-01 | Prioritario. Necesario para tener datos correctos desde el escaneo. |
| **3.** | Modificar `Database` para persistir los nuevos campos. | EP-01 | Añadir migración mínima para bases existentes (si las hubiera). |
| **4.** | Implementar *preview downmix* en `AudioEngine`. | EP-04 | Requiere `channel_layout` ya disponible. |
| **5.** | Ajustar `WaveformView` para usar señal downmixada. | EP-04 | Mantener coherencia con previsualización. |
| **6.** | Actualizar `ResultsTable` y `MetadataPanel` para mostrar layout. | EP-03 | Mejorar legibilidad para el usuario. |
| **7.** | Adaptar `AutoTagger`/`InferenceEngine` para downmix a mono. | EP-06 | Solo al activar `LIBRAIRY_ENABLE_AI`. |

## 6. Criterios de Aceptación

- [ ] Archivos mono, estéreo, 5.1 y 7.1 son correctamente identificados tras el escaneo.
- [ ] La UI muestra `5.1 (6ch)`, `7.1 (8ch)`, etc., en lugar de únicamente el número.
- [ ] Al previsualizar un archivo 5.1 con salida estéreo/auriculares, se escucha correctamente sin artefactos.
- [ ] La forma de onda generada corresponde al downmix utilizado para la audición.
- [ ] El archivo original permanece **íntegro** (sin cambios de bytes).
- [ ] El filtro por "Multicanal" funciona correctamente.
- [ ] Al arrastrar un archivo multicanal al DAW, se conserva sin modificaciones.
- [ ] La inferencia IA funciona con archivos multicanal gracias al downmix a mono.

## 7. Riesgos y Mitigaciones

| Riesgo | Impacto | Mitigación |
|---|---|---|
| **iXML ausente** | Medio | Usar heurística conservadora + mostrar `Desconocido` en UI sin bloquear el escaneo. |
| **Layouts no estándar** | Medio | Guardar `channel_order` en JSON y mostrar `X canales (layout personalizado)`. |
| **Latencia en previsualización** | Bajo | Realizar downmix en tiempo real con buffers ya existentes. Evitar reallocations en el *audio thread*. |
| **Diferentes implementaciones iXML** | Medio | Ser tolerante con el parser XML (ignorar tags desconocidos). |
| **Sobrecarga CPU en escaneo** | Bajo | Parseo de iXML ligero, solo cuando se detecta chunk `iXML`. |

## 8. Notas

- **Enfoque pragmático:** Cubrir mono/estéreo/5.1/7.1 cubre la gran mayoría de librerías profesionales de SFX. Layouts más exóticos quedan cubiertos por `num_channels` + `channel_order`.
- **Coherente con filosofía del proyecto:** *"Preservación estricta de datos"* se mantiene al 100%. Todo el manejo multicanal para previsualización es **transitorio y en memoria**.
- **Preparado para el futuro:** `channel_mask` deja la puerta abierta a una integración más profunda con motores multicanal o con exportadores avanzados.

**Acción:** Esta mejora debe abordarse **obligatoriamente** dentro de `EP-01` antes de dar por finalizado el escaneo y antes de profundizar en `EP-04` (Audio en Tiempo Real y Previsualización).
