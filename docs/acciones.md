# Qué puede hacer cada botón y encoder

Todo se configura desde el widget de Plasma (sección de la capa → botones y
encoders) y se aplica con **Guardar**.

## Tipos de acción de un botón

| En el widget | Qué hace |
|---|---|
| Ninguna | Nada. |
| Tecla / atajo | Aprieta una tecla o una combinación (ver abajo). |
| Capa siguiente | Pasa a la capa siguiente; desde la última vuelve a la primera. |
| Capa anterior | Pasa a la capa anterior; desde la primera va a la última. |
| Ir a capa… | Salta a una capa fija. **Se cuenta desde 0**: la primera capa es 0, la segunda es 1, etc. |

Los encoders siempre mandan teclas: una lista para el giro horario y otra para el
antihorario. Cada "click" del encoder aprieta esa combinación una vez.

## Cómo escribir una tecla o un atajo

- Las teclas se separan con **coma**: `ctrl, shift, t`.
- Se aprietan todas juntas en ese orden y se sueltan al revés, igual que hacés
  con la mano. Poné siempre los modificadores primero.
- No importan mayúsculas ni espacios: `Ctrl, T` es lo mismo que `ctrl,t`.
- Si una tecla está mal escrita, ese botón no hace nada (no rompe el resto).
- En la pantalla de la placa se ve el atajo abreviado: `CTRL+SHIFT+T`
  (máximo 14 caracteres).

### Los atajos de KDE valen

El macropad aprieta **teclas reales**, como un teclado más. Así que cualquier
atajo que funcione con tu teclado funciona igual desde el macropad: los de KDE,
los de cada programa, los de juegos, etc. Solo hay que traducir la forma de
escribirlo: donde KDE dice `+`, acá va una coma.

| KDE lo muestra como | En el widget |
|---|---|
| `Meta+E` | `meta, e` |
| `Ctrl+Alt+T` | `ctrl, alt, t` |
| `Meta+L` | `meta, l` |
| `Ctrl+F1` | `ctrl, f1` |
| `Meta+Shift+S` | `meta, shift, s` |
| `Alt+Tab` | `alt, tab` |
| `Imprimir` | `sysrq` |

(Ojo: `Imprimir` / `ImprPant` se llama `sysrq` acá, no `print`.)

### Truco: F13 a F24 para acciones propias

Tu teclado no tiene F13–F24, pero el macropad sí puede mandarlas. Como nadie
más las usa, son ideales para acciones a medida sin chocar con nada:

1. En el widget, asigná a un botón `f13` (o `f14`, …, `f24`).
2. En **Configuración del sistema → Teclado → Atajos de teclado → Agregar
   nuevo → Orden o script**, poné el comando que quieras (abrir una app,
   un script, `systemctl suspend`, etc.).
3. Como atajo, tocá el botón del macropad: KDE lo registra como `F13`.

Así un botón puede hacer cualquier cosa que se pueda escribir como comando.

## Nombres de teclas

### Modificadores

| Escribir | Tecla |
|---|---|
| `ctrl` (o `control`) / `rctrl` | Control izquierdo / derecho |
| `shift` / `rshift` | Shift izquierdo / derecho |
| `alt` / `ralt` | Alt / AltGr |
| `meta` (o `super`, `win`) | Tecla Meta / Windows |

### Letras, números y básicas

| Escribir | Tecla |
|---|---|
| `a` … `z` | Letras |
| `0` … `9` | Números de la fila de arriba |
| `f1` … `f24` | Teclas de función |
| `enter`, `esc`, `tab`, `space`, `backspace` | Las de siempre |
| `delete`, `insert`, `home`, `end`, `pageup`, `pagedown` | Bloque de edición |
| `up`, `down`, `left`, `right` | Flechas |
| `capslock`, `menu`, `sysrq` (Imprimir) | Otras |
| `kp0` … `kp9`, `kpplus`, `kpminus`, `kpasterisk`, `kpslash`, `kpenter`, `kpdot` | Teclado numérico |

### Multimedia y sistema

