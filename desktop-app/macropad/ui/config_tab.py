"""Editor de capas y mapeos de botones/encoders.
Los cambios quedan en un borrador local hasta apretar 'Guardar y aplicar',
que persiste a disco y le avisa al Controller para que lo use ya mismo."""

import copy

from PySide6.QtWidgets import (
    QComboBox,
    QFormLayout,
    QGridLayout,
    QGroupBox,
    QHBoxLayout,
    QInputDialog,
    QLabel,
    QLineEdit,
    QListWidget,
    QMessageBox,
    QPushButton,
    QSpinBox,
    QVBoxLayout,
    QWidget,
)

from macropad.config import NUM_BUTTONS, NUM_ENCODERS
from macropad.controller import Controller

ACTION_CHOICES = [
    ("Ninguna", "none"),
    ("Tecla / atajo", "key"),
    ("Ejecutar comando", "command"),
    ("Capa siguiente", "layer_next"),
    ("Capa anterior", "layer_prev"),
    ("Ir a capa N", "layer_set"),
]


def _action_to_combo_value(action: dict) -> tuple[str, str]:
    kind = action.get("action", "none")
    if kind == "key":
        return kind, ",".join(action.get("keys", []))
    if kind == "command":
        return kind, action.get("cmd", "")
    if kind == "layer_set":
        return kind, str(action.get("index", 0))
    return kind, ""


def _combo_value_to_action(kind: str, value: str) -> dict:
    if kind == "key":
        keys = [k.strip() for k in value.split(",") if k.strip()]
        return {"action": "key", "keys": keys}
    if kind == "command":
        return {"action": "command", "cmd": value.strip()}
    if kind == "layer_set":
        try:
            idx = int(value.strip() or 0)
        except ValueError:
            idx = 0
        return {"action": "layer_set", "index": idx}
    return {"action": kind}


class ButtonRow(QWidget):
    def __init__(self, label: str):
        super().__init__()
        self.combo = QComboBox()
        for text, _ in ACTION_CHOICES:
            self.combo.addItem(text)
        self.value_edit = QLineEdit()
        self.value_edit.setPlaceholderText("ej: ctrl,t  o  playpause  (o el comando a ejecutar)")

        row = QHBoxLayout(self)
        row.addWidget(QLabel(label), 0)
        row.addWidget(self.combo, 1)
        row.addWidget(self.value_edit, 2)

    def set_action(self, action: dict) -> None:
        kind, value = _action_to_combo_value(action)
        for i, (_, k) in enumerate(ACTION_CHOICES):
            if k == kind:
                self.combo.setCurrentIndex(i)
                break
        self.value_edit.setText(value)

    def get_action(self) -> dict:
        kind = ACTION_CHOICES[self.combo.currentIndex()][1]
        return _combo_value_to_action(kind, self.value_edit.text())


class EncoderRow(QWidget):
    def __init__(self, label: str):
        super().__init__()
        self.type_combo = QComboBox()
        self.type_combo.addItems(["Teclas", "Comandos"])
        self.cw_edit = QLineEdit()
        self.cw_edit.setPlaceholderText("horario, ej: volumeup")
        self.ccw_edit = QLineEdit()
        self.ccw_edit.setPlaceholderText("antihorario, ej: volumedown")

        row = QHBoxLayout(self)
        row.addWidget(QLabel(label), 0)
        row.addWidget(self.type_combo, 0)
        row.addWidget(QLabel("Horario:"))
        row.addWidget(self.cw_edit, 1)
        row.addWidget(QLabel("Antihorario:"))
        row.addWidget(self.ccw_edit, 1)

    def set_mapping(self, mapping: dict) -> None:
        if mapping.get("type") == "command":
            self.type_combo.setCurrentIndex(1)
            self.cw_edit.setText(mapping.get("cw_cmd", ""))
            self.ccw_edit.setText(mapping.get("ccw_cmd", ""))
        else:
            self.type_combo.setCurrentIndex(0)
            self.cw_edit.setText(",".join(mapping.get("cw", [])))
            self.ccw_edit.setText(",".join(mapping.get("ccw", [])))

    def get_mapping(self) -> dict:
        if self.type_combo.currentIndex() == 1:
            return {"type": "command", "cw": [], "ccw": [],
                    "cw_cmd": self.cw_edit.text().strip(), "ccw_cmd": self.ccw_edit.text().strip()}
        cw = [k.strip() for k in self.cw_edit.text().split(",") if k.strip()]
        ccw = [k.strip() for k in self.ccw_edit.text().split(",") if k.strip()]
        return {"cw": cw, "ccw": ccw}


