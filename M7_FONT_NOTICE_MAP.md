# Mapa técnico de avisos de fuentes — revisión de derechos OPEN

2026-10-21. C-018H_FontNoticeMap / ATR-13. No es licencia, dictamen legal,
aprobación del propietario ni bundle final de avisos.

## Alcance y evidencia

Se reutilizaron los recibos existentes de `C018H_LocalQA1` Development/Shipping,
no un candidato de release actual. Cada paquete contiene 15 rutas de fuentes
raw con 14 SHA256 distintos. Los dos conjuntos y sus metadatos coinciden.
`Engine/Content/Slate/Old/Tiles/Roboto-Bold.ttf` duplica exactamente los bytes de
`Engine/Content/Slate/Fonts/Roboto-Bold.ttf`; no es una fuente número15 distinta.
La ruta Old/Tiles está fuera de la ubicación declarada por los TPS de Fonts:
la identidad de bytes no amplía automáticamente esa declaración de directorio.

`Artifacts/C018H-FontNoticeMap1/FontNoticeMap02.json`, SHA256
`3e57c9409b4c703f4e8eb813afa43a10491ea3ba5f4ec82484575a19226dcc38`,
conserva por identidad las rutas, bytes, versiones, todos los registros nameID0,
13 y14 con plataforma/encoding/idioma, documentos candidatos, evidencia y gaps.
No normaliza, elimina ni reescribe el texto histórico. Las URLs embebidas son
datos no confiables: no se siguen ni se ejecutan. `license_determination=null`,
`rights_review=pending_owner` y `final_notice_approved=false` en todas las filas.

El generador verifica los cinco recibos originales contra hashes fijados,
vincula los Pak/recibos de extracción/manifiestos y vuelve a comprobar45 rutas
de fuentes:15 instaladas y15 extraídas por configuración. Comprueba además
los hashes de tres TPS y dos documentos instalados; no los modifica ni realiza
una nueva extracción, parseo SFNT, descarga, build, cook o ejecución del juego.
Los inputs exactos y sus hashes permanecen en el JSON y en la
[revisión original](M7_FONT_ATTRIBUTION_REVIEW.md).

## Identidades exactas

Todas las rutas siguientes tienen prefijo `Engine/Content/Slate/Fonts/`, además
del alias Bold indicado arriba. La columna C identifica el texto exacto nameID0
de la tabla posterior; V identifica la versión observada. L identifica solamente
la evidencia documental descrita después, NO una licencia determinada.

| Archivo | SHA256 raw | C / V / L |
| --- | --- | --- |
| DroidSansFallback.ttf | `05d71b179ef97b82cf1bb91cef290c600a510f77f39b4964359e3ef88378c79d` | C1 / V1 / L1 |
| DroidSansMono.ttf | `793a63429da92cb9be35ca3fca080a66b3c74915fc24b778dcc35d78f465e403` | C2 / V2 / L2 |
| NotoNaskhArabicUI-Regular.ttf | `f3878be872cc1d4ad9bd63535db480e3c5752c6313842656208872275cb32226` | C3 / V3 / L3 |
| NotoSansThai-Regular.ttf | `f8997f85ec7aea91a6b077019f76b80145568e22d3056942a28f298510a8434d` | C4 / V4 / L3 |
| Roboto-Black.ttf | `c78ce438e9d91ea8c16341aaf2c6cf07f9c347eb1130bd922796ad4fb78fe885` | C5 / V5 / L4 |
| Roboto-BlackItalic.ttf | `5e7928c14bbddea37ec9ba2453f00bd8f4d73c42c20528e42abc5b89d2732db1` | C5 / V5 / L4 |
| Roboto-Bold.ttf (+Old/Tiles alias) | `24571503140760240924dcd1238f77e7cd0454c0d8b0793990cefa2fad71471f` | C5 / V5 / L4 |
| Roboto-BoldCondensed.ttf | `2c9245c0246a9c745793c7eaa9cc1fc964a12570009031ea0223115bec6e9426` | C5 / V5 / L4 |
| Roboto-BoldCondensedItalic.ttf | `1117eab9f490123cf8a656fa936a4c9a7e4918b88ae576d64d3d79507096f7b9` | C5 / V5 / L4 |
| Roboto-BoldItalic.ttf | `165a6d02fd8973703787feeef5319b7fd8cb3f665a3c1e702ef0ca49701547c9` | C5 / V5 / L4 |
| Roboto-Italic.ttf | `470123c6fb5d6c0005431c0f5390742497c55087361a7636b7e82eb35635ded7` | C5 / V5 / L4 |
| Roboto-Light.ttf | `a6d343d425bc38db90152fa06058b1c7391eca9264f334ef65c1ce175085c6f6` | C6 / V6 / L5 |
| Roboto-Medium.ttf | `e35252aa3dc2e84e9d7211586fee9aede2a426d3230c8b131881d985f16ff836` | C6 / V6 / L5 |
| Roboto-Regular.ttf | `16466ef65064e6f3885a6d2806b8949ac1ac38b524dd0cf8fc96565eb4cc28e8` | C5 / V5 / L4 |

