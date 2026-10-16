# C018H — QA local del player: preparado, NO ejecutado

VigenteOct15 21:48 local: licencia POSTBOOT verificada offline, captura
VM_3DOff_LicenseResult2.png/sentinels completos: EnterpriseEval,LicenseStatus1,
128112min. Transporte COM lento probado; consulta única terminada/worker0,
consola idle. Mismo backend33044/3DOFF/aislado; resume00:36:34UTC tras HostSuspend
observado, no todos los cuelgues explicados. Perfil acelerado/player no calificados:
restaurar3Don con apagado normal y probar con host despierto, no otro forzado
sin autoridad. QA-01–10 PENDING; HANDOFF/VM_3DOff_LicenseVerification1.json vigentes.

VigenteOct15 21:09 local / Oct16 00:09UTC: recuperación autorizada completada,
poweroff0/modificación temporal3DOFF0/arranque único0; backend nuevo33044, aislado.
Primer modify falló por lock liberándose, no repitió poweroff ni start; continuación
solo desde poweroff verificado. Desktop/WinR y DxDiag Pantalla por COMmouse
observados; driver7.2.20.25154/WDDM1.3/VRAM0/DDI12/niveles visibles≥11_0 en3DOFF
NO califican aceleración/player ni baselineDX12. Pausa heartbeat8.85s persiste,
sin arreglo definitivo/causa demostrados. Licencia CIM postboot y QA-01–10 pendientes.
HANDOFF/VM_3DOffRecovery1.json; cerrar Run detrás/DxDiag antes de nueva entrada,
calificar perfil acelerado antes de juego; no nuevo ciclo sin ventana acordada.

Vigente23:53UTC: owner informa cuelgue. Log nuevo registra pausas/heartbeat
recuperado652s; captura muestra comando parcial, ninguna consulta/QA comprobada.
No causa atribuida a3D/Hyper-V. Recuperación forzada SOLO VM + prueba temporal
3DOFF propuesta, NO autorizada; no apagar/reiniciar/cambiar3D sin respuesta.
Prueba sin3D sería diagnóstico de UI, no calificación del player; M7 OPEN.
HANDOFF/VM_OwnerHang1.json; no más teclas ni petición de echo QA mientras cuelga.

Vigente23:47UTC: Inicio abre según owner; PowerShell visible en captura nativa
actualizada23:42 y client23:46. Consulta readonly enviada una vez/worker terminal0,
sin sentinel/resultado guest comprobado; no inferir CIM/dxdiag/QA de transporte0.
Owner prueba echo QA manual para validar teclado/foco; no repetir payload ni
pedir otra vez prueba de mouse. Sin player/reset/red/security; HANDOFF/recibo OwnerUI.

Actualización23:30:06UTC: capturas idénticas tras WinR/CtrlAltDel nativos0,
sin consola/resultado guest verificados; RunLevel3/CPU vivo no prueba UI o API.
Pregunta owner sobre Inicio con mouse pendiente. Helper de CIM nunca ejecutado,
DISABLED; sin dxdiag/licencia postboot/player. No más teclas ciegas/reset/seguridad.
HANDOFF/VM_PostbootInput_20261015_2330.json guardan evidencia; QA-01–10 PENDING.

Actualización23:26:40UTC: escritorio confirmado/captura _3.png, RunLevel3,
running/aislamiento verificados0. Boot terminado, no player. NEXT driver/dxdiag
featurelevels/licencia offline; bienvenida/RunLevel1 siguientes son históricos.

Actualización2026-10-15 23:25:51UTC: usuario autorizó nuevo arranque; startvm
GUI una vez0/query0, mismo UUID running y aislado, backend32772. Windows muestra
«Te damos la bienvenida»; no interacción/credenciales ni player ejecutado.
Additions7.2.20/RunLevel1 no califica driver/API/licencia. HANDOFF registra
operación terminal y capturas; siguiente observar escritorio y verificar postboot.
Los estados apagada/consentimiento de arranque pendiente debajo son históricos.

Estado vigente2026-10-15 03:10:37UTC (00:10 local): apagado forzado SOLO
RomanEmpireQA autorizado por usuario, comando nativo EXIT0 y postconsulta EXIT0
confirman poweroff del UUID90bafe9d-68f9-4d02-8521-5f66a6a60b83. No reiniciada,
sin operaciones pendientes; aislamiento/VDI/DVD conservados y host sin cambios.
VM_ForcedPoweroff1.json/HANDOFF guardan evidencia. Antes de arrancar confirmar
intención owner; luego verificar escritorio/dxdiag/licencia. Posible pérdida de
estado sin guardar, integridad no comprobada. QA-01–10 siguen PENDING.

## Historial anterior al apagado (no estado actual)

