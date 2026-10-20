#!/usr/bin/env bash
# Instala en Linux/KDE Plasma 6: el servicio de fondo (systemd de usuario,
# arranca solo al iniciar sesión) y el widget de Plasma. Se puede volver a
# correr para actualizar. No necesita root.
set -euo pipefail

APP_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PYTHON="$APP_DIR/.venv/bin/python"
UNIT_DIR="$HOME/.config/systemd/user"
UNIT="$UNIT_DIR/ttgo-macropad.service"
PLASMOID_ID="com.github.yyymira.ttgomacropad"

if [[ ! -x "$PYTHON" ]]; then
    echo "Falta el venv. Crealo primero:" >&2
    echo "  cd \"$APP_DIR\" && python3 -m venv --system-site-packages .venv && .venv/bin/pip install -r requirements.txt" >&2
    exit 1
fi

mkdir -p "$UNIT_DIR"
cat > "$UNIT" <<EOF
[Unit]
Description=TTGO Macropad (servicio de fondo)
After=graphical-session.target
PartOf=graphical-session.target

[Service]
ExecStart="$PYTHON" "$APP_DIR/daemon.py"
Restart=on-failure
RestartSec=5
TimeoutStopSec=5

[Install]
WantedBy=graphical-session.target
EOF

systemctl --user daemon-reload
systemctl --user enable ttgo-macropad.service
systemctl --user restart ttgo-macropad.service

if kpackagetool6 --type Plasma/Applet --show "$PLASMOID_ID" >/dev/null 2>&1; then
    kpackagetool6 --type Plasma/Applet --upgrade "$APP_DIR/plasmoid"
else
    kpackagetool6 --type Plasma/Applet --install "$APP_DIR/plasmoid"
fi

echo
echo "Listo. Servicio: systemctl --user status ttgo-macropad"
echo "Para ver el widget: clic derecho en el panel > Agregar o administrar widgets > \"TTGO Macropad\","
echo "o en la bandeja del sistema: Configurar bandeja del sistema > Entradas > TTGO Macropad > Mostrar siempre."
