# Camp03 — integración funcional 7×6

2026-10-30. Primer flujo real verificado en privado, orientación inicial solamente.
Integración completa, adopción raíz y QA del player siguen abiertos.

## Decisión y alcance

El usuario eligió ampliar a 7×6 y ajustar la salida con pruebas. Se conserva la
escala natural de Camp03, sin rehacer geometría ni reducir las proporciones
humanas. Primero se integra en el GamePreview privado existente; no se cambia
todavía el DataAsset, mapa ni contenido de la raíz. La decisión no cierra los
gates de apariencia, navegación, rendimiento, seguridad o lanzamiento.

`ABarracks` resuelve la salida de edificios opt-in usando la huella efectiva y
la cápsula de la clase entrenada. Conserva el lado del SpawnOffset existente,
aumenta la distancia con margen de 50 cm, proyecta al navmesh y comprueba que la
proyección no vuelva al interior. Exige espacio libre con el perfil de colisión
de la cápsula y usa DontSpawnIfColliding. Un rechazo vuelve al mecanismo existente
de reembolso; no se crea otra autoridad de reservas, monedero o entrenamiento.
La rama legacy con PresentationMesh nulo conserva el comportamiento anterior.
La búsqueda vertical se centra en la base de la caja y se limita al step-height
real + 10 cm de tolerancia de navegación: no puede elegir un techo vecino.
El bootstrap del GameMode omite solo los marcadores decorativos generados cuyo
techo invadiría la huella del edificio final. No destruye estructuras del jugador
ni modifica los ocho marcadores de la rama legacy.

## Evidencia / procesos

- Root Build12 native 0; Build11 falló únicamente por include directo faltante
  de CollisionQueryParams, corregido sin relajar el validador.
- Build13 native 0: el caso fallback del test ahora establece explícitamente
  mesh nulo, sin exigir que el catálogo guardado siga siendo un prototipo.
- Root Test10 PID40116 native 0: cuatro pruebas de presentación aprobadas sin
  errores/advertencias; Content/Config/descriptor intactos. Esto no prueba aún
  la salida ampliada de Camp03: sus casos de entrenamiento cancelan la orden.
- `Artifacts/C018H_Camp03Integration1/Preparation01`: backups por hash de las
  fuentes/DLL/descriptor/PDA anteriores, snapshot de 193 archivos de contenido
  privado y del contenido/configuración raíz. Nueve archivos runtime actuales
  y el nuevo fixture Editor-only sincronizados, sin reimportar ni guardar assets.
- Private Build01 terminó6 por dependencia JSON faltante en el módulo de pruebas,
  no por el runtime. Build02 native0 corrige la dependencia privada de Json solo
  en el módulo Editor tests, también declarada en raíz. Backup de reglas previo.

Fixture: `Tools/ArtProbe/Native/CampBuildingIntegration.cpp`, copiado únicamente
al módulo Editor de pruebas privado. `Author` requiere opt-in explícito y calcula
centrado/apoyo desde los bounds reales de SM_CampBarracks, guarda únicamente el
PDA privado y produce recibo. `ProducerPIE` exige el asset real en el campamento
inicial y comprueba transform, footprint, slots PBR, entrenamiento, cápsula fuera
del edificio y camino completo de navegación. Ningún resultado se presume.

## Gates siguientes — abiertos

1. Author01 y Producer06 ya verifican el primer flujo real; no reautorizar,
   reimportar ni repetir esos outputs. Conservar el PDA privado adoptado.
3. Añadir/verificar las cuatro orientaciones, obstrucción/refund, movimiento real,
   colocación válida/inválida y selección mediante las autoridades existentes.
   Producer06 verifica desplazamiento completado, pero solo orientación inicial.
4. Vistas reales RTS/FPS del edificio funcional, regresiones pertinentes y cook/
   reachability del candidato funcional. GPU/cook anteriores prueban arte estático
   congelado, no este actor integrado.
5. Adopción raíz solo tras esos gates; luego gates M6/M7 restantes. Nada aquí
   resuelve MsQuic/OpenSSL/curl, QA Development/Shipping, CI aislado ni rights.

## Evidencia vigente — primer flujo real

| Operación | Resultado y límites |
| --- | --- |
| Author01, PID40948 | Native0/1cleanSuccess/stderr0. Solo cambia el PDA privado; otros192Content, Config/descriptor y todos los inputs raíz intactos. |
| Adopción privada | SM_CampBarracks Camp03 real, 5 slots PBR, 7×6; transformación calculada desde bounds: (-78.2,12,-199.5) cm, escala1. |
| Producer04, PID14540 | Native0; generación pagada real a Xlocal442 cm, cápsula42 cm, margen50 cm y ruta completa. Es la prueba de path, no de orden terminada. |
| Producer06, PID29892 | Native0/1cleanSuccess/stderr0. Mismo actor/asset real; comando canónico CommandMoveTo, 314.483 cm recorridos, 35.517 cm restantes, HasMoveCommand=false. Sin teleport, cámara forzada, dilación ni held-pose. |
| Root Build14 / Test11 | Native0 / cuatro pruebas de presentación cleanSuccess. RootContent/Config/descriptor intactos. No full-suite, IWYU ni Shipping nuevos todavía. |
| Closure | `Artifacts/C018H_Camp03Integration1/verified01.json`: diez fuentes runtime y reglas del módulo test iguales raíz/privado; fixture idéntico, snapshots y recibos vinculados, único delta de193Content es el PDA permitido, todos los jobs terminales. |

Fallos preservados, no maquillados: Producer01 buscaba por Actor.Owner, pero el
Pawn pasa a pertenecer a su AIController; el fixture ahora identifica exactamente
un Character nuevo por FObjectKey antes/después del Tick síncrono. Producer02/03
detectaron un path parcial: navmesh sobre una casa a150cm y recluta aZ248.15,
no un problema de usar centro vs pies. El código de generación colocaba casas
decorativas dentro/junto a la huella ampliada. Se corrigieron la selección del
nivel de suelo y la superposición de decoración; los siguientes procesos verifican
piesZ1.65 y path completo al sueloZ0. No se relajó IsPartial ni se forzó spawn.

Producer05 demostró salida física (>250cm/resto<60cm), pero ese criterio aún no
garantizaba fin de orden. Se preservó su snapshot y Producer06 endurece a resto<45
con HasMoveCommand=false, coherente con aceptación canónica40cm.

Código raíz de este bloque: Barracks.h/.cpp, Core/RomanEmpireGameMode.cpp,
RomanEmpireGameTests.Build.cs y el fallback explícito de BuildingPresentationTests.
Nuevo fixture versionable en Tools/ArtProbe/Native; wrappers y recuperación bajo
Artifacts/C018H_Camp03Integration1. No SourceArt, asset, mapa ni Config raíz nuevos.
Las guías UE mantuvieron definición/colocación/producción/monedero como únicas
autoridades y separaron herramientas/recibos JSON del runtime de Shipping.