CierreUTC00:23: watcher terminado0/28392gone, VM31452 aún viva/Reiniciando,
no desktop/driver/API/player/postboot license verificados. Owner debe decidir
esperar o autorizar apagado forzado SOLO VM/riesgo estado guest no guardado;
ninguna recuperación forzada emitida. Footerwaiting NO es monitor aún activo.

LatestUTC2026-10-15/localOct14: offline Oracle GuestAdditions7.2.20 ISO checksum
and guestEXE Authenticode/hash verified; normal wizard/Direct3D checked reached
Completing, requested normal guest reboot00:12:17UTC. GUI remains Reiniciando,
same VM31452alive/nativeVersion7.2.20r175154/RunLevel1, NOT desktop/3D/API ready.
Watcher/log/HANDOFF/VM_GuestAdditionsClosing1 track actual pending operation;
no force reset without owner, no duplicate installer or reboot on timeout.
WindowsISO replaced by AdditionsDVD47612f43... guest-only; original retained.
Isolation and protections intact, all QA-01–10 PENDING; after desktop verify
dxdiag/actual driver/licensing before existingarchives -dx11. No QA claim from
installer success. Earlier uninstalled-driver notes are historical.

Vigente al cierre2026-10-14: evaluación Windows ACTIVADA por Windows durante
NAT temporal autorizado65.19s, aislamiento restaurado NIC1null/cableoff/NIC2–8none.
CIM OFFLINE23:51:21UTC VM_NATStatus1/sentinel: LicenseStatus1 Licensed,129594min
restantes(~90d), no claves/cuenta/rearm/reloj/contratos por agente. Helper2 nunca
ejecutado/deshabilitado; no más NAT necesario. VM31452running, driver3D/API y todos
QA-01–10 PENDING, último BasicDisplay no calificación. VM_ActivationClosing1.json
y HANDOFF preservan evidencia; datos de licenciaHOLD debajo HISTÓRICOS.

Vigente2026-10-14: VM ya llega al escritorio, Winver26H2/26300.9457; no sigue
esperando región/OOBE. Pero CIM readonly confirma EnterpriseEval Notification5,
0xC004F009/gracia0, y solo Microsoft Basic Display10.0.26100.1. NO QA aprobado
ni licencia/driver3D listos. Capturas/consultas terminales y PID36464 en HANDOFF,
VM_DesktopQualification1.json; datos OOBE posteriores son históricos. Pedir
autorización antes de NAT temporal/activación oficial, no rearmar ni cambiar reloj;
aislar antes de player. GuestAdditions/driver/API real y todos QA-01–10 PENDING.

2026-10-12, America/Montevideo. El usuario confirmó disponer de un entorno QA
confiable y aclaró: «usa esta pc o una VM algo que pueda usar probando aca».
Esto habilita preparar una ruta local; no acredita que exista una VM, que haya
pasado QA, ni autoriza cambiar la seguridad del host, comprar licencias o
transferir paquetes a un destino no acordado. Actualización: «montemos la VM dale»
autoriza preparar/instalar virtualización local; estado y prerrequisitos nuevos
en [VM_SETUP.md](VM_SETUP.md). ISO verificada/adjuntada, host reiniciado y Windows
OOBE de país/región observado2026-10-14 17:49UTC en misma VM (tras poweroff real,
un único arranque nuevo verificado0; PID12900 actual). Configuración personal,
condiciones/cuenta, guest listo/driver/DX12 y todos los viajes QA siguen pendientes.
No inferir licencia ni instalación terminada de OOBE. Preservar VM running/NICoff,
acordar vía legítima si OOBE requiere red; no duplicar arranque ni aceptar términos
por owner; observer PID/log y siguiente interacción en HANDOFF.

## Entorno observado y decisión pendiente

Consultas locales de solo lectura: Windows EditionID `Core` (Home),25H2,
31.4GiB RAM,24 procesadores lógicos; HypervisorPresent=true y vmcompute Running.
VirtualMachinePlatform InstallState=1. Esto NO prueba una VM Windows disponible.
Los flags Win32_Processor devolvieron false bajo ese hipervisor; no inferir que
BIOS/virtualización estén deshabilitados ni cambiar firmware.

