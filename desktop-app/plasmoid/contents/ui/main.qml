// Widget de Plasma 6 para el TTGO Macropad. Habla con el servicio de fondo
// (desktop-app/daemon.py) por su API local en 127.0.0.1:47823; el token se
// lee de $XDG_RUNTIME_DIR/ttgo-macropad/token.

import QtQuick
import QtQuick.Layouts
import org.kde.plasma.plasmoid
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.components as PC3
import org.kde.plasma.plasma5support as P5Support
import org.kde.kirigami as Kirigami

PlasmoidItem {
    id: root

    readonly property string apiBase: "http://127.0.0.1:47823"
    readonly property var actionKinds: ["none", "key", "layer_next", "layer_prev", "layer_set"]
    readonly property var actionNames: ["Ninguna", "Tecla / atajo", "Capa siguiente", "Capa anterior", "Ir a capa…"]
    readonly property var buttonNames: ["Tecla 1", "Tecla 2", "Tecla 3", "Tecla 4", "Tecla 5", "Tecla 6",
                                        "Placa (GPIO0)", "Placa (GPIO35)"]
    readonly property var encoderNames: ["Encoder 1 (atrás)", "Encoder 2 (adelante)"]

    property string token: ""
    property bool daemonUp: false
    property var status: ({})
    property var draft: null      // copia editable de la configuración
    property int rev: 0           // se incrementa para refrescar el formulario
    property bool dirty: false
    property int editLayer: 0
    property string message: ""

    Plasmoid.icon: "input-keyboard"
    Plasmoid.status: PlasmaCore.Types.ActiveStatus
    toolTipMainText: "TTGO Macropad"
    toolTipSubText: !daemonUp ? "El servicio no está corriendo"
                  : !status.connected ? "Placa desconectada"
                  : "Capa activa: " + (status.layer_name || "")

    onExpandedChanged: {
        if (root.expanded) {
            poll()
            if (!dirty) loadConfig()
        }
    }

    // ---------------- comunicación con el servicio ----------------
    P5Support.DataSource {
        id: exec
        engine: "executable"
        connectedSources: []
        onNewData: (source, data) => {
            disconnectSource(source)
            root.token = (data["stdout"] || "").trim()
            if (root.token) {
                root.poll()
                if (root.expanded && !root.dirty) root.loadConfig()
            }
        }
    }

    function readToken() {
        exec.connectSource('cat "$XDG_RUNTIME_DIR/ttgo-macropad/token" 2>/dev/null')
    }

    function api(method, path, body, onOk) {
        if (!token) { readToken(); return }
        const xhr = new XMLHttpRequest()
        xhr.onreadystatechange = () => {
            if (xhr.readyState !== XMLHttpRequest.DONE) return
            if (xhr.status === 0) { daemonUp = false; token = ""; return }
            daemonUp = true
            if (xhr.status === 401) { token = ""; readToken(); return }
            if (xhr.status !== 200) {
                try { message = JSON.parse(xhr.responseText).error } catch (e) { message = "Error " + xhr.status }
                return
            }
            if (onOk) onOk(JSON.parse(xhr.responseText))
        }
        xhr.open(method, apiBase + path)
        xhr.setRequestHeader("Authorization", "Bearer " + token)
        if (body !== undefined) {
            xhr.setRequestHeader("Content-Type", "application/json")
            xhr.send(JSON.stringify(body))
        } else {
            xhr.send()
        }
    }

    function poll() { api("GET", "/status", undefined, s => status = s) }

    function loadConfig() {
        api("GET", "/config", undefined, cfg => {
            draft = cfg
            editLayer = Math.min(editLayer, cfg.layers.length - 1)
            dirty = false
            message = ""
            rev++
        })
    }

    function saveConfig() {
        api("POST", "/config", draft, () => {
            dirty = false
            message = "Guardado"
            poll()
        })
    }

    function activateLayer(i) { api("POST", "/layer", {index: i}, () => poll()) }

    Timer {
        interval: root.expanded ? 1000 : 5000
        running: true
        repeat: true
        triggeredOnStart: true
        onTriggered: root.token ? root.poll() : root.readToken()
    }

    // ---------------- acceso al borrador ----------------
    function layerAt(i, _rev) {
        return (draft && draft.layers && draft.layers[i]) ? draft.layers[i] : null
    }
    function buttonAt(i, _rev, _layer) {
        const l = layerAt(editLayer)
        return (l && l.buttons[i]) ? l.buttons[i] : {action: "none"}
    }
    function encoderAt(i, _rev, _layer) {
        const l = layerAt(editLayer)
        return (l && l.encoders[i]) ? l.encoders[i] : {cw: [], ccw: []}
    }
    function setButton(i, action) {
        const l = layerAt(editLayer)
        if (!l) return
        while (l.buttons.length <= i) l.buttons.push({action: "none"})
        l.buttons[i] = action
        dirty = true
        rev++
    }
    function setButtonField(i, field, value) {
        const l = layerAt(editLayer)
        if (!l || !l.buttons[i]) return
        l.buttons[i][field] = value
        dirty = true
    }
    function setEncoderKeys(i, dir, keys) {
        const l = layerAt(editLayer)
        if (!l) return
        while (l.encoders.length <= i) l.encoders.push({cw: [], ccw: []})
        l.encoders[i][dir] = keys
        dirty = true
    }
    function textToKeys(t) {
        return t.split(",").map(s => s.trim()).filter(s => s.length > 0)
    }
    function addLayer() {
        if (!draft) return
        const buttons = buttonNames.map(() => ({action: "none"}))
        const encoders = encoderNames.map(() => ({cw: [], ccw: []}))
        draft.layers.push({name: "Capa " + (draft.layers.length + 1), buttons: buttons, encoders: encoders})
        editLayer = draft.layers.length - 1
        dirty = true
        rev++
    }
    function removeLayer() {
        if (!draft || draft.layers.length <= 1) return
        draft.layers.splice(editLayer, 1)
        editLayer = Math.max(0, editLayer - 1)
        dirty = true
        rev++
    }

    // ---------------- panel desplegable ----------------
    fullRepresentation: Item {
        Layout.preferredWidth: Kirigami.Units.gridUnit * 26
        Layout.preferredHeight: Kirigami.Units.gridUnit * 34
        Layout.minimumWidth: Kirigami.Units.gridUnit * 22
        Layout.minimumHeight: Kirigami.Units.gridUnit * 18

        ColumnLayout {
            anchors.fill: parent
            spacing: Kirigami.Units.smallSpacing

            // Estado
            RowLayout {
                Layout.fillWidth: true
                Kirigami.Icon {
                    source: root.daemonUp && root.status.connected ? "network-connect" : "network-disconnect"
                    Layout.preferredWidth: Kirigami.Units.iconSizes.smallMedium
                    Layout.preferredHeight: Kirigami.Units.iconSizes.smallMedium
                }
                Kirigami.Heading {
                    level: 4
                    Layout.fillWidth: true
                    text: !root.daemonUp ? "Servicio detenido"
                        : root.status.connected ? "Placa conectada" : "Placa desconectada"
                }
                PC3.Label {
                    visible: root.daemonUp && root.status.connected === true
                    opacity: 0.7
                    text: root.status.mode === "layer" ? "Mostrando: " + root.status.layer_name : "Mostrando stats"
                }
            }
            PC3.Label {
                Layout.fillWidth: true
                visible: root.daemonUp && root.status.stats !== undefined && root.status.stats.cpu !== undefined
                opacity: 0.7
                font: Kirigami.Theme.smallFont
                text: {
                    const s = root.status.stats || {}
                    return "CPU " + s.cpu + "% · " + s.cpu_temp + "°C    GPU " + s.gpu + "% · " + s.gpu_temp
                         + "°C    RAM " + s.ram + "%" + (s.vram >= 0 ? "    VRAM " + s.vram + "%" : "")
                }
            }
            PC3.Label {
                Layout.fillWidth: true
                visible: !root.daemonUp
                wrapMode: Text.WordWrap
                text: "No se puede contactar al servicio. Iniciarlo con:\nsystemctl --user start ttgo-macropad"
            }

            Kirigami.Separator { Layout.fillWidth: true; visible: root.draft !== null }

            PC3.ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                visible: root.draft !== null
                PC3.ScrollBar.horizontal.policy: PC3.ScrollBar.AlwaysOff

                ColumnLayout {
                    width: parent.width - Kirigami.Units.largeSpacing
                    spacing: Kirigami.Units.smallSpacing

                    // Capas
                    Kirigami.Heading { level: 5; text: "Capas" }
                    Repeater {
                        model: root.rev >= 0 && root.draft ? root.draft.layers.length : 0
                        delegate: RowLayout {
                            id: lrow
                            required property int index
                            Layout.fillWidth: true
                            PC3.ItemDelegate {
                                Layout.fillWidth: true
                                highlighted: root.editLayer === lrow.index
                                // tilde = capa activa en la placa; resaltado = capa que se está editando
                                icon.name: root.status.layer_index === lrow.index ? "checkmark" : ""
                                text: {
                                    const l = root.layerAt(lrow.index, root.rev)
                                    return lrow.index + ": " + (l ? l.name : "")
                                }
                                onClicked: { root.editLayer = lrow.index; root.rev++ }
                            }
                            PC3.ToolButton {
                                icon.name: "media-playback-start"
                                display: PC3.AbstractButton.IconOnly
                                text: "Mostrar esta capa en la placa"
                                PC3.ToolTip.text: text
                                PC3.ToolTip.visible: hovered
                                enabled: !root.dirty
                                onClicked: root.activateLayer(lrow.index)
                            }
                        }
                    }
                    RowLayout {
                        PC3.TextField {
                            Layout.fillWidth: true
                            placeholderText: "Nombre de la capa"
                            text: { const l = root.layerAt(root.editLayer, root.rev); return l ? l.name : "" }
                            onTextEdited: {
                                const l = root.layerAt(root.editLayer)
                                if (l) { l.name = text; root.dirty = true }
                            }
                            onEditingFinished: root.rev++
                        }
                        PC3.ToolButton { icon.name: "list-add"; text: "Agregar"; onClicked: root.addLayer() }
                        PC3.ToolButton {
                            icon.name: "list-remove"
                            text: "Borrar"
                            enabled: root.draft !== null && root.draft.layers.length > 1
                            onClicked: root.removeLayer()
                        }
                    }

                    // Botones
                    Kirigami.Heading { level: 5; text: "Botones"; Layout.topMargin: Kirigami.Units.largeSpacing }
                    Repeater {
                        model: root.buttonNames.length
                        delegate: RowLayout {
                            id: brow
                            required property int index
                            readonly property var act: root.buttonAt(index, root.rev, root.editLayer)
                            Layout.fillWidth: true
                            PC3.Label {
                                text: root.buttonNames[brow.index]
                                Layout.preferredWidth: Kirigami.Units.gridUnit * 6
                                elide: Text.ElideRight
                            }
                            PC3.ComboBox {
                                model: root.actionNames
                                Layout.preferredWidth: Kirigami.Units.gridUnit * 8
                                currentIndex: Math.max(0, root.actionKinds.indexOf(brow.act.action || "none"))
                                onActivated: (i) => {
                                    const kind = root.actionKinds[i]
                                    const a = {action: kind}
                                    if (kind === "key") a.keys = []
                                    if (kind === "layer_set") a.index = 0
                                    root.setButton(brow.index, a)
                                }
                            }
                            PC3.TextField {
                                visible: brow.act.action === "key"
                                Layout.fillWidth: true
                                placeholderText: "ej: ctrl, t"
                                text: (brow.act.keys || []).join(", ")
                                onTextEdited: root.setButtonField(brow.index, "keys", root.textToKeys(text))
                            }
                            PC3.SpinBox {
                                visible: brow.act.action === "layer_set"
                                from: 0
                                to: root.draft ? root.draft.layers.length - 1 : 0
                                value: brow.act.index || 0
                                onValueModified: root.setButtonField(brow.index, "index", value)
                            }
                            Item {
                                visible: brow.act.action !== "key" && brow.act.action !== "layer_set"
                                Layout.fillWidth: true
                            }
                        }
                    }

                    // Encoders
                    Kirigami.Heading { level: 5; text: "Encoders"; Layout.topMargin: Kirigami.Units.largeSpacing }
                    Repeater {
                        model: root.encoderNames.length
                        delegate: ColumnLayout {
                            id: erow
                            required property int index
                            readonly property var enc: root.encoderAt(index, root.rev, root.editLayer)
                            Layout.fillWidth: true
                            PC3.Label { text: root.encoderNames[erow.index]; opacity: 0.8 }
                            RowLayout {
                                Layout.fillWidth: true
                                PC3.Label { text: "↻" }
                                PC3.TextField {
                                    Layout.fillWidth: true
                                    placeholderText: "horario, ej: volumeup"
                                    text: (erow.enc.cw || []).join(", ")
                                    onTextEdited: root.setEncoderKeys(erow.index, "cw", root.textToKeys(text))
                                }
                                PC3.Label { text: "↺" }
                                PC3.TextField {
                                    Layout.fillWidth: true
                                    placeholderText: "antihorario"
                                    text: (erow.enc.ccw || []).join(", ")
                                    onTextEdited: root.setEncoderKeys(erow.index, "ccw", root.textToKeys(text))
                                }
                            }
                        }
                    }

                    // General
                    RowLayout {
                        Layout.topMargin: Kirigami.Units.largeSpacing
                        PC3.Label { text: "Volver a stats tras"; Layout.fillWidth: true }
                        PC3.SpinBox {
                            from: 3
                            to: 600
                            value: root.rev >= 0 && root.draft ? (root.draft.idle_timeout_sec || 15) : 15
                            onValueModified: { root.draft.idle_timeout_sec = value; root.dirty = true }
                        }
                        PC3.Label { text: "s" }
                    }
                }
            }

            // Guardar / descartar
            RowLayout {
                Layout.fillWidth: true
                visible: root.draft !== null
                PC3.Label {
                    Layout.fillWidth: true
                    opacity: 0.7
                    text: root.dirty ? "Cambios sin guardar" : root.message
                    elide: Text.ElideRight
                }
                PC3.Button {
                    text: "Descartar"
                    icon.name: "edit-undo"
                    enabled: root.dirty
                    onClicked: root.loadConfig()
                }
                PC3.Button {
                    text: "Guardar"
                    icon.name: "document-save"
                    enabled: root.dirty
                    onClicked: root.saveConfig()
                }
            }
        }
    }
}
