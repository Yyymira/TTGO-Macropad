const ext = globalThis.browser ?? chrome;
const t = (name) => ext.i18n.getMessage(name);
const statusEl = document.getElementById("status");
const pairBtn = document.getElementById("pair");
const unpairBtn = document.getElementById("unpair");
const help = document.getElementById("help");

pairBtn.textContent = t("pairButton");
unpairBtn.textContent = t("unpairButton");

async function refresh() {
    const s = await ext.runtime.sendMessage({ type: "status" });
    statusEl.textContent = t(s === "connected" ? "statusConnected" : s === "offline" ? "statusOffline" : "statusNotPaired");
    statusEl.className = s === "connected" ? "ok" : "";
    pairBtn.hidden = s !== "notPaired";
    unpairBtn.hidden = s === "notPaired";
    help.textContent = s === "notPaired" ? t("pairHelp") : "";
}

pairBtn.addEventListener("click", async () => {
    const ok = await ext.runtime.sendMessage({ type: "pair" });
    help.textContent = t(ok ? "pairOk" : "pairFailed");
    if (ok) setTimeout(refresh, 800);
});
unpairBtn.addEventListener("click", async () => {
    await ext.runtime.sendMessage({ type: "unpair" });
    refresh();
});
refresh();
