# TFTD Workshop 2.12.7

## 1. Primeros pasos

Workshop 2.12.7 edita y muestra contenido TFTD/OXCE. Abre mapas, compone conjuntos y genera mapas procedurales. No es un programa de dibujo: cree o edite los PNG en un editor de imágenes y compruébelos aquí.

Tres conceptos distintos:
• MAP: posiciones de piezas, con cuatro capas por casilla.
• MCD: identidad y propiedades, fotogramas, movimiento, puertas, destrucción, alturas y ocupación.
• PNG: representación gráfica. Cambiar el dibujo no cambia automáticamente colisiones, UT, disparos ni estados de destrucción.

Las capas son Suelo, Muro oeste, Muro norte y Objeto. Z es el nivel vertical. Una casilla puede contener piezas en varias capas. Una imagen de roca puede ser un cuarto de un conjunto de cuatro casillas: compruebe el conjunto antes de usarla sola.

Empiece en Recursos: ubicación TFTD ORIGINAL, OXCE STANDARD y MODS OXCE. TFTD apunta a los datos del juego; OXCE a la raíz de la instalación; MODS a user/mods. Reindexar vuelve a leer archivos y perfiles. Los paneles separan originales, complementos OXCE y mods. Las rutas personales son ajustes locales, no una configuración obligatoria para publicar.

Abra un MAP SEABED existente para estudiar composiciones conocidas. Los originales están protegidos; cree una copia en su mod. Pruebe primero una pieza comprendida y luego el conjunto completo. Esta guía describe el estado del 28 de septiembre de 2026; los prototipos no son funciones validadas en juego.

## 2. Funciones del Workshop

ARCHIVO
Nuevo MAP define X, Y y Z por separado (1 a 255). Abrir MAP carga geometría lógica y resuelve conjuntos. Guardar escribe el documento actual: escenas en JMW; MAP de mod con copia de seguridad. Guardar en mod / Exportar pide nombre, bioma y grupo. Salir trata el trabajo sin guardar.

RECURSOS
Ubicaciones TFTD/OXCE/MODS definen fuentes. Mod HD manual elige proveedor PNG sin cambiar la lógica. Carpetas MAP adicionales sirven para compatibilidad, no para mezclar perfiles de otros mods. Reindexar relee fuentes. Vaciar paleta vacía conjuntos activos, no archivos. Añadir MCD y cargar paleta LBM son avanzados: PCK/TAB debe corresponder a MCD.

NAVEGADOR
Mapas abre/filtra MAP. Biblioteca elige piezas y muestra conjunto/MCD. Conjuntos MAP muestra juegos activos e índices. Buscar filtra; divisores cambian tamaño. LEGACY INACTIVO/OXCE INACCESIBLE indica referencias, no archivos que borrar.

EDICIÓN
Seleccionar [V]: pieza del Z activo; Mayús+clic añade/retira. Conjunto solo reconoce grupos completos de Biblia/Hangar. Colocar [B]: elegir pieza y pulsar; el pincel permite pintar continuo. Borrar [E] elimina explícitamente en el Z elegido. Cuentagotas [I] recoge pieza. Mover [M] y Duplicar piden destino; rueda cambia Z, clic confirma, clic derecho/Esc cancela. Eliminar retira selección. Ctrl+Z restaura toda la operación; Ctrl+Y rehace.

Opciones permite reemplazar lugares ocupados, selección por conjunto, atenuación, opacidad y encuadre. Alertas FLOOR/BigWall señalan revisiones, no arreglos automáticos. Detalles técnicos muestra MCD, fotograma, capa, propiedades y procedencia.

VISTAS
F1/F2/F3/F4 elige Suelo/Muro oeste/Muro norte/Objeto. Rueda cambia Z; Ctrl+rueda zoom; botón central desplaza. RePág/AvPág cambia zoom. Solo activo / activo+inferiores / completa se recuerdan. Colocación sigue en Z activo. Cuadrícula [G] oculta también reservas de naves. Centrar/Encuadrar recupera la vista.

PLANO
Piezas muestra gráficos; Plano 2D, anotaciones lógicas superiores; ISO, perspectiva. [P] alterna. Suelo/zona y objeto/decorado están separados. Casilla, contorno, sólido y relleno eligen acción; cuadrado/círculo/rombo y tamaño definen área. Muros cardinales/diagonales, suelos triangulares y enlaces Z describen el plano. Bloquear protege casillas. Selección rectangular, Ctrl+C, Ctrl+V y clic pega; Esc cancela. Copiar conserva datos y cuatro capas. Una anotación no añade reglas al motor. Diagonales requieren MCD BigWall compatible.

