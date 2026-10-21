"""Acción "Ejecutar comando": lanza un comando de shell en segundo plano,
como el usuario que corre la app, sin esperar a que termine.

En Linux usa bash (si existe) para que funcionen `~`, variables, tuberías,
`&&`, etc.; en Windows, cmd.exe sin abrir ventana de consola. El proceso
queda en su propia sesión/grupo, así que cerrar o reiniciar el servicio no
lo mata, y su salida se descarta."""

import os
import shutil
import subprocess
import sys

_SHELL = shutil.which("bash") if sys.platform != "win32" else None


def run_command(cmd: str) -> None:
    cmd = (cmd or "").strip()
    if not cmd:
        return
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
