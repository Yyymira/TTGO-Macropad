"""Ventana principal: estado de conexión + log en vivo, y la pestaña de
configuración de capas/mapeos."""

import json
import time

from PySide6.QtCore import Slot
from PySide6.QtWidgets import (
    QLabel,
    QMainWindow,
    QPlainTextEdit,
    QTabWidget,
    QVBoxLayout,
    QWidget,
)

from macropad.controller import Controller
from macropad.i18n import tr
from macropad.serial_link import SerialLink
from macropad.ui.config_tab import ConfigTab


class StatusTab(QWidget):
    def __init__(self, link: SerialLink, controller: Controller):
        super().__init__()
        self.link = link
        self.controller = controller

        self.conn_label = QLabel(tr("Disconnected"))
        self.conn_label.setStyleSheet("color: red; font-weight: bold;")
        self.state_label = QLabel(tr("Mode: {mode} — Layer: {layer}", mode="idle", layer="-"))

        self.log = QPlainTextEdit()
        self.log.setReadOnly(True)
        self.log.setMaximumBlockCount(500)

        layout = QVBoxLayout(self)
        layout.addWidget(self.conn_label)
        layout.addWidget(self.state_label)
        layout.addWidget(self.log)

        self.link.connected_changed.connect(self.on_connected_changed)
        self.link.event_received.connect(self.on_event)
        self.controller.mode_changed.connect(self._refresh_state)
        self.controller.layer_changed.connect(self._refresh_state)

    @Slot(bool)
    def on_connected_changed(self, connected: bool) -> None:
        if connected:
            self.conn_label.setText(tr("Connected"))
            self.conn_label.setStyleSheet("color: green; font-weight: bold;")
        else:
            self.conn_label.setText(tr("Disconnected"))
            self.conn_label.setStyleSheet("color: red; font-weight: bold;")
        self._append(tr("[connection] {state}", state=tr("connected") if connected else tr("disconnected")))

    @Slot(dict)
    def on_event(self, event: dict) -> None:
        self._append(f"RX {json.dumps(event, ensure_ascii=False)}")

    def _refresh_state(self, *_args) -> None:
        layer = self.controller.current_layer.get("name", "-")
        self.state_label.setText(tr("Mode: {mode} — Layer: {layer}", mode=self.controller.mode, layer=layer))

    def _append(self, text: str) -> None:
        ts = time.strftime("%H:%M:%S")
        self.log.appendPlainText(f"{ts}  {text}")


class MainWindow(QMainWindow):
    def __init__(self, link: SerialLink, controller: Controller):
        super().__init__()
        self.setWindowTitle("TTGO Macropad")
        self.resize(760, 520)

        tabs = QTabWidget()
        tabs.addTab(StatusTab(link, controller), tr("Status"))
        tabs.addTab(ConfigTab(controller), tr("Settings"))
        self.setCentralWidget(tabs)
