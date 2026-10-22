"""Acción "Ejecutar comando": lanza un comando de shell en segundo plano,
como el usuario que corre la app, sin esperar a que termine.

En Linux usa bash (si existe) para que funcionen `~`, variables, tuberías,
`&&`, etc.; en Windows, cmd.exe sin abrir ventana de consola. La salida se
descarta.

En Linux, si hay systemd, cada comando se lanza como su propia unidad
(`systemd-run --user`): si no, el programa abierto (un navegador, Steam...)
quedaría dentro del servicio del macropad, y systemd lo cerraría cada vez
que el servicio se reinicia."""

import os
import shutil
import subprocess
import sys

_SHELL = shutil.which("bash") if sys.platform != "win32" else None
_SYSTEMD_RUN = shutil.which("systemd-run") if sys.platform.startswith("linux") else None


def run_command(cmd: str) -> None:
    cmd = (cmd or "").strip()
    if not cmd:
        return
    if _SYSTEMD_RUN and _SHELL:
        try:
            subprocess.Popen(
                [_SYSTEMD_RUN, "--user", "--collect", "--quiet",
                 f"--working-directory={os.path.expanduser('~')}", "--", _SHELL, "-c", cmd],
                stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
            ).wait(timeout=5)  # systemd-run vuelve enseguida: el comando sigue en su propia unidad
            return
        except subprocess.TimeoutExpired:
            return  # ya se lanzó; no repetirlo
        except OSError:
            pass  # sin systemd-run utilizable: lanzarlo directo
    kwargs = dict(
        shell=True,
        cwd=os.path.expanduser("~"),
        stdin=subprocess.DEVNULL,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )
    if sys.platform == "win32":
        kwargs["creationflags"] = subprocess.CREATE_NEW_PROCESS_GROUP | subprocess.CREATE_NO_WINDOW
    else:
        kwargs["start_new_session"] = True
        if _SHELL:
            kwargs["executable"] = _SHELL
    try:
        subprocess.Popen(cmd, **kwargs)
    except OSError:
        pass  # comando imposible de lanzar: se ignora, igual que una tecla mal escrita
