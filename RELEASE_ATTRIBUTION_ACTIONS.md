# Acciones para cerrar atribución — OPEN

2026-10-12. Hoja de trabajo, no aprobación legal ni autorización de distribución.
M7 sigue abierto; M8 requiere el slice offline completo. No instalar otro motor,
sustituir DLLs, publicar paquetes o modificar Windows por completar esta hoja.

## Evidencia y alcance

`Artifacts/C018H-DependencyInventory1/AttributionActionCoverage1.json`, SHA256
`77c46f7e8784b597ddeab17e3b6801d6e3d1be230a5617436036a6d4de2af735`,
vincula 28 recibos por hash y valida sus referencias JSON contra identidades
exactas: 61 bibliotecas Development, 54 Shipping, 21/13 DLLs en 11 familias.
Los grupos de bibliotecas no se solapan. Las DLLs son otra dimensión: no sumar
esas cifras para obtener un total de dependencias o licencias.

Esto prueba referencias de identidad, no qué objetos sobreviven al enlace,
proveniencia completa, compatibilidad ABI, proveedores efectivos, derechos ni
avisos finales. Los 33 nombres de bibliotecas de plataforma por configuración
quedan separados; fonts, contenido y dependencias transitivas también requieren
evaluación. Ninguna acción tiene aprobación del propietario registrada.

Detalles y fuentes ya revisados: [dependencias](M7_PACKAGED_DEPENDENCY_REVIEW.md),
[fonts](M7_FONT_ATTRIBUTION_REVIEW.md), [inventario de assets](ASSET_INVENTORY.md),
[avisos candidatos](THIRD_PARTY_NOTICES.md). No repetir búsquedas de esas fuentes.
Los recibos de `Artifacts/` son locales e ignorados: este documento conserva el
resumen versionable, no contiene las licencias ni un paquete distribuible.

`RetainedMetadataAvailability1.json`, SHA256
`9c1780e54ec18bc8025fa057924d821b0ea95b5ffb6bde04cb57a27f577da154`,
identifica los PDB existentes Development/Shipping y respuestas de enlace
hash-intactas con `/DEBUG:FULL`, sin flags `/MAP` o `/VERBOSE`. No se observaron
mapas en los directorios exactos de respuesta/binarios; no es ausencia global.
Siguiente trabajo local: vincular GUID/age PDB–EXE y estudiar contribuciones
mediante formatos documentados/lector seguro. La firma MSF7 y el nombre del PDB
no prueban esa vinculación ni retención. `llvm-pdbutil` no apareció en PATH o
tres rutas concretas probadas; no se instaló herramienta ni se ejecutó el player.

Ese trabajo avanzó: `PdbContributionReview1.json` SHA256
`4857a6a85625cbf679a38c2cc222f82770c760126373f26edc2431e0acfdbf83`
vincula GUID/age PDB–EXE/DBI y hashes EXE–QA; headers de secciones coinciden,
sin OMAP. Hay intervalos positivos asociados por sufijo de ruta a 54/61 libs
Development y 45/54 Shipping. No prueba todos los objetos/orígenes, bytes de
archivo original, ejecución o derechos. Ausencias no excluyen dependencias;
wrappers/imports/ICF/LTO y fuentes transitivas requieren alcance separado.
SSL/crypto/curl tienen contribuciones .text en ambos builds, no prueba de CVE
alcanzable ni mitigación. [Resultados y límites](M7_PACKAGED_DEPENDENCY_REVIEW.md).
Usar esta evidencia para ATR, sin volver a leer/parsear los PDB ya verificados.

`PeImportReview1.json`, SHA256
`2c32d6f2b3194fa21eed7d6464accbc802c95ddc468d4d50edb7a807e40549ed`,
completa la lectura de imports normales/diferidos de las 38 imágenes nativas
QA exactas (23Dev/15Ship). Vorbis/VorbisFile importan Ogg en ambos paquetes:
su ausencia en imports directos del Game Shipping no elimina obligaciones.
MsQuic/tbb12 sin descriptor explícito en este conjunto no significa ausencia de
carga dinámica o mitigación. Coincidencia de basename no resuelve path/version
del loader; imágenes/componentes cargados por otros mecanismos quedan fuera.
No repetir este parseo ni ejecutar el player para sustituir evidencia pendiente.

