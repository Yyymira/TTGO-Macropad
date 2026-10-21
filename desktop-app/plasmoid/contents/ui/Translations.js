.pragma library

// Traducciones del widget. Los textos de main.qml están en inglés; tr()
// devuelve la traducción al idioma de Plasma si existe, o el inglés si no.
// Para sumar un idioma: otro objeto en TRANSLATIONS con su código de 2
// letras y las mismas claves que "es".

const TRANSLATIONS = {
    "es": {
        "None": "Ninguna",
        "Key / shortcut": "Tecla / atajo",
        "Run command": "Ejecutar comando",
        "Next layer": "Capa siguiente",
        "Previous layer": "Capa anterior",
        "Go to layer…": "Ir a capa…",
        "Key {n}": "Tecla {n}",
        "Board (GPIO0)": "Placa (GPIO0)",
        "Board (GPIO35)": "Placa (GPIO35)",
        "Encoder 1 (press)": "Encoder 1 (apretar)",
        "Encoder 2 (press)": "Encoder 2 (apretar)",
        "Encoder 1 (back)": "Encoder 1 (atrás)",
        "Encoder 2 (front)": "Encoder 2 (adelante)",
        "Stats only": "Solo stats",
        "Clock and weather only": "Solo reloj y clima",
        "Both (alternating)": "Ambos (alternando)",
        "The service is not running": "El servicio no está corriendo",
        "Board disconnected": "Placa desconectada",
        "Active layer: {name}": "Capa activa: {name}",
        "Error {code}": "Error {code}",
        "Saved": "Guardado",
        "WiFi connected ({ip})": "WiFi conectado ({ip})",
        "WiFi not connected": "WiFi sin conectar",
        "phone connected via Bluetooth": "celular conectado por Bluetooth",
        "Bluetooth waiting for a phone": "Bluetooth esperando celular",
        "Layer {n}": "Capa {n}",
        "Service stopped": "Servicio detenido",
        "Board connected": "Placa conectada",
        "Showing: {name}": "Mostrando: {name}",
        "Showing stats": "Mostrando stats",
        "Can't reach the service. Start it with:": "No se puede contactar al servicio. Iniciarlo con:",
        "Layers": "Capas",
        "Show this layer on the board": "Mostrar esta capa en la placa",
        "Layer name": "Nombre de la capa",
        "Keys go to": "Las teclas van a",
        "PC": "PC",
        "Phone (Bluetooth)": "Celular (Bluetooth)",
        "Switch to this layer automatically with:": "Pasar a esta capa sola con estos programas:",
        "e.g. firefox, org.kde.dolphin": "ej: firefox, org.kde.dolphin",
        "Add the program that was in the foreground": "Agregar el programa que estaba en primer plano",
        "Add": "Agregar",
        "Delete": "Borrar",
        "Buttons": "Botones",
        "e.g. ctrl, t": "ej: ctrl, t",
        "e.g. konsole  or  bash ~/scripts/thing.sh": "ej: konsole  o  bash ~/scripts/algo.sh",
        "Encoders": "Encoders",
        "Keys": "Teclas",
        "Commands": "Comandos",
        "command on clockwise turn": "comando al girar horario",
        "clockwise, e.g. volumeup": "horario, ej: volumeup",
        "command on counterclockwise turn": "comando al girar antihorario",
        "counterclockwise": "antihorario",
        "Device": "Dispositivo",
        "Idle screen": "Pantalla en reposo",
        "Without the PC, the board always shows clock and weather, and keys go to the phone over Bluetooth.":
            "Sin la PC, la placa muestra siempre reloj y clima, y las teclas van al celular por Bluetooth.",
        "WiFi network": "Red WiFi",
        "network name (2.4 GHz)": "nombre de la red (2.4 GHz)",
        "Password": "Contraseña",
        "City": "Ciudad",
        "e.g. London, United Kingdom": "ej: Córdoba, Argentina",
        "Weather for: {place}": "Clima de: {place}",
        "Back to idle after": "Volver a reposo tras",
        "Unsaved changes": "Cambios sin guardar",
        "Discard": "Descartar",
        "Save": "Guardar",
    },
};

const LANG = Qt.locale().name.substring(0, 2);

function tr(text, args) {
    const table = TRANSLATIONS[LANG] || {};
    let out = table[text] !== undefined ? table[text] : text;
    if (args) {
        for (const key in args) out = out.split("{" + key + "}").join(String(args[key]));
    }
    return out;
}
