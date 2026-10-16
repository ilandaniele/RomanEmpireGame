# VM local de QA — diagnóstico autorizado sin 3D, calificación Unreal pendiente

2026-10-12, America/Montevideo. El usuario pidió «montemos la VM dale» después
del protocolo QA. Autoriza preparar/instalar virtualización local; NO cambiar
SAC/Defender/Code Integrity, desactivar VBS/Hyper-V, comprar Windows ni aceptar
automáticamente contratos del invitado. Ningún player ha sido ejecutado.

## Estado real de este bloque

VigenteOct15 21:48 local / Oct16 00:48UTC: licencia POSTBOOT verificada offline
en VM_3DOff_LicenseResult2.png/QA_LICENSE_BEGIN-DONE: Windows EnterpriseEval,
LicenseStatus1,128112min (~89d), sin activación/claves/red/reloj/rearm/contratos.
Nuevo helper SendQaText_3DOff1.ps1 prueba texto completo con COM PutScancodes,
conteo1/código, ReleaseKeys/Shift/dwell20ms+80ms y hyphenLAA35; guard PID33044 y
logs create-only. Wrapper readonly InvokeQaLicenseProbe1.ps1/payload498chars,
observer42600 terminal0 y resultado guest completo, no reutilizar intento License1.
Consola queda idle; sin monitor propio. Cleanup nested-finally endurecido después
del probe/parsePASS; TestQaTextCleanup1.ps1 cinco casos fakes PASS sobre finally
real extraído por AST (sin VM/COM), hash actual en recibo, no hash preejecución.
VM sigue3DOFF/aislada/RunLevel3, StateChangeTime00:36:34.609UTC por HostResume:
log muestra HostSuspend00:25:37.428792→HostResume00:32:57.212528elapsed; eventos
Windows42/107 corroboran sleep/resume con tiempos no exactamente alineados.
Explica una pausa, no todos los lags ni causa3D probada. No cambiar seguridad,
WHP/VBS/reloj/energía; mantener PC despierta en QA. C:20.58GiB libres observado.
NEXT apagado NORMAL/restaurar3Don y calificar perfil acelerado antes de player;
no repetir apagado forzado/NAT/activación ni considerar3DOFF QA Unreal aprobada.
HANDOFF/VM_3DOff_LicenseVerification1.json son estado actual; lo siguiente histórico.

Vigente2026-10-15 21:09 local / Oct16 00:09UTC: owner autorizó recuperación.
Apagado forzado SOLO VM0; primer modify falló por sesión liberándose y script1
se detuvo sin start. Verificados poweroff/backend viejo ausente; continuación
modify3DOFF0/startGUI único0/postquery0, backend33044/creation00:03:36.1861210UTC,
running desde00:03:44.229UTC. Config/VDI/DVD/isolation/security preservados salvo
accelerate3d OFF temporal. No repetir helpers create-only ni otro ciclo automático.
Escritorio y WinR respondieron; DxDiag offline (NO WHQL) Pantalla abrió por mouse
COM. Reporta VirtualBox Graphics Adapter(WDDM),7.2.20.25154/WDDM1.3/DDI12/niveles
visibles12_1/12_0/11_1/11_0/10_1...,VRAM0,Direct3DEnabled; es reporte3DOFF, NO
aceleración hardware/player probado. Nuevo heartbeat8.85s muestra pausas persisten;
no causa ni estabilidad demostradas. Licencia postboot pendiente, sin juego.
Current DxDiag foreground/Run detrás; cerrar ambos por botones verificados antes
de otra entrada. HANDOFF/VM_3DOffRecovery1.json/logs/capturas vigentes; notas de
permiso de recuperación pendiente debajo HISTÓRICAS. No restaurar3D/reiniciar sin
nueva ventana acordada; calificar perfil acelerado antes de Unreal.

### Canal de mouse comprobado en esta sesión

Sin credenciales ni portapapeles: COM instalado `VirtualBox.VirtualBox`7.2.20,
`FindMachine` del UUID exacto y verificar Name/Id; crear `VirtualBox.Session`,
`LockMachine(session,1)`, comprobar `session.Console.Mouse.AbsoluteSupported`.
`PutMouseEventAbsolute(236,143,0,0,1)` y misma coordenada botones0 cambió DxDiag
a Pantalla. Siempre `UnlockMachine()` en finally para la sesión propia. Coordenadas
son de captura guest1024x768; verificar pantalla actual antes de click, no reutilizar
coordenadas sin verla ni inferir éxito por retorno COM. SDK webfetch402; firmas
`LockMachine(ISession,LockType)` y `PutMouseEventAbsolute(int,int,int,int,int)`
corroboradas en metadatos del COM instalado, no se instaló ni modificó SDK/servidor.

