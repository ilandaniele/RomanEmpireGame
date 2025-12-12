# Roman Empire Game - Setup Guide 🎮🏛️

Un prototipo de Unreal Engine 5 que combina **Total War**, **Age of Empires**, y **Counter-Strike**.

---

## 🚀 Quick Start (3 pasos)

### 1. Abrir Proyecto
```
Double-click: RomanEmpireGame.uproject
```
Espera a que compile el código C++.

### 2. Ejecutar Script de Setup
```
Tools > Execute Python Script > Content/Scripts/MasterSetup.py
```
¡Esto crea TODOS los assets automáticamente!

### 3. Configurar y Jugar
```
Edit > Project Settings > Maps & Modes > Default GameMode: BP_GameMode
Press Play
```

---

## 📜 Scripts de Python (UE Editor)

| Script | Qué Crea |
|--------|----------|
| `MasterSetup.py` | **TODO** (ejecutar este) |
| `CreateBlueprints.py` | 13 Blueprints |
| `CreateMaterials.py` | 13 Materiales |
| `CreateAudioAssets.py` | 28 Sound Cues |

Para ejecutar: **Tools > Execute Python Script** > seleccionar archivo

---

## 🎨 Blueprints Generados

### Core
- `BP_GameMode` - Modo de juego principal
- `BP_PlayerController` - Control de input
- `BP_SeamlessCamera` - Cámara con zoom continuo
- `BP_AudioManager` - Sistema de audio
- `BP_MaterialManager` - Colores de facciones

### UI
- `WBP_MainHUD` - HUD completo con recursos, minimapa, menús

### Units
- `BP_Legionary` - Soldado romano con gladius, pilum, scutum
- `BP_UnitBase` - Base para todas las unidades

### Buildings
- `BP_Barracks` - Entrena infantería
- `BP_BuildingBase` - Base para edificios

### World
- `BP_Territory` - Regiones conquistables
- `BP_WorldMapManager` - Mapa del Mediterráneo
- `BP_CampaignManager` - Turnos y victoria
- `BP_FactionManager` - Roma, Cartago, Galia

---

## 🔊 Audio Assets

**28 Sound Cues** organizados en:

| Categoría | Sonidos |
|-----------|---------|
| Combat | SwordSwing, SwordHit, ShieldBlock, Death, Charge |
| Building | BuildStart, BuildProgress, BuildComplete |
| UI | ButtonClick, MenuOpen, TurnEnd, Victory |
| Ambient | BattleAmbience, CityAmbience, MarchingLoop |
| Music | MainMenuTheme, BattleTheme, CampaignTheme |

**Para agregar audio:**
1. Importa archivos `.wav` a `Content/Audio/`
2. Abre el Sound Cue correspondiente
3. Conecta el Wave Player al Output

Ver `Content/Audio/AudioConfig.json` para especificaciones técnicas.

---

## 📁 Archivos de Diseño (.design.json)

Estos archivos describen la estructura visual de cada Blueprint:

- `WBP_MainHUD.design.json` - Layout del HUD completo
- `BP_Legionary.design.json` - Equipo y animaciones del soldado
- `BP_Barracks.design.json` - Configuración del cuartel

Úsalos como referencia al editar Blueprints en el editor.

---

## ⚙️ Configuración Detallada

### Game Mode Setup
1. **Edit > Project Settings > Maps & Modes**
2. Set Default GameMode: `BP_GameMode`
3. Abre `BP_GameMode` y configura:
   - Default Pawn Class: `BP_SeamlessCamera`
   - Player Controller Class: `BP_PlayerController`
   - HUD Class: `RomanEmpireHUD`

### Level Setup
1. **File > New Level > Empty Level**
2. Save as `/Game/Maps/MainMap`
3. Agrega al nivel:
   - `BP_AudioManager`
   - `BP_MaterialManager`
   - Directional Light + Sky

---

## 🎮 Controles

| Tecla | Acción |
|-------|--------|
| 🖱️ Scroll | Zoom in/out |
| 🖱️ Click Izq | Seleccionar |
| 🖱️ Click Der | Mover/Atacar |
| `B` | Menú de edificios |
| `F` | Entrar modo FPS |
| `WASD` | Moverse (FPS) |
| `ESC` | Salir FPS |

---

## ❓ Troubleshooting

| Problema | Solución |
|----------|----------|
| Error de compilación | Ver Output Log, recompilar en VS |
| Clases no encontradas | Asegurar que C++ compiló correctamente |
| Sin sonido | Verificar device, revisar Sound Cue setup |
| Python script falla | Habilitar Python Editor Script Plugin |

---

## 📂 Estructura del Proyecto

```
RomanEmpireGame/
├── Config/              # Configuración UE5
├── Content/
│   ├── Audio/           # Sound Cues + AudioConfig.json
│   ├── Blueprints/      # BP generados + .design.json
│   └── Scripts/         # Python scripts para UE Editor
├── Source/
│   └── RomanEmpireGame/
│       ├── Core/        # GameMode, Controller, HUD
│       ├── Camera/      # SeamlessZoomCamera
│       ├── Faction/     # FactionManager
│       ├── Building/    # BuildingBase, Barracks
│       ├── Units/       # UnitBase, Legionary
│       ├── World/       # Territory, Campaign
│       ├── UI/          # Widgets
│       ├── Audio/       # AudioManager
│       └── Assets/      # Materials, Meshes
├── README.md
└── SETUP_GUIDE.md       # ← Estás aquí
```