CONJUNTOS / HANGAR
Abrir Hangar gestiona biblioteca personal. Nueva captura: pulsar piezas visibles de todas capas y niveles; repetir clic retira; nombrar y guardar. Usar recoge conjunto; renombrar/eliminar gestiona entradas. Copiar MAP entero conserva posiciones relativas. Elegir carpeta cambia biblioteca.

COMPOSITOR
OpenXcom fiel respeta restricciones oficiales; Ver receta explica órdenes y grupos. Libre monta una familia con dimensiones múltiplos de 10. Variación define semilla; otra variación la cambia. Opciones nave/USO: aparatos, posiciones y separación. Insertar USO llena reserva. Guardar/Abrir JMW conserva escena; cerrar vuelve al documento inferior. Recetas no compatibles se rechazan, no se sustituyen en silencio.

RENDERIZADO
F6 alterna Legacy, PNG Remastered, proveedor manual, REAL HD y depuración. Plantillas universales es proveedor gráfico. GEO_TERRAIN abre inspector independiente de 102 piezas y escenas: abrir/guardar GEO, ver pieza, conjuntos de referencia, geometría neutra/material SAND y dominio/nivel visible. Laboratorio externo distinto del generador.

RUTAS RMP
Mostrar/ocultar [R] no cambia datos. Edición coloca/selecciona/elimina nodos. Enlace conecta dos nodos; conectores N/E/S/W unen bloques. Ruta sencilla usa valores neutros; todos/1x1/vuelo restringe unidades. Avanzado: rango, preferencia de patrulla, prioridad de aparición, objetivo. Analizar propone en memoria; borrar quita propuestas; aplicar añade nodos revisados. Guardar RMP escribe mod con copia. Las propuestas no prueban navegación IA en misión.

PROCEDURAL se explica después. IDIOMA cambia interfaz y recuerda selección. TUTORIAL abre guía. AYUDA/Acerca de muestra versión y límites.

## 3. Generador y conjuntos

Procedural mejora un solo generador. Tamaño: ancho/largo 20 a 120 casillas. Riqueza: disperso, variado, denso. Relieve: plano o terrazas SAND + GEO multinivel. Separación: 2 a 5 casillas. Naves X-COM/alienígenas opcionales conservan orientación. Semilla (0 a 4294967295) reproduce mapa con mismos recursos y versión. Mapa aleatorio cambia semilla y genera. Fallo/cancelación conserva documento anterior.

Conjuntos históricos se mezclan con relieve nuevo. Decorado sobre zonas horizontales con acceso geométrico libre. No prueba que todas las unidades puedan recorrer el mapa en juego.

Catálogo: ROCKS MCD 0 y 1 autónomos; bloques 2×2 con filas [9,8]/[7,10], [5,4]/[3,6] o [9,8]/[7,6], verificados en MAP originales. ROCKS 10 no es independiente. Bloques reservan huella completa. En altura solo se colocan rocas autónomas. Defina conjuntos nuevos con piezas/posiciones explícitas, no proximidad visual.

Estado GEO: generación, vista de piezas y guardado JMW4 con controles automáticos. Edición directa GEO, representación en plano y exportación MAP/OXCE no conectadas. Exportación bloqueada; guardar .JMW. Ejecutables anteriores rechazan JMW4; proyectos antiguos siguen abriendo. Decorado fraccionario ligado a GEO. Para cambiar forma, regenerar con otra semilla por ahora.

## 4. Crear PNG HD 512 × 640

Un sprite Legacy de terreno usa una envolvente de 32 × 40 píxeles. El lienzo HD estándar 512 × 640 amplía ×16 en ambos ejes. Es el lienzo, no un objeto que deba estirarse hasta los bordes. Conserve proyección isométrica, origen, posición y transparencia. No recorte cada pieza por separado ni recentre su base. Algunas plantillas documentadas exceden la envolvente: no las corte para forzar altura 640.

Antes de dibujar, anote conjunto, índice MCD, Frame[0], capa, orientación, animación, estado destruido y conjunto completo. Ejemplo SAND original verificado: MCD 13 → Frame[0] 15 → 015.png; MCD 15 → Frame[0] 17 → 017.png. MCD y PNG no son intercambiables. El inspector muestra correspondencia; varios MCD pueden compartir fotograma.