class ConfigTab(QWidget):
    def __init__(self, controller: Controller):
        super().__init__()
        self.controller = controller
        self.draft = copy.deepcopy(controller.config)

        self.layer_list = QListWidget()
        btn_add = QPushButton("+ Capa")
        btn_add.clicked.connect(self.add_layer)
        btn_del = QPushButton("- Capa")
        btn_del.clicked.connect(self.remove_layer)
        btn_rename = QPushButton("Renombrar")
        btn_rename.clicked.connect(self.rename_layer)

        layer_buttons = QHBoxLayout()
        layer_buttons.addWidget(btn_add)
        layer_buttons.addWidget(btn_del)
        layer_buttons.addWidget(btn_rename)

        left = QVBoxLayout()
        left.addWidget(QLabel("Capas"))
        left.addWidget(self.layer_list)
        left.addLayout(layer_buttons)

        names = [f"Tecla {i + 1}" for i in range(6)] + ["Placa (GPIO0)", "Placa (GPIO35)",
                                                         "Encoder 1 (apretar)", "Encoder 2 (apretar)"]
        self.button_rows = [ButtonRow(name) for name in names[:NUM_BUTTONS]]
        buttons_box = QGroupBox("Botones de la capa seleccionada")
        buttons_layout = QVBoxLayout()
        for row in self.button_rows:
            buttons_layout.addWidget(row)
        buttons_box.setLayout(buttons_layout)

        enc_names = ["Encoder 1 (atrás)", "Encoder 2 (adelante)"]
        self.encoder_rows = [EncoderRow(name) for name in enc_names[:NUM_ENCODERS]]
        encoders_box = QGroupBox("Encoders de la capa seleccionada")
        encoders_layout = QVBoxLayout()
        for row in self.encoder_rows:
            encoders_layout.addWidget(row)
        encoders_box.setLayout(encoders_layout)

        self.idle_spin = QSpinBox()
        self.idle_spin.setRange(3, 600)
        self.idle_spin.setSuffix(" s")

        global_form = QFormLayout()
        global_form.addRow("Timeout de inactividad (vuelve a stats):", self.idle_spin)

        save_btn = QPushButton("Guardar y aplicar")
        save_btn.clicked.connect(self.save)

        right = QVBoxLayout()
        right.addWidget(buttons_box)
        right.addWidget(encoders_box)
        right.addLayout(global_form)
        right.addWidget(save_btn)

        root = QGridLayout(self)
        root.addLayout(left, 0, 0)
        root.addLayout(right, 0, 1)
        root.setColumnStretch(0, 1)
        root.setColumnStretch(1, 3)

        self.layer_list.currentRowChanged.connect(self._load_layer_into_form)
        self._refresh_layer_list()
        self.idle_spin.setValue(self.draft.get("idle_timeout_sec", 15))
        if self.draft["layers"]:
            self.layer_list.setCurrentRow(0)

    def _refresh_layer_list(self) -> None:
        self.layer_list.blockSignals(True)
        self.layer_list.clear()
        for layer in self.draft["layers"]:
            self.layer_list.addItem(layer.get("name", "Sin nombre"))
        self.layer_list.blockSignals(False)

    def _current_layer_index(self) -> int:
        return self.layer_list.currentRow()

    def _save_form_into_draft(self, index: int) -> None:
        if not (0 <= index < len(self.draft["layers"])):
            return
        layer = self.draft["layers"][index]
        layer["buttons"] = [row.get_action() for row in self.button_rows]
        layer["encoders"] = [row.get_mapping() for row in self.encoder_rows]

    def _load_layer_into_form(self, index: int) -> None:
        # Antes de cambiar de capa, persistimos lo editado de la anterior.
        prev = getattr(self, "_last_index", -1)
        if prev != -1:
            self._save_form_into_draft(prev)
        self._last_index = index

        if not (0 <= index < len(self.draft["layers"])):
            return
        layer = self.draft["layers"][index]
        buttons = layer.get("buttons", [])
        for i, row in enumerate(self.button_rows):
            row.set_action(buttons[i] if i < len(buttons) else {"action": "none"})
        encoders = layer.get("encoders", [])
        for i, row in enumerate(self.encoder_rows):
            row.set_mapping(encoders[i] if i < len(encoders) else {"cw": [], "ccw": []})

    def add_layer(self) -> None:
        name, ok = QInputDialog.getText(self, "Nueva capa", "Nombre de la capa:")
        if not ok or not name.strip():
            return
        self.draft["layers"].append(
            {
                "name": name.strip(),
                "buttons": [{"action": "none"} for _ in range(NUM_BUTTONS)],
                "encoders": [{"cw": [], "ccw": []} for _ in range(NUM_ENCODERS)],
            }
        )
        self._refresh_layer_list()
        self.layer_list.setCurrentRow(len(self.draft["layers"]) - 1)

    def remove_layer(self) -> None:
        index = self._current_layer_index()
        if index < 0 or len(self.draft["layers"]) <= 1:
            QMessageBox.warning(self, "No se puede borrar", "Tiene que quedar al menos una capa.")
            return
        del self.draft["layers"][index]
        self._last_index = -1
        self._refresh_layer_list()
        self.layer_list.setCurrentRow(0)

    def rename_layer(self) -> None:
        index = self._current_layer_index()
        if index < 0:
            return
        current_name = self.draft["layers"][index].get("name", "")
        name, ok = QInputDialog.getText(self, "Renombrar capa", "Nombre:", text=current_name)
        if ok and name.strip():
            self.draft["layers"][index]["name"] = name.strip()
            self._refresh_layer_list()
            self.layer_list.setCurrentRow(index)

    def save(self) -> None:
        self._save_form_into_draft(self._current_layer_index())
        self.draft["idle_timeout_sec"] = self.idle_spin.value()
        self.controller.reload_config(copy.deepcopy(self.draft))
        QMessageBox.information(self, "Listo", "Configuración guardada y aplicada.")
