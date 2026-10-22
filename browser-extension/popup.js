const ext = globalThis.browser ?? chrome;
const t = (name) => ext.i18n.getMessage(name);
const statusEl = document.getElementById("status");
const pairBtn = document.getElementById("pair");
const unpairBtn = document.getElementById("unpair");
const help = document.getElementById("help");

pairBtn.textContent = t("pairButton");
document.getElementById("pick").textContent = t("pickButton");
document.getElementById("targetsTitle").textContent = t("targetsTitle");

document.getElementById("pick").addEventListener("click", async () => {
    await ext.runtime.sendMessage({ type: "start_pick" });
    window.close(); // el modo selección sigue en la página
});

async function showTargets() {
    const list = document.getElementById("targets");
    const targets = await ext.runtime.sendMessage({ type: "targets" });
    list.replaceChildren();
    document.getElementById("empty").textContent = targets.length ? "" : t("noTargets");
    for (const target of targets) {
        const li = document.createElement("li");
        const name = document.createElement("span");
        name.textContent = target.name + " ";
        const siteEl = document.createElement("small");
        siteEl.textContent = target.site;
        name.appendChild(siteEl);
        const del = document.createElement("button");
        del.textContent = t("deleteTarget");
        del.addEventListener("click", async () => {
            await ext.runtime.sendMessage({ type: "delete_target", id: target.id });
            showTargets();
        });
        li.append(name, del);
        list.appendChild(li);
    }
}
showTargets();
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