Proceso:
1. Parta del sprite original o plantilla aceptada y captura del conjunto. Conserve el original.
2. Amplíe pixel art 32×40 a 512×640 con vecino más cercano. Amplía píxeles, no crea detalles. Redibujo HD o mejora asistida añade detalles, pero debe preservar forma y conexiones.
3. Separe referencia geométrica, máscara alfa y material/detalles en capas. En plantilla aceptada no cambie silueta ni alfa. Reconstruir geometría nueva requiere validación aparte.
4. Exporte PNG RGBA transparente, sin fondo opaco. Mantenga lienzo completo y nombre de tres cifras. Revise sombras pintadas, bordes y semitransparencias sobre fondos claros/oscuros.
5. Coloque en carpeta de prueba; compruebe pieza sola, vecinos, repetición, varios Z y destrucción. Una imagen bonita aislada puede dejar uniones visibles.

Animación: Workshop usa el fotograma inicial MCD para PNG de terreno; no muestra todos los ciclos del juego. Revise otros fotogramas/orientaciones en juego con proveedor gráfico activo.

Texturas no reparan pendientes ni conexiones incorrectas. No rellene vacíos intencionales ni transforme Floor funcional en Object solo para arreglar dibujo.

## 5. Carpetas y primera prueba PNG

Para una prueba personal, cree MiTallerHD fuera de mods protegidos:

MiTallerHD/
  Resources/TFTD_HD/Terrain/SAND/015.png

SAND es el conjunto; 015 el fotograma. Recursos > Mod HD manual: elija raíz MiTallerHD. F6: Plantillas universales/proveedor manual. Abra mapa con SAND MCD 13. El PNG sustituye fotograma inicial. Si falta, usa Legacy. Material REAL HD de arena utiliza otro camino: capítulo 6.

La carpeta funciona en Workshop sin ser mod activo del juego. Para mod OXCE, añada metadata.yml con id único, name, version, author y master: xcom2; actívelo según el motor. Ejemplo:

id: mi_taller_hd
name: Mi Taller HD
version: 0.1.0
author: Su nombre
master: xcom2

Metadatos identifican el mod; no conectan por sí solos proveedor gráfico en todas versiones OXCE. Revise motor REAL HD adaptado y prioridad en juego. Mantenga ids/nombres técnicos; traduzca etiquetas aparte.

PNG Remastered busca primero en mod de la pieza y después en TFTD PNG remastered, bajo Resources/TFTD_HD/Terrain/<dataset>/<frame>.png. No busca arbitrariamente en todos los mods. Proveedor manual da ruta explícita independiente de perfiles lógicos.

Organización personal:
Resources/ = archivos finales
Sources/ = capas/proyectos editables
References/ = capturas y notas conjunto/MCD/frame
Documentation/ = conjuntos y validación

En mod compartido respete reglas: PNG finales en Terrain/<dataset>; fuentes/pruebas en Datasets/<dataset>; documentación estable. GEO: Terrain/00_geo_terrain y Datasets/GEO_TERRAIN. No cree prefijos por revisión ni sustituya plantillas aceptadas para probar.

Tras editar PNG, reinicie Workshop para vaciar caché con seguridad. No vigila cambios externos en vivo. La vista Workshop no confirma prioridad ni colisión del juego.

## 6. REAL HD: geometría y materiales

REAL HD es más que ampliar sprites. Reconstruye superficies desde mapa, perfiles y geometría y aplica materiales. OXCE conserva simulación: colisiones, movimiento, disparos, visibilidad, puertas y destrucción. La textura no decide reglas.

Recursos SAND activos:
user/mods/TFTD_REAL_HD_TEXTURES/Resources/TFTD_HD/RealHD/Datasets/SAND/Materials/
  TOP_BASE.png
  VERTICAL_BASE.png
  TOP_NORMAL_DX.png
  TOP_ROUGHNESS.png
  TOP_AO.png

TOP_BASE: color superior, no PNG isométrico de pieza. VERTICAL_BASE: caras verticales. NORMAL_DX: microdetalle de luz, convención DirectX; no cambia silueta/colisión. ROUGHNESS: rugosidad (habitualmente oscuro=liso, claro=rugoso; revisar sombreador). AO: oclusión local, no sustituye toda iluminación. Máscaras de impactos, BlastSets y polvo son efectos separados.

Materiales repetibles, no lienzos 512×640. Use texturas que conecten en cuatro bordes a escala coherente. Conserve dimensiones y convenciones iniciales. Normal es dato, no foto para recolorear. Evite sombras direccionales fuertes si el motor calcula luz.

Primera prueba: copie materiales SAND a su espacio de trabajo. Modifique solo TOP_BASE, conserve otras imágenes/nombres. Workshop lee ambas bases del mod explícito TFTD_REAL_HD_TEXTURES; proveedor PNG manual no redirige materiales. Para esta vista, conserve copia recuperable del material actual, sustituya solo el archivo deseado y reinicie. Restaure si hace falta. La guía no sustituye nada. Aún no hay selector de carpetas REAL HD arbitrarias.

