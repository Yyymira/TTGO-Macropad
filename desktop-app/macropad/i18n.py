"""Traducciones de la app y del servicio. Los textos del código están en
inglés; `tr()` devuelve la traducción al idioma del sistema si existe, o el
inglés si no. El mismo idioma se le manda a la placa (ver
`controller.settings_message`) para que su pantalla coincida.

Para sumar un idioma: agregar un diccionario a TRANSLATIONS con su código
de 2 letras ("pt", "fr"...) y las mismas claves que "es". En el firmware
hay que sumarlo también a `firmware/src/i18n.cpp`, y en el widget a
`plasmoid/contents/ui/Translations.js`."""

import locale
import os
import sys

TRANSLATIONS: dict[str, dict[str, str]] = {
    "es": {
        # controller (etiquetas en la pantalla de la placa)
        "Layer >>": "Capa >>",
        "<< Layer": "<< Capa",
        "Layer {n}": "Capa {n}",
        "View: {v}": "Vista: {v}",
        "Auto": "Auto",
        "Stats": "Stats",
        "Weather": "Clima",
        "Media": "Multimedia",
        "Speed +": "Vel +",
        "Speed -": "Vel -",
        "Speed 1x": "Vel 1x",
        "Skip ad": "Saltar anunc.",
        "Tab vol +": "Vol pest +",
        "Tab vol -": "Vol pest -",
        "Go to tab": "Ir a pestana",
        "Mic": "Mic",
        "Camera": "Camara",
        "Switch idle screen": "Cambiar pantalla de reposo",
        # config por defecto
        "Browser": "Navegador",
        # servicio (errores que muestra el widget)
        "Couldn't find the city “{city}”": "No encontré la ciudad «{city}»",
        "Couldn't look up the city (no internet?)": "No pude buscar la ciudad (¿sin internet?)",
        "invalid token": "token inválido",
        "not found": "no existe",
        "invalid configuration": "configuración inválida",
        "missing index": "falta index",
        # ventana (Windows)
        "Disconnected": "Desconectado",
        "Connected": "Conectado",
        "Mode: {mode} — Layer: {layer}": "Modo: {mode} — Capa: {layer}",
        "[connection] {state}": "[conexión] {state}",
        "connected": "conectado",
        "disconnected": "desconectado",
        "Status": "Estado",
        "Settings": "Configuración",
        "Show window": "Mostrar ventana",
        "Quit": "Salir",
        "Still running in the tray.": "Sigue corriendo en la bandeja.",
        "None": "Ninguna",
        "Key / shortcut": "Tecla / atajo",
        "Run command": "Ejecutar comando",
        "Browser": "Navegador",
        "Next layer": "Capa siguiente",
        "Previous layer": "Capa anterior",
        "Go to layer N": "Ir a capa N",
        "e.g. ctrl,t  or  playpause  (or the command to run, or a browser action)":
            "ej: ctrl,t  o  playpause  (o el comando a ejecutar, o una acción del navegador)",
        "Keys": "Teclas",
        "Commands": "Comandos",
        "clockwise, e.g. volumeup": "horario, ej: volumeup",
        "counterclockwise, e.g. volumedown": "antihorario, ej: volumedown",
        "Clockwise:": "Horario:",
        "Counterclockwise:": "Antihorario:",
        "+ Layer": "+ Capa",
        "- Layer": "- Capa",
        "Rename": "Renombrar",
        "Layers": "Capas",
        "Unnamed": "Sin nombre",
        "Keys go to:": "Las teclas van a:",
        "PC": "PC",
        "Phone (Bluetooth)": "Celular (Bluetooth)",
        "Switch to this layer with programs:": "Pasar a esta capa con los programas:",
        "e.g. firefox.exe, spotify.exe": "ej: firefox.exe, spotify.exe",
        "Key {n}": "Tecla {n}",
        "Board (GPIO0)": "Placa (GPIO0)",
        "Board (GPIO35)": "Placa (GPIO35)",
        "Encoder 1 (press)": "Encoder 1 (apretar)",
        "Encoder 2 (press)": "Encoder 2 (apretar)",
        "Encoder 1 (back)": "Encoder 1 (atrás)",
        "Encoder 2 (front)": "Encoder 2 (adelante)",
        "Buttons of the selected layer": "Botones de la capa seleccionada",
        "Encoders of the selected layer": "Encoders de la capa seleccionada",
        "Idle timeout (back to stats):": "Timeout de inactividad (vuelve a stats):",
        "Save and apply": "Guardar y aplicar",
        "New layer": "Nueva capa",
        "Layer name:": "Nombre de la capa:",
        "Can't delete": "No se puede borrar",
        "There must be at least one layer.": "Tiene que quedar al menos una capa.",
        "Rename layer": "Renombrar capa",
        "Name:": "Nombre:",
        "Done": "Listo",
        "Settings saved and applied.": "Configuración guardada y aplicada.",
    },
}


def _detect_language() -> str:
    for var in ("LANGUAGE", "LC_ALL", "LC_MESSAGES", "LANG"):
        value = os.environ.get(var, "")
        if value:
            return value.split(":")[0][:2].lower()
    if sys.platform == "win32":
        try:
            import ctypes

            lcid = ctypes.windll.kernel32.GetUserDefaultUILanguage()
            return locale.windows_locale.get(lcid, "en")[:2].lower()
        except Exception:
            pass
    return "en"


_detected = _detect_language()
LANG = _detected if _detected in TRANSLATIONS else "en"


def tr(text: str, **fmt) -> str:
    out = TRANSLATIONS.get(LANG, {}).get(text, text)
    return out.format(**fmt) if fmt else out
