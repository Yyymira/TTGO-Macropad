**English** | [Español](acciones.md)

# What each button and encoder can do

On Linux everything is set up in the Plasma widget (layer section → buttons
and encoders) and applied with **Save**. On Windows, in the app's
**Settings** tab, with **Save and apply**; the format is the same, but it
understands fewer keys (see [On Windows](#on-windows)).

## Button action types

| In the widget | What it does |
|---|---|
| None | Nothing. |
| Key / shortcut | Presses a key or a combination (see below). |
| Run command | Runs a command or script (see [Running commands](#running-commands)). |
| Next layer | Goes to the next layer; from the last one it wraps to the first. |
| Previous layer | Goes to the previous layer; from the first one it wraps to the last. |
| Go to layer… | Jumps to a fixed layer. **Counting starts at 0**: the first layer is 0, the second is 1, and so on. |
| Switch idle screen | Pins the idle screen for a while: each press goes **Auto → Stats → Weather → Auto**, and the board shows it right away. It's temporary: it doesn't change your settings, and restarting the PC or the board goes back to the configured one. On the board, the button shows which one is active ("View: Weather"). |

There are up to 10 buttons per layer: the 6 keys, the 2 on the board, and
each encoder's push switch ("Encoder 1 (press)" and "Encoder 2 (press)",
pressing the knob). The last two only exist if your encoders have a push
switch; with 3-pin encoders, leave them on "None". All of them are set up the
same way; the encoder ones start out unassigned. On the screen, the encoder
switches show up in a column on the left only in layers where they have
something assigned.

Each encoder's rotation is set up with **Keys** (one combination for
clockwise and another for counterclockwise; each "click" presses it once) or
with **Commands** (one command per direction, run on each "click").

## How to write a key or a shortcut

- Separate keys with a **comma**: `ctrl, shift, t`.
- They're pressed together in that order and released in reverse, just like
  you'd do by hand. Always put the modifiers first.
- Case and spaces don't matter: `Ctrl, T` is the same as `ctrl,t`.
- If a key name is misspelled, that button does nothing (it doesn't break
  anything else).
- The board's screen shows the shortcut abbreviated: `CTRL+SHIFT+T`
  (14 characters max).

### KDE shortcuts work

The macropad presses **real keys**, like any other keyboard. So any shortcut
that works with your keyboard works the same from the macropad: KDE's, each
program's, games', etc. You only need to change how it's written: where KDE
shows `+`, use a comma here.

| KDE shows it as | In the widget |
|---|---|
| `Meta+E` | `meta, e` |
| `Ctrl+Alt+T` | `ctrl, alt, t` |
| `Meta+L` | `meta, l` |
| `Ctrl+F1` | `ctrl, f1` |
| `Meta+Shift+S` | `meta, shift, s` |
| `Alt+Tab` | `alt, tab` |
| `Print` | `sysrq` |

(Note: `Print` / `PrtSc` is called `sysrq` here, not `print`.)

### Tip: F13 to F24 for your own actions

Your keyboard doesn't have F13–F24, but the macropad can send them. Since
nothing else uses them, they're perfect for custom actions that won't clash
with anything:

1. In the widget, assign `f13` (or `f14`, …, `f24`) to a button.
2. In **System Settings → Keyboard → Shortcuts → Add New → Command or
   Script**, enter whatever command you want (open an app, a script,
   `systemctl suspend`, etc.).
3. For the shortcut, press the macropad button: KDE records it as `F13`.

For most things, though, the **Run command** action is simpler.

## Running commands

With **Run command** (on a button) or **Commands** (on an encoder), you type
the command exactly as you would in a terminal:

| To… | Command |
|---|---|
| Open an app | `konsole`, `dolphin`, `firefox`, `spotify` |
| Run your own script | `bash ~/scripts/thing.sh` |
| Open a web page | `xdg-open https://github.com` |
| Open a folder | `xdg-open ~/Downloads` |
| Suspend the PC | `systemctl suspend` |
| Volume in 5 % steps (encoder) | clockwise `wpctl set-volume -l 1 @DEFAULT_AUDIO_SINK@ 5%+`, counterclockwise `wpctl set-volume @DEFAULT_AUDIO_SINK@ 5%-` |
| Test notification | `notify-send "Macropad" "It works!"` |

- It runs with **bash**, as your user, from your home folder: `~`,
  variables, pipes (`|`), `&&`, etc. all work.
- It runs in the background: the macropad doesn't wait for it to finish and
  you don't see the output. To see what it does, open it in a terminal:
  `konsole --hold -e bash ~/scripts/thing.sh`.
- No `sudo`. If a command needs it, it won't work (there's nowhere to type
  the password).
- The board's screen shows the command cut to 14 characters.
- On Windows it runs with `cmd.exe` (e.g. `notepad`,
  `start https://github.com`, `C:\scripts\thing.bat`).
- In Bluetooth mode (without the PC) commands do nothing: the phone can't run
  things on your PC.

Commands are stored in `~/.config/ttgo-macropad/config.json`, which only your
user can read and modify.

## Key names

### Modifiers

| Write | Key |
|---|---|
| `ctrl` (or `control`) / `rctrl` | Left / right Control |
| `shift` / `rshift` | Left / right Shift |
| `alt` / `ralt` | Alt / AltGr |
| `meta` (or `super`, `win`) | Meta / Windows key |

### Letters, numbers and basics

| Write | Key |
|---|---|
| `a` … `z` | Letters |
| `0` … `9` | Top-row numbers |
| `f1` … `f24` | Function keys |
| `enter`, `esc`, `tab`, `space`, `backspace` | The usual ones |
| `delete`, `insert`, `home`, `end`, `pageup`, `pagedown` | Editing block |
| `up`, `down`, `left`, `right` | Arrows |
| `capslock`, `menu`, `sysrq` (Print) | Others |
| `kp0` … `kp9`, `kpplus`, `kpminus`, `kpasterisk`, `kpslash`, `kpenter`, `kpdot` | Numeric keypad |

### Media and system

| Write | What it does |
|---|---|
| `volumeup` (or `vol+`) / `volumedown` (or `vol-`) | Volume up / down (with KDE's on-screen display) |
| `mute` | Mute |
| `micmute` | Mute the microphone |
| `playpause` (or `play`) | Play / pause |
| `nextsong` (or `next`) / `previoussong` (or `prev`) | Next / previous track |
| `stop` | Stop |
| `brightnessup` / `brightnessdown` | Screen brightness (on monitors KDE can control) |
| `calc`, `mail`, `homepage`, `search`, `file` | Open the calculator, mail, browser, search, file manager |
| `zoomin`, `zoomout` | Zoom (where the app supports it) |

### Punctuation

Punctuation is written by the **name of the physical key** on a US keyboard,
not by the character. With another keyboard layout the same key types
something else: the macropad presses the key and the system decides which
character comes out, just like with your keyboard.

| Write | Physical key (US) |
|---|---|
| `minus` | `-` |
| `equal` | `=` |
| `leftbrace` / `rightbrace` | `[` / `]` |
| `semicolon` | `;` |
| `apostrophe` | `'` |
| `grave` | `` ` `` |
| `backslash` | `\` |
| `comma`, `dot`, `slash` | `,` `.` `/` |
| `102nd` | the extra key next to left Shift on ISO keyboards (`<` `>` / `\` `\|`) |

The full list is the Linux kernel's `KEY_…` names, in lowercase and without
the `KEY_`: any of them works.

## Ready-made examples

| To… | Button / encoder | Write |
|---|---|---|
| Volume | Encoder | clockwise `volumeup`, counterclockwise `volumedown` |
| Brightness | Encoder | clockwise `brightnessup`, counterclockwise `brightnessdown` |
| Zoom | Encoder | clockwise `ctrl, kpplus`, counterclockwise `ctrl, kpminus` (the keypad + and − work the same on any layout) |
| Switch tabs | Encoder | clockwise `ctrl, tab`, counterclockwise `ctrl, shift, tab` |
| Switch virtual desktops | Encoder | clockwise `meta, ctrl, right`, counterclockwise `meta, ctrl, left` |
| Scrub a video timeline | Encoder | clockwise `right`, counterclockwise `left` |
| Undo / redo | Encoder | clockwise `ctrl, shift, z`, counterclockwise `ctrl, z` |
| Lock the session | Button | `meta, l` |
| Screenshot (Spectacle) | Button | `sysrq` or `meta, shift, s` |
| Window overview | Button | `meta, w` |
| Show desktop | Button | `meta, d` |
| Terminal (Konsole) | Button | `ctrl, alt, t` |
| File manager | Button | `meta, e` |
| Mute the mic on a call | Button | `micmute` |
| Copy / paste | Buttons | `ctrl, c` / `ctrl, v` |

These are KDE Plasma 6's default shortcuts. If you changed any, check yours
in **System Settings → Keyboard → Shortcuts**.

## On Windows

The Windows app uses the same format (comma-separated keys, layers, "Go to
layer" from 0), and Windows shortcuts work just like KDE's do on Linux:
`Win+E` is written `win, e`.

For now it understands **fewer key names** than Linux:

- Modifiers: `ctrl`, `shift`, `alt` (and their `l…`/`r…` versions, like
  `lctrl`, `rshift`, `ralt`), `win` (or `meta`, `super`).
- Letters `a` … `z` and numbers `0` … `9`.
- `f1` … `f12`.
- `enter`, `esc`, `tab`, `space`, `backspace`, `delete`, arrows, `home`,
  `end`, `pageup`, `pagedown`.
- Media: `volumeup`, `volumedown`, `mute`, `playpause`, `stop`, `nextsong`,
  `previoussong` (with their aliases). Volume shows Windows' on-screen
  display.
- `brightnessup` / `brightnessdown`: only on their own (not combined with
  other keys) and only on screens Windows can control, usually laptop
  screens. On a desktop monitor they do nothing.

Not available on Windows: F13–F24, `sysrq`, `insert`, `capslock`, `menu`, the
numeric keypad, punctuation, `micmute`, `calc`/`mail`/`homepage`/`search`/
`file` and `zoomin`/`zoomout`. A button with any of those keys does nothing
(it does work on Linux).

Examples with Windows shortcuts:

| To… | Write |
|---|---|
| File Explorer | `win, e` |
| Show desktop | `win, d` |
| Lock the session | `win, l` |
| Snipping tool | `win, shift, s` |
| Task view | `win, tab` |
| Switch virtual desktops (encoder) | clockwise `win, ctrl, right`, counterclockwise `win, ctrl, left` |
| Task Manager | `ctrl, shift, esc` |
| Settings | `win, i` |

## Per-program layers

Each layer can switch on by itself when a program comes to the foreground. In
the widget, under the layer's destination, there's **"Switch to this layer
automatically with"**: type the names separated by commas (`firefox,
org.kde.dolphin`), or press the **+** button next to it, which adds the
program that was in the foreground before you opened the widget.

- When you enter the program, the board shows the new layer for a few
  seconds.
- When you leave to a program without its own layer (or to the desktop), it
  goes back to the layer you had before.
- If you change layers by hand while in the program, that's respected: when
  you leave, you stay on the one you picked.
- Case doesn't matter. Both the long name (`org.kde.dolphin`) and the short
  one (`dolphin`) work.

On KDE this works through a small KWin script installed by
`linux/install.sh` ("TTGO Macropad: active window", in **System Settings →
Window Management → KWin Scripts**): on Wayland, only KWin knows which window
is active. On Windows it uses the executable name (`firefox.exe`,
`spotify.exe`) and is set up in the Settings tab.

## Layers for the phone

Each layer has a destination: **"Keys go to: PC"** (the default) or
**"Phone (Bluetooth)"**. In a phone layer, with the board plugged into the
PC, keys and encoders go out through the board's Bluetooth to the phone
instead of being typed on the PC. So you can have, for example, a "phone
Spotify" layer and switch to it with "Next layer" without unplugging
anything.

- Layer switching keeps working the same, so you can go back and forth
  between PC and phone layers.
- The keys it understands are the Bluetooth-mode ones (see below).
- "Run command" in a phone layer still runs the command on the PC: the phone
  can't run commands.
- On the screen, a phone layer shows the battery and "BT" (light blue if a
  phone is connected) at the top right; in the widget's list it shows
  "· BT".
- Without the PC, every layer goes to the phone, whatever its destination.

## Without the PC: Bluetooth mode

When the board works on its own as a Bluetooth keyboard (for your phone), it
uses the same layers, but understands **fewer keys**:

- Modifiers (`ctrl`, `shift`, `alt`, `meta` and their variants).
- Letters, numbers and **single-character** symbols written as-is
  (`a`, `7`, `-`, `.`), not by name (`minus` doesn't work over Bluetooth).
- `enter`, `esc`, `tab`, `space`, `backspace`, `delete`, arrows, `home`,
  `end`, `pageup`, `pagedown`, `f1` … `f12`.
- Media: `playpause`, `stop`, `next`, `prev`, `mute`, `volumeup`,
  `volumedown` (with their aliases).

No brightness, F13–F24, `micmute`, `sysrq` or app keys (`calc`, `mail`…). A
button with any of those keys does nothing in Bluetooth mode, but keeps
working normally with the PC.

On Android, apps like **Key Mapper** (free, open source) can turn a key from
an external keyboard into an action: assign a key you don't use for typing
(e.g. `f9`) to a button, and map "F9 → open Spotify" in Key Mapper.