## Orden de trabajo

1. ATR-00: obtener un candidato de motor/parche realmente distinto y verificable.
2. ATR-01–13: integrar evidencia existente de procedencia, objetos retenidos y
   avisos; pedir al propietario solo el contrato o decisión que falte.
3. ATR-14: producir/revisar el bundle para una revisión limpia concreta, después
   de resolver los gates independientes de seguridad, CI y player confiable.

Todas las acciones están **PENDING**. Los números Dev/Ship describen identidades
de bibliotecas declaradas, no componentes retenidos ni licencias aplicables.

| Acción | Dev/Ship | Evidencia reutilizable | Pendiente concreto |
| --- | ---: | --- | --- |
| ATR-00 | independiente | MsQuicUpdateEvaluation1, MsQuicOwnerBaselineConfirmation1, NetworkSourceReview1 | Candidato vendor/parche, compatibilidad y seguridad QUIC + OpenSSL + curl. UE5.8.2/CL56702186 indicado es el mismo baseline, no una corrección. Conservar QUIC. |
| ATR-01 | 7/7 | StaticGapSourceReview1 | Contratos Epic/codecs/Oodle y alcance 9/13 frente a SDK9/16; avisos del fork libpas; UEJpeg/stb_image y Draco retenidos. Resolver elección aplicable MIT/Unlicense, no asignarla automáticamente. |
| ATR-02 | 3/0 | DevProfilerSourceReview1 | Términos/procedencia de ConcurrencyVisualizerNull, SuperLuminalNull y VSPerf; el sufijo Null no exonera el aviso. Evaluar solo distribución Development si procede. |
| ATR-03 | 3/3 | CompressionFontCoreReview1 | Avisos zlib/PNG, discrepancia de copyright PNG y elección FreeType/otros componentes realmente retenidos. No reutilizar el crédito actual como bundle completo. |
| ATR-04 | 3/3 | NetworkSourceReview1 | Parche/build/backend/features efectivos de SSL/crypto/curl; avisos OpenSSL AND SSLeay y copyright curl aplicables. La lista de avisos de versión no prueba CVEs ejecutables en el juego. |
| ATR-05 | 3/3 | TextWebSocketSourceReview1, Diagnostics1 | Avisos completos/per-file ICU/HarfBuzz/UCDN; alcance de libWebSockets modificado, excepciones y cumplimiento de fuente. No imponer disclosure del juego ni elegir términos por TPS antiguo. |
| ATR-06 | 3/3 | HashTangentTiffSourceReview1, Diagnostics1 | Rama CC0/Apache de BLAKE3; fuente/aviso de Mikk alterado; TIFF y zlib/turbojpeg transitivos retenidos. La técnica de tangentes es un ámbito separado. |
| ATR-07 | 5/5 | AudioVideoSourceReview1, Supplement1 | Build Opus/Speex efectivo, avisos separados API/root/resampler; VPX/WebM y alcance condicional de patentes. Revisar referencias IETF de Opus solo si aplican al candidato. |
| ATR-08 | 10/10 | ImageHttpSourceReview1, Supplement1 | Avisos nghttp2/JPEG/APV/Imath/OpenEXR/OpenJPH. Evaluar crédito IJG/TurboJPEG y componentes por separado; OpenJPH no queda cubierto por OpenEXR. No reparar licencias/copyrights antiguos. |
| ATR-09 | 12/12 | GpuDeclarationReview1, Supplement1 | Mantener candidatos MIT AGS/NVAPI raw-matched y alcance explícito de libs; obtener términos completos/build Intel; elegir alcance ITT; acreditar derechos del SDK DirectX exacto. No relicensear Aftermath ni aplicar MIT de headers a todo el SDK. |
| ATR-10 | 9/8 | CompanionImportBinding1 + recibos previos de cada DLL | Reutilizar las asociaciones AMD64/owners a oneTBB, Ogg/Vorbis, Aftermath, MsQuic, DirectML y PIX. Determinar miembros/objetos retenidos y alcance real; nombre importado no prueba versión DLL ni ABI. PIX es Development-only en estos índices. |
| ATR-11 | 3/0 | DevTailSourceReview1 | Detours4.0.1 frente a source/notice distinto; SDK DirectShow exacto/REDIST/marked sample/Media credit condicional; derechos Molecular Matters/Live++/Epic de JumpToSelf, no xxHash BSD. |
| ATR-12 | DLLs 21/13 | NativeFamilyCoverage1 y los 11 recibos familiares enlazados | Cerrar distribución/avisos/build de DbgHelp, MsQuic, Aftermath, Ogg/Vorbis, Vulkan, PIX, XAudio2, ONNX Runtime, Agility, DirectML y oneTBB. Separar proveedores/avisos package-wide de código realmente retenido. |
| ATR-13 | fuera de cuenta | [Mapa por identidad](M7_FONT_NOTICE_MAP.md), FontNoticeMap02, M7_FONT_ATTRIBUTION_REVIEW, ASSET_INVENTORY, Inventory/SourceMappings | 15 rutas/14 hashes font por paquete histórico, copyright/nameID0/13/14 y versiones vinculados;45 paths raw revalidados. Resolver aplicabilidad/avisos de8Roboto2011 sin13/14,2Roboto2017,2Noto, DroidFallback y Mono con prueba exacta separada; alias Bold no amplía TPS. Contenido/plantillas Epic, providers/transitivos y plataforma siguen fuera de este mapa. Owner PENDING; no inferir derechos por familia o DLL del sistema. |
| ATR-14 | candidato final | RELEASE_CHECKLIST + registros de revisión anteriores | Bundle final íntegro con bytes/procedencia/avisos/créditos aplicables y revisión del propietario para el candidato limpio exacto. Regenerar y verificar paquetes nuevos; no modificar los QA históricos. |

