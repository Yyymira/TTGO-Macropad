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

import "Translations.js" as T

PlasmoidItem {
    id: root

    readonly property string apiBase: "http://127.0.0.1:47823"
    readonly property var actionKinds: ["none", "key", "command", "layer_next", "layer_prev", "layer_set", "idle_view"]
    readonly property var actionNames: [T.tr("None"), T.tr("Key / shortcut"), T.tr("Run command"), T.tr("Next layer"), T.tr("Previous layer"), T.tr("Go to layer…"), T.tr("Switch idle screen")]
    readonly property var buttonNames: [1, 2, 3, 4, 5, 6].map(n => T.tr("Key {n}", {n: n})).concat([
                                        T.tr("Board (GPIO0)"), T.tr("Board (GPIO35)"),
                                        T.tr("Encoder 1 (press)"), T.tr("Encoder 2 (press)")])
    readonly property var encoderNames: [T.tr("Encoder 1 (back)"), T.tr("Encoder 2 (front)")]
    readonly property var idleViews: ["stats", "weather", "both"]
    readonly property var idleViewNames: [T.tr("Stats only"), T.tr("Clock and weather only"), T.tr("Both (alternating)")]

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
    toolTipSubText: !daemonUp ? T.tr("The service is not running")
                  : !status.connected ? T.tr("Board disconnected")
                  : T.tr("Active layer: {name}", {name: status.layer_name || ""})

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
                try { message = JSON.parse(xhr.responseText).error } catch (e) { message = T.tr("Error {code}", {code: xhr.status}) }
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
        api("POST", "/config", draft, (res) => {
            if (res.device) draft.device = res.device  // trae la ubicación resuelta
            dirty = false
            message = T.tr("Saved")
            rev++
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
    function setEncoderType(i, type) {
        const l = layerAt(editLayer)
        if (!l) return
        while (l.encoders.length <= i) l.encoders.push({cw: [], ccw: []})
        l.encoders[i].type = type
        dirty = true
        rev++
    }
    function deviceAt(_rev) { return (draft && draft.device) ? draft.device : {} }
    function setDevice(field, value) {
        if (!draft) return
        if (!draft.device) draft.device = {}
        draft.device[field] = value
        dirty = true
    }
    function netSummary() {
        const n = status.device_net || {}
        if (n.wifi === undefined) return ""
        const wifi = n.wifi ? T.tr("WiFi connected ({ip})", {ip: n.ip}) : T.tr("WiFi not connected")
        const ble = n.ble ? T.tr("phone connected via Bluetooth") : T.tr("Bluetooth waiting for a phone")
        return wifi + " · " + ble
    }
    function textToKeys(t) {
        return t.split(",").map(s => s.trim()).filter(s => s.length > 0)
    }
    function addLayer() {
        if (!draft) return
        const buttons = buttonNames.map(() => ({action: "none"}))
        const encoders = encoderNames.map(() => ({cw: [], ccw: []}))
        draft.layers.push({name: T.tr("Layer {n}", {n: draft.layers.length + 1}), buttons: buttons, encoders: encoders})
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
                    text: !root.daemonUp ? T.tr("Service stopped")
                        : root.status.connected ? T.tr("Board connected") : T.tr("Board disconnected")
                }
                PC3.Label {
                    visible: root.daemonUp && root.status.connected === true
                    opacity: 0.7
                    text: root.status.mode === "layer" ? T.tr("Showing: {name}", {name: root.status.layer_name}) : T.tr("Showing stats")
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
                text: T.tr("Can't reach the service. Start it with:") + "\nsystemctl --user start ttgo-macropad"
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
                    Kirigami.Heading { level: 5; text: T.tr("Layers") }
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
                                    return lrow.index + ": " + (l ? l.name : "") + (l && l.target === "phone" ? "  · BT" : "")
                                }
                                onClicked: { root.editLayer = lrow.index; root.rev++ }
                            }
                            PC3.ToolButton {
                                icon.name: "media-playback-start"
                                display: PC3.AbstractButton.IconOnly
                                text: T.tr("Show this layer on the board")
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
                            placeholderText: T.tr("Layer name")
                            text: { const l = root.layerAt(root.editLayer, root.rev); return l ? l.name : "" }
                            onTextEdited: {
                                const l = root.layerAt(root.editLayer)
                                if (l) { l.name = text; root.dirty = true }
                            }
                            onEditingFinished: root.rev++
                        }
                        PC3.ToolButton { icon.name: "list-add"; text: T.tr("Add"); onClicked: root.addLayer() }
                        PC3.ToolButton {
                            icon.name: "list-remove"
                            text: T.tr("Delete")
                            enabled: root.draft !== null && root.draft.layers.length > 1
                            onClicked: root.removeLayer()
                        }
                    }
                    RowLayout {
                        PC3.Label { text: T.tr("Keys go to"); Layout.fillWidth: true }
                        PC3.ComboBox {
                            model: [T.tr("PC"), T.tr("Phone (Bluetooth)")]
                            currentIndex: { const l = root.layerAt(root.editLayer, root.rev); return l && l.target === "phone" ? 1 : 0 }
                            onActivated: (i) => {
                                const l = root.layerAt(root.editLayer)
                                if (!l) return
                                l.target = i === 1 ? "phone" : "pc"
                                root.dirty = true
                                root.rev++
                            }
                        }
                    }
                    // Capas por programa: se activan solas con estos programas en primer plano.
                    PC3.Label { text: T.tr("Switch to this layer automatically with:") }
                    RowLayout {
                        PC3.TextField {
                            id: appsField
                            Layout.fillWidth: true
                            placeholderText: T.tr("e.g. firefox, org.kde.dolphin")
                            text: { const l = root.layerAt(root.editLayer, root.rev); return l && l.apps ? l.apps.join(", ") : "" }
                            onTextEdited: {
                                const l = root.layerAt(root.editLayer)
                                if (l) { l.apps = root.textToKeys(text); root.dirty = true }
                            }
                        }
                        PC3.ToolButton {
                            readonly property string app: root.status.active_app || ""
                            visible: app.length > 0
                            icon.name: "list-add"
                            text: app
                            PC3.ToolTip.text: T.tr("Add the program that was in the foreground")
                            PC3.ToolTip.visible: hovered
                            onClicked: {
                                const l = root.layerAt(root.editLayer)
                                if (!l) return
                                const apps = l.apps || []
                                if (apps.indexOf(app) < 0) apps.push(app)
                                l.apps = apps
                                root.dirty = true
                                root.rev++
                            }
                        }
                    }

                    // Botones
                    Kirigami.Heading { level: 5; text: T.tr("Buttons"); Layout.topMargin: Kirigami.Units.largeSpacing }
                    Repeater {
                        model: root.buttonNames.length
                        delegate: RowLayout {
                            id: brow
                            required property int index
                            readonly property var act: root.buttonAt(index, root.rev, root.editLayer)
                            Layout.fillWidth: true
                            PC3.Label {
                                text: root.buttonNames[brow.index]
                                Layout.preferredWidth: Kirigami.Units.gridUnit * 8
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
                                    if (kind === "command") a.cmd = ""
                                    if (kind === "layer_set") a.index = 0
                                    root.setButton(brow.index, a)
                                }
                            }
                            PC3.TextField {
                                visible: brow.act.action === "key"
                                Layout.fillWidth: true
                                placeholderText: T.tr("e.g. ctrl, t")
                                text: (brow.act.keys || []).join(", ")
                                onTextEdited: root.setButtonField(brow.index, "keys", root.textToKeys(text))
                            }
                            PC3.TextField {
                                visible: brow.act.action === "command"
                                Layout.fillWidth: true
                                placeholderText: T.tr("e.g. konsole  or  bash ~/scripts/thing.sh")
                                text: brow.act.cmd || ""
                                onTextEdited: root.setButtonField(brow.index, "cmd", text)
                            }
                            PC3.SpinBox {
                                visible: brow.act.action === "layer_set"
                                from: 0
                                to: root.draft ? root.draft.layers.length - 1 : 0
                                value: brow.act.index || 0
                                onValueModified: root.setButtonField(brow.index, "index", value)
                            }
                            Item {
                                visible: ["key", "command", "layer_set"].indexOf(brow.act.action) < 0
                                Layout.fillWidth: true
                            }
                        }
                    }

                    // Encoders
                    Kirigami.Heading { level: 5; text: T.tr("Encoders"); Layout.topMargin: Kirigami.Units.largeSpacing }
                    Repeater {
                        model: root.encoderNames.length
                        delegate: ColumnLayout {
                            id: erow
                            required property int index
                            readonly property var enc: root.encoderAt(index, root.rev, root.editLayer)
                            Layout.fillWidth: true
                            readonly property bool isCmd: enc.type === "command"
                            RowLayout {
                                Layout.fillWidth: true
                                PC3.Label { text: root.encoderNames[erow.index]; opacity: 0.8; Layout.fillWidth: true }
                                PC3.ComboBox {
                                    model: [T.tr("Keys"), T.tr("Commands")]
                                    currentIndex: erow.isCmd ? 1 : 0
                                    onActivated: (i) => root.setEncoderType(erow.index, i === 1 ? "command" : "keys")
                                }
                            }
                            RowLayout {
                                Layout.fillWidth: true
                                PC3.Label { text: "↻" }
                                PC3.TextField {
                                    Layout.fillWidth: true
                                    placeholderText: erow.isCmd ? T.tr("command on clockwise turn") : T.tr("clockwise, e.g. volumeup")
                                    text: erow.isCmd ? (erow.enc.cw_cmd || "") : (erow.enc.cw || []).join(", ")
                                    onTextEdited: erow.isCmd ? root.setEncoderKeys(erow.index, "cw_cmd", text)
                                                             : root.setEncoderKeys(erow.index, "cw", root.textToKeys(text))
                                }
                                PC3.Label { text: "↺" }
                                PC3.TextField {
                                    Layout.fillWidth: true
                                    placeholderText: erow.isCmd ? T.tr("command on counterclockwise turn") : T.tr("counterclockwise")
                                    text: erow.isCmd ? (erow.enc.ccw_cmd || "") : (erow.enc.ccw || []).join(", ")
                                    onTextEdited: erow.isCmd ? root.setEncoderKeys(erow.index, "ccw_cmd", text)
                                                             : root.setEncoderKeys(erow.index, "ccw", root.textToKeys(text))
                                }
                            }
                        }
                    }

                    // Dispositivo: pantalla de reposo, WiFi, clima
                    Kirigami.Heading { level: 5; text: T.tr("Device"); Layout.topMargin: Kirigami.Units.largeSpacing }
                    PC3.Label {
                        Layout.fillWidth: true
                        visible: text.length > 0
                        opacity: 0.7
                        wrapMode: Text.WordWrap
                        font: Kirigami.Theme.smallFont
                        text: root.netSummary()
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        PC3.Label { text: T.tr("Idle screen"); Layout.fillWidth: true }
                        PC3.ComboBox {
                            Layout.preferredWidth: Kirigami.Units.gridUnit * 11
                            model: root.idleViewNames
                            currentIndex: Math.max(0, root.idleViews.indexOf(root.deviceAt(root.rev).idle_view || "both"))
                            onActivated: (i) => { root.setDevice("idle_view", root.idleViews[i]); root.rev++ }
                        }
                    }
                    // Modo "ambos": cuánto dura cada pantalla (multimedia solo mientras suena algo)
                    GridLayout {
                        Layout.fillWidth: true
                        visible: (root.deviceAt(root.rev).idle_view || "both") === "both"
                        columns: 3
                        Repeater {
                            model: [["stats_secs", T.tr("Stats for")], ["weather_secs", T.tr("Weather for")],
                                    ["media_secs", T.tr("Media (while playing) for")]]
                            delegate: RowLayout {
                                required property var modelData
                                Layout.columnSpan: 3
                                PC3.Label { text: modelData[1]; Layout.fillWidth: true }
                                PC3.SpinBox {
                                    from: 2
                                    to: 3600
                                    value: root.deviceAt(root.rev)[modelData[0]] || 10
                                    onValueModified: root.setDevice(modelData[0], value)
                                }
                                PC3.Label { text: "s" }
                            }
                        }
                    }
                    PC3.Label {
                        Layout.fillWidth: true
                        opacity: 0.6
                        wrapMode: Text.WordWrap
                        font: Kirigami.Theme.smallFont
                        text: T.tr("Without the PC, the board always shows clock and weather, and keys go to the phone over Bluetooth.")
                    }
                    GridLayout {
                        Layout.fillWidth: true
                        columns: 2
                        PC3.Label { text: T.tr("WiFi network") }
                        PC3.TextField {
                            Layout.fillWidth: true
                            placeholderText: T.tr("network name (2.4 GHz)")
                            text: root.deviceAt(root.rev).wifi_ssid || ""
                            onTextEdited: root.setDevice("wifi_ssid", text)
                        }
                        PC3.Label { text: T.tr("Password") }
                        PC3.TextField {
                            Layout.fillWidth: true
                            echoMode: TextInput.Password
                            text: root.deviceAt(root.rev).wifi_pass || ""
                            onTextEdited: root.setDevice("wifi_pass", text)
                        }
                        PC3.Label { text: T.tr("City") }
                        PC3.TextField {
                            Layout.fillWidth: true
                            placeholderText: T.tr("e.g. London, United Kingdom")
                            text: root.deviceAt(root.rev).city || ""
                            onTextEdited: root.setDevice("city", text)
                        }
                    }
                    PC3.Label {
                        Layout.fillWidth: true
                        visible: (root.deviceAt(root.rev).place || "").length > 0
                        opacity: 0.7
                        wrapMode: Text.WordWrap
                        font: Kirigami.Theme.smallFont
                        text: T.tr("Weather for: {place}", {place: root.deviceAt(root.rev).place || ""})
                    }

                    // General
                    RowLayout {
                        Layout.topMargin: Kirigami.Units.largeSpacing
                        PC3.Label { text: T.tr("Back to idle after"); Layout.fillWidth: true }
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
                    text: root.dirty ? T.tr("Unsaved changes") : root.message
                    elide: Text.ElideRight
                }
                PC3.Button {
                    text: T.tr("Discard")
                    icon.name: "edit-undo"
                    enabled: root.dirty
                    onClicked: root.loadConfig()
                }
                PC3.Button {
                    text: T.tr("Save")
                    icon.name: "document-save"
                    enabled: root.dirty
                    onClicked: root.saveConfig()
                }
            }
        }
    }
}
