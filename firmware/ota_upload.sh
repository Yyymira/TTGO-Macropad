#!/usr/bin/env bash
# Compila y sube el firmware por WiFi (OTA), sin cable.
# Uso: ./ota_upload.sh [ip-o-host]
# Sin argumento, usa la IP que reporta la placa al servicio de la PC, o
# ttgo-macropad.local si el servicio no está corriendo.
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")"

CFG="$HOME/.config/ttgo-macropad/config.json"
TTGO_OTA_PASS="$(python3 -c 'import json,sys; print(json.load(open(sys.argv[1]))["device"]["ota_pass"])' "$CFG")"
export TTGO_OTA_PASS

HOST="${1:-}"
TOKEN_FILE="${XDG_RUNTIME_DIR:-/run/user/$(id -u)}/ttgo-macropad/token"
if [[ -z "$HOST" && -r "$TOKEN_FILE" ]]; then
    HOST="$(curl -s -m 3 -H "Authorization: Bearer $(cat "$TOKEN_FILE")" http://127.0.0.1:47823/status \
        | python3 -c 'import json,sys; print((json.load(sys.stdin).get("device_net") or {}).get("ip") or "")' || true)"
fi
HOST="${HOST:-ttgo-macropad.local}"

echo "Subiendo por WiFi a $HOST ..."
.venv/bin/pio run -e ota -t upload --upload-port "$HOST"