### Información que debe completar el propietario/revisor

Para cada acción registrar, sin credenciales ni copias de contratos privados:

- Responsable y fecha; identificación de candidato/revisión/configuración.
- Evidencia de procedencia y alcance de distribución: versión/build/contrato o
  referencias a documentos locales controlados, con hash cuando corresponda.
- Bibliotecas/objetos/providers/contenido incluidos y excluidos, con evidencia
  de enlace/cocción/staging al nivel de la afirmación. Un `.rsp` o JsonExport
  demuestra declaraciones; su presencia/ausencia no prueba ejecución o retención.
- Avisos, copyright, licencia elegida cuando existe elección, créditos y otras
  obligaciones aplicables; ubicación de cada resultado en el bundle candidato.
- Decisión `PENDING`, `NEEDS_EVIDENCE` o `REVIEWED_FOR_CANDIDATE`, con fundamento.
  La última solo es válida para el candidato/alcance indicado y no aprueba los
  demás gates. Una decisión del modelo no sustituye derechos del propietario.

No subir automáticamente acuerdos privados, fonts, SDKs, fuentes, volcados,
`ue.projectstore`, credenciales o logs sin revisión/redacción. No ampliar cambios
a servicios, remotos, seguridad de Windows o contratos por rellenar esta tabla.

## Criterio para el bundle final

ATR-14 queda OPEN hasta que el alcance completo esté sustentado y revisado,
los avisos/créditos correspondientes estén incluidos, y se verifique el contenido
real de nuevos paquetes Development y Shipping vinculados al candidato limpio.
El `NOTICES.txt` Shipping QA actual, 111 bytes/solo FreeType, no ha cambiado.
No copiar todos los avisos package-wide, sustituir licencias por versiones modernas
o eliminar notas históricas para simular cobertura; resolver primero el alcance.

La revisión de derechos no mitiga vulnerabilidades; la corrección de seguridad
no valida licencias. Este trabajo tampoco prueba CI aislado/protegido, firma,
player/input/save/load real, rendimiento o aprobación visual. Todos conservan
sus gates y evidencia independiente en [el checklist](RELEASE_CHECKLIST.md).