F6 > REAL HD muestra geometría SAND/DEBRIS y bases, no todo el renderizado del motor: sin los mismos sombreadores, luz, agua, cáusticas o efectos persistentes de P2ZJ. Revise normal, rugosidad y AO en juego con motor actual; anote versión/mods/ajustes.

Para otras familias, crear carpeta similar no basta: deben existir productor gráfico y reglas. SEA/agua tiene contrato/ajustes propios; unidades, HUD y efectos usan otros proveedores. Cambiar SAND no crea materiales de casco, unidad o agua.

Renderizado mixto (2.12.7): elegir Renderizado → REAL HD textura o REAL HD debug, y después PNG junto a REAL HD → PNG Remastered o Plantillas universales. SAND/DEBRIS y GEO_TERRAIN conservan la geometría REAL HD; las otras piezas usan el proveedor PNG elegido y Legacy si falta el PNG. La selección PNG se recuerda. GEO usa TOP_BASE y VERTICAL_BASE de SAND del mod normal o debug correspondiente. La geometría y los datos lógicos no cambian.

## 7. Validar y resolver problemas

PNG invisible: revise F6, carpeta manual, conjunto, Frame[0], nombre 000.png de tres cifras, extensión .png real, archivo legible y alfa no vacío. Legacy no demuestra que PNG cargó. Reinicie tras editar para vaciar caché.

Pieza grande/desplazada/cortada: revise lienzo, anclaje, proporciones; 512 ancho, 640 alto. Busque recorte, estiramiento y extensiones documentadas.

Uniones: monte vecinos reales según MAP/MCD; compare bordes, orientación y repetición. No esconda errores con plantas/sombras.

Roca incompleta: verifique unidad entera del catálogo; fragmentos no son autónomos. Bloque en altura necesita apoyo horizontal igual en toda huella.

Damero magenta REAL HD: revise TFTD_REAL_HD_TEXTURES, TOP_BASE/VERTICAL_BASE, raíz MODS y PNG legibles. Renombrar sprite no arregla normal ausente.

Índice MAP sin resolver: cargue perfil correcto. MAP usa 0=vacío, 1..255=índices de lista de conjuntos; añadir al azar crea piezas fantasma. Comprenda aviso antes de normalizar.

Colisión/paso incorrecto con dibujo correcto: revise MCD, parches, capa, altura, BigWall, destrucción y pruebe OXCE. Anotaciones no crean reglas de movimiento.

Cuatro etapas: archivo producido → controles de software → conjunto Workshop → misión OXCE y opinión del usuario. Conserve versión, recursos, semilla/mapa y capturas. Revise luz/agua, descubrimiento/ocultación, animación, puertas, destrucción y niveles Z. Exportación correcta o captura aislada no es validación en juego.

## 8. Compartir y contribuir

El futuro repositorio puede incluir fuentes, compilación, pruebas, traducciones y guía. Entrega local: ningún repositorio creado/publicado. Datos TFTD/OXCE y PNG del proyecto no incluidos en el código; usuario configura rutas en Recursos.

Traducciones: locales/fr.json, en.json, es.json, de.json. Claves vinculadas a fuentes en sources.json. No traduzca carpetas técnicas, ids MCD/MAP, formatos ni atajos. Conserve %ls, %d, %u y otros marcadores en mismo orden. Generador valida y crea workshop_i18n_data.h antes de compilar. Idioma nuevo requiere catálogo completo y entrada de menú. Interfaz traducida; nombres personales, archivos e ids conservan original. Revisión por nativos puede mejorar estilo sin cambiar parámetros.

Guía: docs/chapters.json contiene cuatro idiomas. Herramientas generan páginas locales y datos del lector Tutorial. Actualice versión/límites al cambiar funciones. Dé ejemplos reproducibles, no compatibilidad general sin prueba.

Informe errores con versiones Workshop/motor, modo, conjunto/MCD/frame, mapa/semilla, resultado esperado/observado y captura del conjunto. Textura: dimensiones/alfa, fuente geométrica, finales y pruebas reales Workshop/juego. PNG válido no implica plantilla aceptada.

Aprenda por etapas: suelo individual → cuatro vecinos → conjunto de varias casillas → mapa → misión. Cambie una cosa cada vez para identificar efecto.

Contacto de Benjamin: colmoutarde57700@gmail.com

Thanks to GPT-6 Sol

Si desea apoyar el proyecto, los donativos son opcionales.

Cuenta PayPal: col.moutarde@hotmail.fr