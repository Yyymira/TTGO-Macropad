"""Ícono de bandeja: cerrar la ventana minimiza a la bandeja en vez de salir;
salir de verdad se hace desde el menú del ícono."""

from PySide6.QtGui import QIcon, QPixmap, QPainter, QColor
from PySide6.QtWidgets import QApplication, QMainWindow, QMenu, QSystemTrayIcon

from macropad.i18n import tr


def _fallback_icon() -> QIcon:
    pixmap = QPixmap(32, 32)
    pixmap.fill(QColor("#2e8b57"))
    painter = QPainter(pixmap)
    painter.setPen(QColor("white"))
    painter.drawText(pixmap.rect(), 0x84, "TT")  # Qt.AlignCenter
    painter.end()
    return QIcon(pixmap)


def install_tray(window: QMainWindow) -> QSystemTrayIcon:
    icon = QIcon.fromTheme("input-keyboard")
    if icon.isNull():
        icon = _fallback_icon()

    tray = QSystemTrayIcon(icon, window)
    tray.setToolTip("TTGO Macropad")

    menu = QMenu()
    show_action = menu.addAction(tr("Show window"))
    show_action.triggered.connect(window.showNormal)
    quit_action = menu.addAction(tr("Quit"))
    quit_action.triggered.connect(QApplication.instance().quit)
    tray.setContextMenu(menu)

    def on_activated(reason) -> None:
        if reason == QSystemTrayIcon.ActivationReason.Trigger:
            window.showNormal()
            window.raise_()
            window.activateWindow()

    tray.activated.connect(on_activated)
    tray.show()

    def close_event(event) -> None:
        event.ignore()
        window.hide()
        tray.showMessage("TTGO Macropad", tr("Still running in the tray."), icon, 2000)

    window.closeEvent = close_event
    return tray