Vigente23:53UTC: owner «se colgo». Query0 misma VM/backend32772 running/aislada,
VBoxSVGA256MiB/3Don/RunLevel3; VM_OwnerHang1.png muestra comando parcial sin resultado.
Log marca heartbeat recuperado tras652s y catch-up lag672s: retrasos, no causa probada.
No más teclas/reset/poweroff/3D cambiados. Owner debe autorizar nuevo apagado forzado
SOLO VM y prueba temporal3DOFF si la desea; riesgo estado sin guardar. No sustituye
calificación Unreal/DX11/DX12 ni habilita ejecutar juego sin driver/API aptos.
Documentación primaria [Oracle7.2 Troubleshooting](https://docs.oracle.com/en/virtualization/virtualbox/7.2/user/Troubleshooting.html)
describe posible conflicto Hyper-V, NO prueba de esta causa. No aplicar su workaround
de desactivar WHP/VBS/seguridad del host. [Guest Additions7.2](https://docs.oracle.com/en/virtualization/virtualbox/7.2/user/guestadditions.html)
describe aceleración/drivers; no establece que este cuelgue sea3D ni dice experimental.
HANDOFF/VM_OwnerHang1.json vigentes; prueba de teclado pendiente debajo histórica.

Vigente23:47UTC: owner confirma Inicio por mouse y ve consola. Captura23:42 sí
cambió al prompt; helper readonly se ejecutó una vez/observer87407 terminal0 a
23:45:09, pero sin resultado guest visible. Log create-only impide repetir.
Captura client de ventana23:46 confirma misma consola; RestoreTrue/ForegroundFalse,
no foco demostrado. Pregunta owner sobre escribir echo QA pendiente. No más
entrada ciega, cambios de seguridad/red/3D o reinicios. Driver/licencia/player
siguen pendientes; HANDOFF/recibo OwnerUI vigentes, notas DISABLED/mouse pendiente históricas.

Actualización23:30:06UTC: escritorio visible no implica UI funcional. Capturas
_3/Console/Run/InputCheck/SecureAttention idénticas tras entrada nativa0; no consola
ni CtrlAltDel visible. Backend32772 vivo/CPU creciente, RunLevel3, NO causa probada.
Owner debe comprobar Inicio con mouse (pregunta pendiente); parar entrada ciega.
PostbootDiagnostics helper nunca ejecutado/ahora DISABLED; licencia/dxdiag siguen
pendientes. No reset/reinicio/red/seguridad modificados; HANDOFF/recibo Input fijan evidencia.

Actualización23:26:40UTC: escritorio Windows confirmado en captura
VM_AuthorizedStart_20261015_2325_3.png, consulta0 running/RunLevel3/7.2.20r175154,
aislamiento conservado. Arranque terminado; verificar dxdiag/driver/licencia antes
de player, sin nueva instalación ni reinicio. Bienvenida debajo es observación previa.

Vigente2026-10-15 23:25:51UTC (20:25 local): owner «enciende la VM si».
Arranque GUI único EXIT0 tras guard UUID/poweroff/aislamiento; consulta0 confirma
running desde23:25:17.955UTC. Backend32772/creation23:25:08.7968170UTC por
CIM y cabecera VBox.log; captura VM_AuthorizedStart_20261015_2325_2.png muestra
Windows «Te damos la bienvenida», no pide credenciales. NIC1null/cableoff y
NIC2–8none conservados, host sin cambios. RunLevel1/version7.2.20 no prueba
API/driver/licencia postboot; observar misma VM, no duplicar arranque/installer.
Estado apagado y permisos pendientes siguientes HISTÓRICOS; HANDOFF vigente.

Vigente2026-10-15 03:10:37.4624104UTC (00:10 local): usuario autorizó apagado
forzado. VBoxManage firmado Valid/Oracle America, Inc., guard nombre/UUID
90bafe9d-68f9-4d02-8521-5f66a6a60b83; poweroff EXIT0 y consulta posterior EXIT0
confirma VMState=poweroff/StateChangeTime03:10:37.290UTC. NO reiniciada, sin
operación pendiente ni cambios al host, seguridad, aislamiento, VDI o DVD.
Preestado running tenía StateChangeTime02:42:18.581UTC: PID31452 histórico no
es identidad actual comprobada. No prueba de integridad del disco o driver/API/
licencia postboot; posible pérdida de estado sin guardar. Recibo ignorado
VM_ForcedPoweroff1.json; HANDOFFready. Antes de un nuevo arranque confirmar
intención del owner; ningún player ejecutado, M7 OPEN.

### Historial anterior al apagado (no estado actual)

Cierre00:23:30UTC: watcher74097 TERMINAL0/PID28392gone,9muestras/120s sin
pasar de RunLevel1 y captura final AÚN Reiniciando. VM31452/creation23:39:13.9103300UTC
sigue viva; footerwaiting es ese reinicio, no monitor activo. Owner debe elegir
esperar o autorizar apagado forzado SOLO VM+arranque/riesgo estado guest no
guardado. NO reset/poweroff antes de autorización ni inferir terminal de timeout.
No postboot driver/API/licencia comprobados, ningún player ejecutado.

VIGENTE GuestAdditions2026-10-15UTC/2026-10-14local: ISO53151744bytes SHA
4c6ba898e37b307cf9088be8ff480d9aec13bb0e395aaf31c78d564bb35916e7 coincide
con fila oficial [Oracle7.2.20](https://download.virtualbox.org/virtualbox/7.2.20/SHA256SUMS)
(GET200;2959chars). Montada SOLO en guest/SATA1, UUID47612f43-7f5a-4a00-a943-f31d9d4f5a38;
WindowsISO995cb764... sigue intacta en Downloads, ya no es DVD actual. Tarhost no
pudo listar Rockridge1: sin extracción/mount/installer en host ni lector nuevo.

EXEguest D:/VBoxWindowsAdditions-amd64.exe Version7.2.20/AuthenticodeValid/
OracleAmericaInc/SHA12b0658d1b66c70502e8c5e707597ea38fc3c6192fb04751c7232b5808c17b7c.
Guard antes de ejecutar, guestPID8860/creation00:08:10.6316441UTC; wizard default
guestpath/Direct3D CHECKED/Install→Completing needsrestart, sin EULA/credenciales/
rootcertmanual/security changes. Finish/RebootNow normal00:12:17.6215886UTC,
NO hostreboot/reset/poweroff. Exitcode instalador no capturado, no inventar0.

Reinicio aún no verificado: GUI «Reiniciando», mismaVM31452running/CIMcreation
23:39:13.9103300UTC; nativeAdditionsVersion7.2.20r175154/RunLevel1/BaseDriver50/
GraphicsMode0. Esto NO prueba desktop/userland/3D/featurelevels. Watcher readonly
PID28392/session74097/log GuestAdditions_RebootWatch1.log en HANDOFF; comprobar
misma operación, no reiniciar por timeout ni contar GUIidle como instalación viva.
Si requiere apagado forzado pedir autoridad owner/riesgo de estado no guardado,
no asumir VM muerta. Guard de identidad GetProcess.StartTime falló por precisión
100ns y se corrigió con MISMA API CIM, no PIDreuso. Sin causa de bug/parche probado.
VM_GuestAdditionsClosing1.json registra evidencia; luego dxdiag/driver/API/licencia
y existingarchives -dx11, baselineDX12/fullM7/security/CI/rights/performance OPEN.
NIC1null/cableoff/NIC2–8none/clipboard-dragdropdisabled preservados. Todo siguiente
que dice «instalar Additions»/«DVDWindows actual» es HISTÓRICO, no duplicar setup.

VIGENTE2026-10-14: owner autorizó NAT temporal. Única conexión23:44:27→23:45:32UTC
(65.19s observados), limpieza try/finally confirmó NIC1null/cableoff/NIC2–8none.
NIC hardware1 ahora existe pero sin red, no bit-idéntico a none inicial; aislamiento
sí verificado. No bridge/hostonly/portforward ni hostloopback accesible configurado.
Consulta OFFLINE23:51:21.6515648UTC, VM_NATStatus1.png: Admintrue, EnterpriseEval
unregistro/LicenseStatus1 Licensed, Reason1074068484 raw, GraceMinutes129594(~90d).
Fecha1600/1601 no útil; no asumir razón no-cero=fallo ni derechos comerciales.
Windows activó automáticamente al tener red. NO Activate/slmgr, claves/cuenta,
rearm/reloj/seguridad/contratos alterados por agente; UAC normal Microsoft aprobado.
Helper1 terminal por PIDausente/transcript final, exitcode no capturado; no rerun.
Helper2 preparado NUNCA ejecutado, ahora throw inicial DISABLED; no segunda NAT.

VM31452/creation23:39:13.9103300UTC/UUIDISOyVDI anteriores, GUIrunning consolaQAidle.
NEXT: procedencia/medio GuestAdditions+driver apropiado offline firmado, comprobar
API real y existingarchives -dx11; no juego ni aceleración aún. No repetir permiso
NAT/activación ni confundir VMviva con instalador activo; aislar SIEMPRE antes QA.
VM_ActivationClosing1.json conserva cierre. FullM7/security/CI/rights/performance
siguen OPEN. Párrafos de licenciaHOLD y permisos pendientes siguientes HISTÓRICOS.

VIGENTE2026-10-14: la misma VM estaba poweroff19:01:51UTC; inicio GUI0/query0
con guard poweroff/firma Oracle llegó a ESCRITORIO (VM_LocaleScreen1.png), sin
que el agente completara OOBE ni aceptara condiciones. Winver26H2/26300.9457;
el build26100 del watermark NO es la versión real. PID36464/creation19:07:39.5138140UTC,
VBox.log/CIM asociados; ISO/VDI/UUID/NIC1–8none preservados, no otra instancia.

Consultas readonly completas, VM_ReadOnlyDiagnostics1.png: EnterpriseEval,
LicenseStatus5/Notification, reason0xC004F009/gracia0. Fecha1600/1601 informada
NO demuestra fin real de90dias. Driver solo Microsoft Basic Display10.0.26100.1,
no prueba VBoxWDDM/D3D11/RHI. VM_ShutdownDiagnostics1.png: dos1074/wlms.exe
con comentario licencia; horas guest no correlacionadas con últimos poweroffhost.
[Microsoft](https://www.microsoft.com/en-us/evalcenter/evaluate-windows-11-enterprise)
advierte apagados horarios por evaluación sin activar o expirada; no reiniciar en
bucle ni cambiar reloj/rearmar para sortearlo. [Estados CIM](https://learn.microsoft.com/en-us/previous-versions/windows/desktop/sppwmi/softwarelicensingproduct)
y [código](https://learn.microsoft.com/en-us/troubleshoot/windows-server/licensing-and-activation/troubleshoot-activation-error-codes)
respaldan solo interpretación del estado, no un contrato/licencia ni cura MAK/KMS.

NEXT: pedir autorización NAT temporal para intentar SOLO activación oficial de
evaluación si corresponde; no conexión/activación ya hechas ni éxito garantizado.
Restaurar aislamiento antes QA; términos/cuenta/claves solo owner, sin compartirlos.
Luego licencia válida verificada→GuestAdditions/driver firmado/APIreal→existing
archives -dx11, sin sustituir baselineDX12. FullM7/security/CI/rights/performance OPEN.
Guiones LAA via scancode35/b5 y payload Base64 alfanumérico sinpadding corrigieron
keyboardputstring USA; primer shell falló por flags convertidos en apóstrofes,
NO securityblock demostrado. Sentinels readonly completos, consola QA idle.
VM_DesktopQualification1.json conserva cierre; datos siguientes HISTÓRICOS.

Histórico2026-10-14 17:49:07.2975734UTC: VM_ResumeScreen2.png muestra Windows
OOBE de país/región («España» seleccionada), no WinPE de idioma. La VM estaba
apagada desde17:42:04UTC al retomar, VDI64GiB había crecido a17899MiB físicos;
se inició UNA vez la misma UUID, firma Valid/Oracle y guard poweroff previos,
start/query0, sin reset/recreación/formato ni teclas al CD. PID12900 actual,
creation17:47:40.0822230UTC, log propio/CIM verificados; no usar PID32756 viejo.
Boot pasó a Windows/OOBE, pero NO guest listo/licencia/QA/Additions aprobados.
NIC1–8none y misma ISO/VDI preservados, no red/cuenta/términos por agente.
Owner continúa país/región/teclado/condiciones/cuenta personalmente; si requiere
red, acordar vía legítima antes de cambiar NIC, sin bypass y aislar antes QA.
Al llegar a escritorio verificar guest real/driver/API/Additions y existing
archives -dx11; mantener baselineDX12 y gates M7/security/CI/rights/performance.
VM_OobeResume1.json conserva nuevo cierre; datos siguientes son HISTÓRICOS.

Última revalidación/recuperación: a16:00UTC misma VM/ISO/PID y aislamiento,
pero captura VM_InstallerScreen3.png mostraba firmware/CD timer vencido.
Se abrió BootManager→UEFI VBOX CD-ROM→Space SIN reset/relaunch/cambio de
bootorder/disco/seguridad. VM_InstallerScreen4.png a16:06:37.7789404UTC
confirma nuevamente instalador Windows11 en idioma Español; comandos0,
observer79279 terminal0. Owner continúa Siguiente/teclado/condiciones.
VMState=running NO demuestra qué pantalla está abierta: comprobarla antes
de actuar. Si reaparece modal host de DVD, Cancelar solo aviso, no montar
otro DVD/reintentar inicio. Invitado instalado y QA siguen pendientes.

Post-reinicio comprobado: hostBoot2026-10-14 08:19:55UTC/WHP1/hypervisortrue.
RomanEmpireQA YA RUNNING por GUIstart0/query0; Windows11 Setup Español visible
en captura VM_InstallerScreen1.png. No otro reboot/startvm/enable/hash/attach.
Timer CD inicial venció; BootManager→UEFI CDROM→una tecla recuperó el arranque,
sin reset/bootorder/security/format/términos. Owner continúa idioma/teclado y
condiciones personalmente; NO Windows instalado aún ni player/QA aprobado.
PID32756/creation15:25:20.4916270UTC/log propio en HANDOFF y VM_PostRebootBoot1.json.
Ventana intencional se deja abierta, no una tarea de instalación desatendida.
Preservar VM/ISO/VDI y aislamiento; si red/OOBE requiere decisión legítima, pedirla.
Datos de bloques anteriores abajo son históricos cuando dicen reboot/poweroff.

VirtualBox7.2.20 INSTALADO: VBoxManage firmado Valid por Oracle America, Inc.,
--version7.2.20r175154/exit0 y registro DisplayVersion7.2.20. No service keys
de VBoxUSBMon/VBoxNetLwf/VBoxNetAdp/VBoxNetAdp6/VBoxNetFlt observados.
Installer PID12544 terminó; código final no capturado (el intento tardío de
adjuntar monitor falló porque ya no existía). No inventar installer exit0.

VM RomanEmpireQA registrada UUID90bafe9d-68f9-4d02-8521-5f66a6a60b83, poweroff,
8192MiB/4CPU, VBoxSVGA/256MiB/3D on, EFI/TPM2.0/SecureBoot enabled.
SATA0 disco UUID08d4ac74-5db6-40ea-a124-a7e1e265e785,64GiB dinámicos/2MiB reales;
SATA1 inicialmente DVD vacío; ahora ISO verificada adjuntada (detalle abajo), sin OS.
NIC1–8 none, clipboard/dragdrop disabled,
USB off/audio-input off, ninguna carpeta compartida. Archivos generados/ignorados
en `Artifacts/VirtualMachines/RomanEmpireQA`; no copiarlos a Git/LFS o distribución.

2026-10-13 16:09:58UTC: HypervisorPlatform habilitado (InstallState2→1) mediante
DISM firmado Valid/Microsoft Windows, elevación UAC normal y SOLO esa función.
Se usaron /NoRestart /LimitAccess, sin /All: ningún reinicio automático ni acceso
a Windows Update. PID42500 terminal, native exit3010 capturado del Process original;
reinicio MANUAL del host pendiente. Último boot sigue2026-10-10 21:04:22.500UTC.
Tras el reinicio, comprobar boot/feature; NO repetir enable ni recrear VM/disco.
VM verificada poweroff/DVDempty en ese bloque; luego ISO adjuntada sin arrancarla.
Sin guest/player ejecutado; instalación/condiciones del owner pendientes.
Recibo create-only: Artifacts/VMSetup/WHP_Enable_1/EnableFeature1.json.
Log nativo de errores/advertencias3bytes: no usar ese tamaño como prueba de éxito;
la evidencia es el exit3010 real y consulta CIM posterior. Sintaxis/alcance:
[Microsoft DISM](https://learn.microsoft.com/en-us/windows-hardware/manufacture/desktop/dism-global-options-for-command-line-syntax?view=windows-11).
El Manager abierto previamente era UI del usuario, no job QA. Comprobar estado
real antes de otro instalador/arranque; no arrancar guest vacío como prueba.

## Shaders del paquete — calificación de datos, NO runtime

2026-10-13: ShaderQualification1.json en Artifacts/VMSetup/VirtualBox_7_2_20_1,
SHA00928136b2b6b7dd9a595064dbbf0ba551f5f9d0a1af77963d8f69c3ff5d0a71.
31 inputs con hashes estables antes/después y comprobados nuevamente; los10
contenedores de ambos archives coinciden con los manifests QA originales.
Se reutilizaron índices/Verify/Extract nativos aprobados, sin ejecutar UnrealPak,
player, build, cook o nueva extracción.

Ambos índices Pak1737Dev/1730Ship incluyen GlobalShaderCache-PCD3D_SM5 y dos
ShaderTypeInfo SM5. Ambos índices IO1210filas incluyen dos ShaderCodeLibrary
SM5 (Global109728bytes/proyecto41696bytes) y566filas ShaderCode. Copias extraídas
de las bibliotecas existen y sus tamaños coinciden con los registros IO.
BaseEngine.ini ENTREGADO de ambos archives declara PCD3D_SM5 para D3D11 Y D3D12.
No basta con leer la ausencia de overrides D3D11 en el INI del proyecto para
concluir que se necesita otro cook. SM5 no prueba por sí mismo qué RHI arrancó.

Siguiente alternativa técnica: tras guest licenciado/aislado y driver D3D11 real,
calificar los archives existentes con el flag documentado
[-dx11](https://dev.epicgames.com/documentation/en-us/unreal-engine/unreal-engine-command-line-arguments-reference).
Solo diagnóstico en la VM APROBADA, nunca el host bloqueado. Registrar RHI,
ventana/input/viaje efectivos; no modificar DefaultGraphicsRHI ni recocinar por
anticipado. Un resultado DX11 no aprueba automáticamente el arranque defaultDX12,
baseline D3D12, rendimiento de referencia ni todos los gates M7.

Límites: metadata/index y hashes de contenedores, no decodificación independiente
de bytecode ni prueba de features/driver/objetos retenidos/compatibilidad runtime.
No se recalculó independientemente cada hash IO de chunks anónimos; los hash Pak
comprimidos no se compararon erróneamente con las copias descomprimidas. El primer
collector serializó metadatos extendidos de strings Get-Content y el resultado
quedó truncado; no se persistió esa salida. Proyección de31registros simples,
resumen escalar corregido y revalidación0; no fallo de Unreal/paquetes.
Skill unreal-packaging aplicada para separar cook/datos de prueba real del player.
WHP2/VMpoweroff a15:59:46UTC fue la observación histórica del bloque shaders;
WHP habilitado después, con reinicio pendiente. ISO/evaluación sigue sin aprobación.

## Windows invitado: falta decisión del propietario

Usuario respondió a ISO/licencia: «no, creo que no». No se recibió ninguna ISO
ni evidencia de licencia para el invitado; no reutilizar claves/activación del
host ni comprar Windows automáticamente.

Microsoft ofrece [Windows11 Enterprise Evaluation](https://www.microsoft.com/en-us/evalcenter/evaluate-windows-11-enterprise)
de90días, sin product key, con registro previo y términos de evaluación; orientada
a profesionales IT/organizaciones. Hay
[ISO x64 en español](https://www.microsoft.com/en-us/evalcenter/download-windows-11-enterprise).
Disponibilidad pública no acredita elegibilidad/aceptación del propietario.
La guía oficial menciona activación/cuenta y apagados horarios al vencer/no activar.
2026-10-13: usuario «podes conseguir la ISO?» autoriza buscar/obtener medios
oficiales. El enlace DownloadISO del EvalCenter conduce al
[formulario oficial de registro](https://info.microsoft.com/ww-landing-windows-11-enterprise.html).
Microsoft exige registro ANTES de descargar la evaluación; completar datos/condiciones
corresponde al propietario, no al agente. No autorregistrar, inventar organización,
aceptar condiciones ni saltar activación/OOBE. Avisar tras registro si corresponde;
entonces obtener Español x64 por enlace oficial y verificar medios antes de adjuntar.
Abrir ese URL en navegador mediante Start-Process fue rechazado por policy ANTES
de ejecutar: no PID/exit nativo ni workaround. Owner puede abrir el enlace manualmente.
Estado posterior: owner confirmó descarga de Español x64 EnterpriseEval NO LTSC.
Archivo en Downloads: `26300.9457.260913-1737.26h2_ge_release_svc_refresh_CLIENTENTERPRISEEVAL_OEMRET_x64FRE_es-es.iso`,
8228114432bytes, SHA256 local8f344beaa1c84646534e35f416b2086f6bef54e521c09a997dc326a330e4c584.
Nombre/tamaño coinciden con redirect oficial y HEAD200; comparación exhaustiva
TERMINADA0/614.05s:4rangos contiguos HTTP206 con ETag fijado, SHA256local=remote
para TODOS los8228114432bytes y huella local completa final estable. Recibo
ISO_FullCoverage1.json y transcript ISO_ParallelVerify1.log, no otra ISO guardada.
Esto prueba igualdad del contenido con CDN oficial bajo HTTPS, NO manifest SHA
publicado por separado/ETag-as-SHA ni condiciones/licencia/Windows instalado.
Secuencial anterior cancelado por lentitud/bound15min, no timeout de observación;
fallo lookup VerifyWindowsISO conservado. Verificadores terminales: no repetirlos.

ISO adjuntada2026-10-13 local: SATA port1/device0 de RomanEmpireQA existente,
mediaUUID995cb764-5469-4450-912b-f9982418ca3c, storageattach0/postquery0/poweroff.
Guard de UUID/config previo/DVDempty y archivo estable; NO arrancar por ese éxito.
Config SHA previodfb5375f...→ed50a0ad73cae5ef1adacb1534a866e2b286f2bd461b382bfc549503ac8227c4.
ISO_Attachment1.json conserva evidencia. No borrar/mover/renombrar ISO de Downloads
mientras esté conectada. Falta reinicio MANUAL del host y luego instalación guest
por UI intencional; owner lee/acepta condiciones/registro, nunca el agente por él.
Preflight C64.27GiB libres; volver a comprobar tamaño/espacio real al descargar,
conservar20GiB libres considerando crecimiento del guest. No instalar en el host.
Otra opción sigue siendo ISO con licencia válida; obtener medios no concede licencia.

Observación histórica00:52:23UTC: HypervisorPlatform seguíaInstallState2,
OptionalFeatures PID40832 ya no está; managerPID1036 sí, list runningvms vacío.
Enable posterior terminado3010; no enable/reinicio ni descarga ISO en marcha.
Manager es UI interactiva del usuario, no un job QA/build ni VM en ejecución.

## Selección y preflight

- VirtualBox base GPL-3.0-only, sin Extension Pack, por disponibilidad pública.
  WinGet source winget informa Oracle.VirtualBox7.2.20, release2026-09-22.
- Host Home25H2/31.4GiB/24logical; C:70.8GiB libres al inicio. Sesión no elevada.
  VCRedist x64 instalado14.50.35719.0: no actualizarlo por instalar la VM.
- Hypervisor y VirtualMachinePlatform ya activos. HypervisorPlatform inicialmente
  InstallState2; habilitado después a1, reinicio manual pendiente. Oracle requiere
  [Windows Hypervisor Platform activo para coexistir con Hyper-V](https://docs.oracle.com/en/virtualization/virtualbox/7.2/user/AdvancedTopics.html).
  No desactivar hipervisor/VBS ni reiniciar automáticamente. Acción administrativa
  puntual terminada; acordar reinicio manual y comprobarlo antes de instalar guest.
- [VirtualBox Guest Additions](https://docs.oracle.com/en/virtualization/virtualbox/7.2/user/guestadditions.html)
  documenta Direct3D11.0/11.1, NO soporte del perfil DX12 de este paquete.
  VM instalada/Windows arrancado NO equivale a juego compatible ni QA aprobado.
  No modificar DefaultGraphicsRHI, cook, baseline visual o paquetes actuales.
  Si se necesita perfil QA-DX11, pedir dirección para ese paquete separado y
  verificar shaders/features en lugar de asumir que -d3d11 bastará.
- No ISO encontrada en el nivel superior de Downloads/Desktop/Artifacts
  comprobado. No búsqueda recursiva de archivos personales. Ruta de ISO y
  licencia del invitado solicitadas al propietario; no pedir claves/credenciales.

## Instalador y ejecución controlada

Ruta local: `Artifacts/VMSetup/VirtualBox_7_2_20_1/VirtualBox-7.2.20-175154-Win.exe`.
Descarga oficial:
`https://download.virtualbox.org/virtualbox/7.2.20/VirtualBox-7.2.20-175154-Win.exe`.
SHA256 anunciado por WinGet:
`a81777d2b36380ce042a29e9c554cf032eb46a793f62e3cc82e7411e535c2c26`.
Antes de ejecutar: descarga terminal exit0, hash exacto y Authenticode Valid,
publisher Oracle. Una descarga parcial/firma inválida no se ejecuta ni se borra
para ocultar el error. Conservar recibos create-only bajo Artifacts/VMSetup.

Primera descarga timeout180s: shell1/curl28,98940084bytes. Reanudación del mismo
archivo/URL con -C - y límite600s: exit0,178192488bytes y SHA exacto; Authenticode
Valid, SimpleName exactamente Oracle America, Inc. La primera comprobación del
publisher falló protegida por un regex que no admitía comillas del Subject;
se corrigió a SimpleName exacto, sin debilitar hash/firma y sin ejecutar antes.

Asistente gráfico con UAC normal confirmado por el usuario; no autoaceptar
advertencias de drivers unsigned. Instalación mínima según
[Oracle](https://docs.oracle.com/en/virtualization/virtualbox/7.2/user/installation.html):
ADDLOCAL=VBoxApplication, sin USB/bridged/host-only/Python; VBOX_START=0,
sin asociaciones/shortcuts nuevos y REBOOT=ReallySuppress. No instalación silenciosa
con aceptación de contratos oculta. El proceso y estado efectivos están en HANDOFF;
no iniciar otro instalador mientras esté vivo ni inferir éxito del launcher.

Después: comprobar VBoxManage.exe --version y registro de versión instalado;
no declarar instalado solo porque se abrió el asistente. Solo una VM registrada,
con showvminfo y disco reales, acredita creación de la VM.

## Perfil creado — invitado todavía no instalado

Nombre RomanEmpireQA;4vCPU,8GiB RAM,64GiB disco dinámico, EFI/TPM2.0/SecureBoot
del invitado si la ISO seleccionada es Windows11. No bypass de requisitos:
[Microsoft](https://www.microsoft.com/en-us/windows/windows-11-specifications).
Disco dinámico no reserva64GiB reales: vigilar crecimiento y conservar al menos
20GiB libres en el host; snapshots multiplican almacenamiento. No instalar UE
dentro del invitado: solo Windows/prerequisites aprobados y paquetes QA.

Para QA: NIC deshabilitada, clipboard/drag-drop/USB y cuentas del host ausentes;
transferencia solo de cohorte aprobada por canal acordado, nunca workspace/engine/
Saved/Cooked/Windows/ue.projectstore. La instalación inicial de Windows puede
tener requisitos de red/cuenta según edición: resolver con el propietario y
aislar red ANTES del juego, sin scripts para saltar OOBE/licencias/seguridad.

Provisionado: herramienta/VM/disco/UEFI-TPM-SecureBoot/aislamiento. Primera creación
se detuvo protegida al intentar agregar SATA ya creado por --default, exit1;
se consultó VM/disco reales y se reutilizó SATA, SIN recrear VM/disco/UUIDs.
La consulta showmediuminfo --machinereadable falló (flag no soportado); consulta
sin ese flag y continuación storageattach/NVRAM/verificación exit0.

Orden pendiente: owner idioma/teclado/condiciones → instalar Windows en VDI64GiB
(owner condiciones/licencia) →
Guest Additions verificadas → comprobar GPU/API →
resolver perfil gráfico del juego → viaje [QA-01–10](M7_TRUSTED_PLAYER_QA.md).
No FPS de referencia/clean-CI/security-HOLD/licencias de distribución aprobados.