| Escribir | Qué hace |
|---|---|
| `volumeup` (o `vol+`) / `volumedown` (o `vol-`) | Subir / bajar volumen (con el cartelito de KDE) |
| `mute` | Silenciar |
| `micmute` | Silenciar el micrófono |
| `playpause` (o `play`) | Reproducir / pausar |
| `nextsong` (o `next`) / `previoussong` (o `prev`) | Tema siguiente / anterior |
| `stop` | Detener |
| `brightnessup` / `brightnessdown` | Brillo de pantalla (en monitores que KDE puede controlar) |
| `calc`, `mail`, `homepage`, `search`, `file` | Abren calculadora, correo, navegador, búsqueda, gestor de archivos |
| `zoomin`, `zoomout` | Zoom (donde la app lo soporte) |

### Signos de puntuación

Los signos se escriben por el **nombre de la tecla física** en un teclado
estadounidense, no por el carácter. Con la distribución en español la
misma tecla escribe otra cosa: el macropad aprieta la tecla y el sistema
decide qué carácter sale, igual que con tu teclado.

| Escribir | Tecla física (EE.UU.) | Con distribución española (es) |
|---|---|---|
| `minus` | `-` | `'` |
| `equal` | `=` | `¡` |
| `leftbrace` / `rightbrace` | `[` / `]` | `` ` `` / `+` |
| `semicolon` | `;` | `ñ` |
| `apostrophe` | `'` | `´` |
| `grave` | `` ` `` | `º` |
| `backslash` | `\` | `ç` |
| `comma`, `dot`, `slash` | `,` `.` `/` | `,` `.` `-` |
| `102nd` | (no existe) | `<` |

(Con distribución latinoamericana cambian un poco: si un signo no sale como
esperás, probá con la tecla vecina.)

La lista completa son los nombres `KEY_…` del kernel de Linux, en minúscula
y sin el `KEY_`: cualquiera de ellos funciona.

## Ejemplos listos

| Para… | Botón / encoder | Escribir |
|---|---|---|
| Volumen | Encoder | horario `volumeup`, antihorario `volumedown` |
| Brillo | Encoder | horario `brightnessup`, antihorario `brightnessdown` |
| Zoom | Encoder | horario `ctrl, kpplus`, antihorario `ctrl, kpminus` (el + y − del teclado numérico andan igual con cualquier distribución) |
| Cambiar de pestaña | Encoder | horario `ctrl, tab`, antihorario `ctrl, shift, tab` |
| Cambiar de escritorio virtual | Encoder | horario `meta, ctrl, right`, antihorario `meta, ctrl, left` |
| Scroll de línea de tiempo (video) | Encoder | horario `right`, antihorario `left` |
| Deshacer / rehacer | Encoder | horario `ctrl, shift, z`, antihorario `ctrl, z` |
| Bloquear sesión | Botón | `meta, l` |
| Captura de pantalla (Spectacle) | Botón | `sysrq` o `meta, shift, s` |
| Vista general de ventanas | Botón | `meta, w` |
| Escritorio | Botón | `meta, d` |
| Terminal (Konsole) | Botón | `ctrl, alt, t` |
| Gestor de archivos | Botón | `meta, e` |
| Silenciar micrófono en una llamada | Botón | `micmute` |
| Copiar / pegar | Botones | `ctrl, c` / `ctrl, v` |

Estos son los atajos por defecto de KDE Plasma 6. Si cambiaste alguno,
mirá el tuyo en **Configuración del sistema → Teclado → Atajos de teclado**.

## Sin la PC: modo Bluetooth

Cuando la placa funciona sola como teclado Bluetooth (para el celular), usa
las mismas capas, pero entiende **menos teclas**:

- Modificadores (`ctrl`, `shift`, `alt`, `meta` y sus variantes).
- Letras, números y signos de **un solo carácter** escritos tal cual
  (`a`, `7`, `-`, `.`), no por nombre (`minus` no anda en Bluetooth).
- `enter`, `esc`, `tab`, `space`, `backspace`, `delete`, flechas, `home`,
  `end`, `pageup`, `pagedown`, `f1` … `f12`.
- Multimedia: `playpause`, `stop`, `next`, `prev`, `mute`, `volumeup`,
  `volumedown` (con sus alias).

No hay brillo, F13–F24, `micmute`, `sysrq` ni las de aplicaciones
(`calc`, `mail`…). Un botón con alguna de esas teclas no hace nada en modo
Bluetooth, pero sigue funcionando normal con la PC.
