# Entrada para Claude

Las instrucciones compartidas están en `AGENTS.md`. Si aún no está en contexto, léelo una vez y síguelo.

Lee `HANDOFF.md` para retomar la próxima tarea y actualízalo al cerrar tu bloque. `MODEL_WORKFLOW.md` contiene los prompts de relevo y la receta de tests dirigidos.

Para empezar rápido: lee primero el checkpoint compacto y la siguiente tarea de `HANDOFF.md`, comprueba `git status --short` / `git log -3 --oneline`, y marca esa tarea active antes de escribir. Conserva los cambios sin commit y los assets LFS del modelo saliente. No vuelvas a ejecutar gates ya aprobados si el código no cambió.

Carga solo la sección necesaria de `IMPLEMENTATION_PLAN.md` y del contexto Unreal; conserva los avances ya verificados. El trabajo de generación visual/modelos se entrega a GPT mediante la cola de assets del relevo.

Termina la respuesta con la marca `HANDOFF_READY` indicada en `AGENTS.md`.