Get-VM no disponible; no VBoxManage/vmrun en las tres rutas convencionales
comprobadas. No WindowsSandbox/WindowsSandboxClient/wsb en System32 ni paquete
WindowsSandbox del usuario actual. Es una comprobación acotada, no un inventario
global de VMs o instalaciones. Microsoft indica que
[Sandbox no admite Windows Home](https://learn.microsoft.com/en-us/windows/security/application-security/application-isolation/windows-sandbox/).
No preparar un .wsb como si fuera ejecutable aquí ni usar parches para habilitarlo.

Siguiente decisión del propietario: instalación de un hipervisor compatible con
este host y Windows invitado con licencia/ISO aprobada, o un proceso legítimo de
firma/confianza del paquete. Setup local autorizado ahora, no firma ni compra de
licencia ni bypass de seguridad. La GPU/API gráfica del invitado debe comprobarse:
una VM que arranca no garantiza que ejecute este juego Unreal. FPS en una VM no
sustituye el gate de rendimiento en hardware de referencia.

Comprobación dirigida del perfil gráfico: Config/DefaultEngine.ini:28 establece
DefaultGraphicsRHI_DX12; no hay entradas D3D11TargetedShaderFormats explícitas en
ese archivo. No inferir ausencia de shaders DX11 en el cook o forzar -d3d11 sin
verificar su disponibilidad. Host: Intel Graphics + NVIDIA GeForce RTX5070Ti
Laptop GPU, drivers32.0.101.8331/32.0.15.9201. Esto NO es la GPU/API del invitado.
Elegir/instalar una VM sin comprobar soporte del perfil DX12 podría no permitir
QA; resolver esa compatibilidad antes de cualquier instalación o nuevo cook.

El host ya bloqueó el player unsigned mediante Code Integrity. No reintentar aquí,
desactivar SAC/Defender/Code Integrity, agregar exclusiones ni alterar políticas.
Una VM no elimina los HOLD de MsQuic/OpenSSL/curl ni aprueba distribución.

## Entradas existentes: reutilizar, no recompilar

Paquetes DIRTY LOCAL QA, revisión base
`0726de25b05f84736b421480a1f1d633f87ac29c`, con cambios no comprometidos.
[Review de paquetes](M7_LOCAL_QA_PACKAGE_REVIEW.md) conserva el cook, contenedores
y demás verificaciones; no repetirlos por preparar esta prueba.

| Configuración | Carpeta bajo Artifacts/Packages/C018H_LocalQA1 | Archivos | Manifest SHA256 |
|---|---|---|---|
| Development | Development/Windows |48|28f9b94128a5fa18c305e20dee808711f29c543f6e3c610f41a43cd1fbf99859|
| Shipping | Shipping/Windows |30|4773d3fa6483f1c58f3df1625da7f94134110955b737422b18bb402cd8b315d0|

Manifests hermanos de Windows: `archive-sha256-Development.json` y
`archive-sha256-Shipping.json`. Ambos hashes/conteos/revision/dirty se comprobaron
de nuevo en este bloque, exit0. El bootstrap de ambas configuraciones se llama
`Windows/RomanEmpireGame.exe`; no confundirlo con el Game EXE de Binaries/Win64.

Game EXEs ya verificados contra esos manifests, sin nueva ejecución:

- Development: `RomanEmpireGame/Binaries/Win64/RomanEmpireGame.exe`,
  SHA256 `63330b9e509f080f82041f8b1738d4a94039f4fd6572eeffbc3dab64c78bc39c`.
- Shipping: `RomanEmpireGame/Binaries/Win64/RomanEmpireGame-Win64-Shipping.exe`,
  SHA256 `3e3da2b7cab7a869479b4c62134f6ada51c0e5e1086e57749195a56c15db9c2a`.

Ambos Game EXEs NotSigned. Eso no determina por sí solo la política del invitado.
Si el entorno aprobado también los bloquea, detener y registrar el bloqueo:
NO sortearlo ni presentar un exit del launcher como prueba de gameplay.

## Preparación, únicamente después de aprobar el entorno

Calificación2026-10-13: [VM setup](VM_SETUP.md) y ShaderQualification1.json
confirman metadata/libraries SM5 y targets D3D11+D3D12 en AMBOS archives existentes;
10containerhashes manifest-bound/31inputs estables. Antes de decidir un cook QA
separado, calificar -dx11 solo en guest aprobado con driver adecuado. Registrar
esa corrida como DX11/VM, separada de defaultDX12 y rendimiento hardware; ninguna
fila se aprueba por inventario, hashes o flags. No lanzar en el host bloqueado.

1. Registrar anfitrión/invitado, Windows, hipervisor, GPU/driver/API disponible,
   RAM/CPU asignadas, resolución/DPI y persona responsable. No registrar cuentas,
   credenciales ni identificadores personales innecesarios.
2. Usar un invitado dedicado, sin cuentas/tokens de desarrollo ni datos personales,
   snapshot inicial y red desconectada para el viaje offline. No usar la VM como
   runner CI por defecto. No modificar la seguridad del host o invitado.
3. Acordar destino y alcance de copia. Solo el paquete de cada configuración y
   sus sidecars aprobados, nunca repo/engine/caches/projectstore/credenciales.
   Los archives actuales pueden incluir símbolos: no distribuirlos sin revisión.
   Si se acuerda una cohorte reducida, crear manifest NUEVO y preservar originales;
   no ocultar una exclusión como si el manifest original siguiera cubriéndola.
4. Verificar el hash del manifest contra la tabla y TODOS los archivos recibidos
   contra sus bytes/SHA256 antes del arranque. Cualquier mismatch: STOP, conservar
   evidencia. Mantener la estructura Windows completa; no copiar solo el EXE.
5. Ejecutar Development y Shipping por separado, con perfiles/saves aislados.
   Usar el bootstrap del paquete aprobado mediante una sesión gráfica visible;
   no depender de Editor, MCP, consola de tests ni servicios del proyecto.
   Cualquier prerequisite ausente requiere identificarlo y aprobar su instalación;
   no descargar/instalar redistributables automáticamente.

## Prueba real: repetir para Development Y Shipping

Usar controles y botones que realmente muestra el juego, no teclas inventadas
ni fixtures que fuerzan bajas/victoria. Documentar secuencia reproducible y
resultado real; PENDING/NOT_RUN no significa PASS.

| ID | Acción del jugador / observación exigida | Dev | Ship |
|---|---|---|---|
| QA-01 | Arranque visible offline, menú usable, sin dependencia de Editor/MCP/test server. Registrar PID/procesos y listeners del proceso, no solo config. |PENDING|PENDING|
| QA-02 | Entrar en campaña; seleccionar región/ejército, comprar/asignar y entrenar. Cancelar antes de generar unidad: reembolso completo y sin unidad fantasma. Guardar cantidades/IDs visibles. |PENDING|PENDING|
| QA-03 | Guardar campaña manual y avanzar turno real. Describir cambios de recursos/producción/IA y lanzar una batalla soportada. |PENDING|PENDING|
| QA-04 | RTS: selección, orden de movimiento/ataque y UI/foco reales; entrar/salir de posesión dos veces. Confirmar cámara/selección/IA restauradas y tiempo real, sin cámara lenta. |PENDING|PENDING|
| QA-05 | Resolver una batalla por juego normal; resultado real, Continue y consecuencias en campaña aplicadas una sola vez. Registrar victoria/derrota observada; otra rama no probada queda NOT_RUN. |PENDING|PENDING|
| QA-06 | Guardar después de batalla, cambiar el estado con un turno y cargar el guardado. Comparar recursos/ejércitos/propiedad/estado visible. Si IA inicia otra batalla, resolverla antes de cargar; no cargar durante combate. |PENDING|PENDING|
| QA-07 | Preferencias: aplicar/cancelar/restaurar defaults, remap válido y conflicto/rechazo, volumen, sensibilidad y texto200%. Pausa/modal/foco/resume, incluido soltar una tecla de movimiento mantenida. |PENDING|PENDING|
| QA-08 | Cerrar y relanzar el player de esa configuración en el mismo perfil aislado: preferencias/remap y guardado sobreviven; cargar y continuar. No confundir persistencia del proceso con persistencia del invitado. |PENDING|PENDING|
| QA-09 | Revisar ventanas/DPI previstos, navegación teclado/puntero, legibilidad de facciones, audio y texto200% sin controles inaccesibles. Registrar casos realmente vistos y faltantes. |PENDING|PENDING|
| QA-10 | Cierre limpio, errores/crashes y servicios/listeners propios antes/durante/después. Sin listener del juego no prueba ausencia de servicios de toda la máquina. |PENDING|PENDING|

Reglas aprobadas adicionales: moral0→retirada; captura por ocupación sin enemigos
vivos. Ejercitarlas cuando haya un caso reproducible con inputs normales, anotando
el escenario exacto; si no se observa la condición, NO marcarla aprobada. Tests
Editor existentes no sustituyen QA-01–10 ni pruebas de player pendientes.

## Evidencia por corrida: completar, no aprobar por plantilla

Crear un resultado NUEVO bajo `Artifacts/TrustedPlayerQA/<id-unico>/`, nunca
sobrescribir una corrida fallida. Este directorio está previsto, NO creado.
Fecha/responsable/entorno/configuración, revision+dirty, hashes de manifest/Game,
prerequisites y aislamiento verificados, PID/arranque/cierre, pasos/inputs reales,
QA-01–10 con PASS/FAIL/NOT_RUN y evidencia, repro de fallos, path real de saves,
preferencias antes/después de relanzar y alcance de captura de listeners.

Adjuntar solo capturas/logs/receipts pertinentes y redactados autorizados. No
recolectar carpetas de usuario completas ni subir dumps/logs automáticamente.
Shipping puede no tener los mismos logs que Development: no inventar evidencia.
Guardar/exportar evidencia aprobada antes de descartar la VM/snapshot.

El QA de estos paquetes dirty, aunque pase, NO cierra clean-candidate/CI,
rendimiento objetivo, firma, security HOLD, derechos ni avisos finales. Esos gates
siguen en [checklist](RELEASE_CHECKLIST.md) y [gates externos](M7_EXTERNAL_GATES_REVIEW.md).
