// Le avisa al servicio del TTGO Macropad (desktop-app/daemon.py) qué programa
// está en primer plano. En Wayland solo KWin lo sabe, por eso este script.
// Se ignoran paneles, menús y popups (como el del propio widget), para que
// abrir el widget no cambie la capa; el escritorio sí cuenta como "salir".

function report(window) {
    if (!window || !(window.normalWindow || window.dialog || window.desktopWindow)) {
        return;
    }
    callDBus("io.github.yyymira.TtgoMacropad", "/ActiveWindow",
             "io.github.yyymira.TtgoMacropad.ActiveWindow", "activeWindow",
             String(window.resourceClass || ""), String(window.resourceName || ""));
}

workspace.windowActivated.connect(report);
report(workspace.activeWindow);
