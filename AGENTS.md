# Instrucciones compartidas: GPT / Claude

Proyecto: RomanEmpireGame, Unreal Engine 5.8.2, Windows, PowerShell, VS Code + VS 2022 Build Tools. Objetivo vigente: implementar `IMPLEMENTATION_PLAN.md` completo y verificar sus gates. El usuario dirige los cambios de modelo.

## Inicio de cada sesión

1. Leer `HANDOFF.md`: estado actual, siguiente tarea, evidencia y decisiones pendientes.
2. Comprobar `git status --short` y `git log -3 --oneline`. Si hay diferencias con el relevo, inspeccionar solo los cambios afectados y reconciliarlo antes de escribir.
3. Leer únicamente la sección del plan y los archivos necesarios para la siguiente tarea. Consultar `.agents/ue-project-context.md` por sección cuando haga falta arquitectura o configuración.
4. Si es el primer cambio de modelo, consultar `MODEL_WORKFLOW.md`. Reutilizar lo aprendido después.

No reiniciar la auditoría ni regenerar el plan o el contexto desde cero.

## Reparto acordado

- **Claude:** C++, gameplay, arquitectura, UMG, scripts, tests, integración/importación de assets, builds, depuración y documentación.
- **GPT:** generación de imágenes, iconos, texturas, referencias visuales y creación de modelos 3D mediante herramientas realmente disponibles. Una imagen de un modelo no equivale a una malla utilizable.
- Para un asset pendiente, preparar su brief en la cola de `HANDOFF.md`, asignarlo a GPT y continuar tareas independientes de implementación. Claude integra y verifica el resultado en Unreal.
- Respetar una instrucción posterior del usuario que cambie este reparto. Ningún cliente hereda automáticamente herramientas, conexiones MCP, credenciales o skills de otro.

## Forma de trabajar

- Un solo modelo escribe en este worktree a la vez. Cerrar el bloque y guardar el relevo antes de cambiar.
- Usar búsquedas dirigidas (`rg`); evitar escaneos y lecturas repetidos. Mantener las actualizaciones al usuario breves.
- No rehacer módulos correctos. Una autoridad por dominio; UI por eventos/consultas; identificadores estables para datos y guardados.
- Preservar cambios ajenos. No hacer commits automáticos, `git add -A`, pushes, resets destructivos ni cambios de seguridad de Windows. Un commit local requiere petición explícita del usuario.
- Mantener assets Unreal en Git LFS y salida generada en `Artifacts/`/`Saved/`. Registrar procedencia y licencias en `ASSET_INVENTORY.md`.
- MCP Unreal: comprobar conexión real, solo Editor/loopback, sesión intencional, operaciones secuenciales, compilación/PIE inactivos durante edición de assets, sin servidor en Shipping.
- Skills globales son opcionales por cliente: sus fuentes están en la sección 6 del plan. Leer las que se utilicen; no reinstalar ni conectar servicios solo por cambiar de modelo.
- Ejecutar primero la prueba pertinente. Ampliar verificación según el cambio y los gates del milestone; no repetir gates aprobados si no hay cambios o incertidumbres que lo justifiquen.

## Comandos habituales

```powershell
.\Tools\validate_repository.ps1
.\BuildProject.ps1 -Action Editor
.\BuildProject.ps1 -Action Test -DisableFab
.\BuildProject.ps1 -Action IWYU
.\BuildProject.ps1 -Action Game -Configuration Shipping
git lfs fsck
git diff --check
```

`Test` ejecuta toda la suite; para una prueba concreta usar la receta de `MODEL_WORKFLOW.md`. Un build/cook aprobado no prueba el comportamiento del player.

## Cierre de cada bloque y antes del límite de contexto

Actualizar `HANDOFF.md` con: tarea/modelo, trabajo terminado, archivos cambiados (incluidos los no comprometidos), pruebas y resultados, decisiones, errores/procesos pendientes, siguiente tarea y siguiente modelo. Guardarlo al terminar cada bloque; no esperar al último token. Cambiar el plan solo si cambian estado, alcance o evidencia.

La última línea de la respuesta debe usar este formato, con valores reales:

```text
HANDOFF_READY | FROM=GPT | TO=CLAUDE | NEXT=C-001 | STATE=ready
```

En el relevo inverso usar `FROM=CLAUDE | TO=GPT`. `HANDOFF_READY` significa que el contexto está guardado; no significa que el milestone o el objetivo completo estén terminados. Si queda una operación en marcha, usar `STATE=waiting` y guardar su PID/handle, log y forma de observarla. El siguiente cliente comprueba ese mismo proceso antes de iniciar otro; los handles internos no se transfieren entre clientes.