| C | Texto exacto nameID0, sin sustituir años ni titulares |
| --- | --- |
| C1 | Digitized data copyright Google Corporation © 2006 |
| C2 | Digitized data copyright © 2006, Google Corporation. |
| C3 | Copyright 2013 Google Inc. All Rights Reserved. |
| C4 | Copyright 2016 Google Inc. All Rights Reserved. |
| C5 | Font data copyright Google 2011 |
| C6 | Copyright 2011 Google Inc. All Rights Reserved. |

| V | Valores distintos observados en nameID5 |
| --- | --- |
| V1 | `Version 2.55b` |
| V2 | `Version 1.00`; `Version 1.00 build 107` |
| V3 | `Version 1.06 uh` |
| V4 | `Version 2.000;GOOG;noto-source:20170915:90ef993387c0; ttfautohint (v1.7)` |
| V5 | `Version 1.00000; 2011` |
| V6 | `Version 2.137; 2017` |

## Evidencia documental por grupo y acción pendiente

- L1, Droid Fallback: nameID13 declara Apache2.0 y14 contiene una URL de Apache.
  El TPS instalado `DroidSans.tps` redirige a un archivo ausente. Estos recibos no
  incluyen prueba upstream por bytes para Fallback: no heredar la de Mono.
- L2, Droid Mono: nameID13 conserva el lenguaje histórico Ascender de cinco PC
  y14 su URL EULA. Separadamente, `DroidMonoUpstream1.json` vincula exactamente
  los78,296 bytes al commit AOSP `54b6cfa9a9e5b861a9930af873580d6dc20f773c`,
  `data/fonts/DroidSansMono.ttf`. README/NOTICE/marcador del mismo directorio
  declaran Apache2.0. Es evidencia más fuerte que un nombre de familia, no una
  eliminación del texto Ascender ni aprobación de obligaciones/distribución.
- L3, ambos Noto: nameID13 declara OFL1.1 y14 su URL. `Noto.tps` señala
  `Source/ThirdParty/Licenses/Noto_License.txt`, SHA256
  `1beea52b7cf6b8f5fa3120f3b6fc4bd5828037bdbe513624ea55100876654c18`.
  La declaración de directorio no fija cada face/version/hash upstream.
- L4, ocho Roboto2011 distintos: nameID13/14 AUSENTES, no licencia inferida.
  `Roboto.tps` señala `Source/ThirdParty/Licenses/ROBOTO_License.txt`, SHA256
  `813aec82b71fe6fc927f1f9f5c08c5a098785b0a89e3a2e76697aa4662e5f666`.
  Contiene un header2012 y texto Apache2.0; el TPS2016 no es un inventario de
  hashes de fonts2011. Resolver versión/procedencia/acuerdo exactos; no aplicar
  automáticamente términos de los siblings2017. Mantener aparte el alias Bold.
- L5, Light/Medium2017: nameID13 declara Apache2.0 y14 su URL; mismo documento
  instalado Roboto anterior. No confundir copyright2011, versión2017, TPS2016
  y header2012 como identidad de una única entrega. La codificación legacy del
  documento instalado se conserva, sin recodificar ni sustituir la licencia.

README/NOTICE de Mono y sus hashes completos están retenidos como evidencia en
el JSON, no copiados como bundle de release. Los16 antiguos404 de Roboto siguen
acotados a las rutas de dos repositorios ya consultados; no se repitieron ni
prueban ausencia global de fuente o licencia.

Para cada identidad, el revisor/propietario debe registrar candidato exacto,
fundamento contractual aplicable, texto/ubicación de avisos y decisión. La lista
anterior separa pruebas y gaps; no afirma que todo gap requiera un hash upstream
como requisito legal universal. No hay nuevos derechos aceptados por el modelo.

## Verificación y siguiente gate

12 tests dirigidos PASS: igualdad Dev/Ship, fuentes instaladas, rutas repetidas o
ajenas, metadatos contradictorios en un alias, campos inválidos/ausentes, no
herencia de licencia Roboto, vínculo exacto Mono/documentos y no extensión a
Fallback. Las URLs se conservan como datos. No prueba revisión legal humana.

```powershell
py -m unittest discover -s Tools/tests -p test_font_notice_map.py
py Tools/font_notice_map.py --output FontNoticeMap03.json
```

La segunda línea es receta opcional, NO ejecutada ni necesaria al cambiar de
modelo. Exige nombre NUEVO y datos originales intactos. El recibo01 histórico
precede un cambio de nombre de clave;02 corresponde al helper actual, SHA256
`7d91cbaff40a4639f4f268bc632e460362b23178c3a25668609b4f32f052c4ab`.
Tests SHA256`c9a0981d327a8a11bd6cc1e93a2cc16774d637f2d4a7607d0f3bc16e3cf33cd6`.

`verification01.json` vincula salida/helper/tests y resultado observado. Validación
del repositorio PASS con0avisos M1 abiertos; diffcheckPASS (warningsCRLF ajenos
preexistentes). Comprobación cruzada de14hashes/copyright/versiones documentados
y tres identidades del reciboPASS. No nueva suite nativa/fullPython para este
bloque de lectura/agrupación y documentación.

ATR-13 sigue PENDING, también para contenido Epic/providers/plataforma fuera de
este mapa. ATR-14 requiere el candidato limpio exacto y todos sus gates. El
NOTICES.txt Shipping QA de111bytes permanece intacto: no se modificaron Pak,
fonts, documentos originales, motor, assets o configuración. M7/M8 siguen OPEN.
