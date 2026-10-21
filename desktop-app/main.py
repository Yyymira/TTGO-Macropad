import sys

from PySide6.QtWidgets import QApplication

from macropad.active_window import start_active_window_watcher
from macropad.controller import Controller
from macropad.serial_link import SerialLink
from macropad.ui.main_window import MainWindow
from macropad.ui.tray import install_tray


def main() -> None:
    app = QApplication(sys.argv)
    app.setQuitOnLastWindowClosed(False)  # cerrar la ventana no mata la app: queda en la bandeja

    link = SerialLink()
    controller = Controller(link)
    window = MainWindow(link, controller)
    install_tray(window)
    window_watcher = start_active_window_watcher(controller)  # noqa: F841 (hay que mantenerlo vivo)

    link.start()
    controller.start()
    window.show()

    exit_code = app.exec()
    controller.shutdown()
    link.stop()
    sys.exit(exit_code)


if __name__ == "__main__":
    main()
