// Fondo de la extensión: junta lo que informan las pestañas (content.js), se
// lo pasa al servicio del macropad y ejecuta las órdenes que llegan de los
// botones con la acción "Navegador". Ver desktop-app/macropad/browser.py.

const API = "http://127.0.0.1:47823";
const ext = globalThis.browser ?? chrome;

let key = null;
const mediaTabs = new Map();   // tabId -> {media, at}
const meetingTabs = new Map(); // tabId -> {meeting, at}
let site = "";
let lastSent = "";
let lastSentAt = 0;

const sleep = (ms) => new Promise((r) => setTimeout(r, ms));
let targets = []; // botones de páginas elegidos por el usuario: {id, name, site, selector, index, hint}
const keyReady = ext.storage.local.get(["key", "targets"]).then((v) => {
    key = v.key || null;
    targets = v.targets || [];
});

async function saveTargets() {
    await ext.storage.local.set({ targets });
    lastSent = "";
    push();
}

function hostOf(url) {
    try {
        const u = new URL(url);
        return u.protocol.startsWith("http") ? u.hostname.replace(/^www\./, "") : "";
    } catch {
        return "";
    }
}

// --- qué sitio está al frente (solo si la ventana del navegador tiene el foco) ---
async function updateSite() {
    let next = "";
    try {
        const win = await ext.windows.getLastFocused();
        if (win && win.focused) {
            const [tab] = await ext.tabs.query({ active: true, windowId: win.id });
            next = tab ? hostOf(tab.url || "") : "";
        }
    } catch { /* sin ventanas */ }
    if (next !== site) {
        site = next;
        push();
    }
}
ext.windows.onFocusChanged.addListener(updateSite);
ext.tabs.onActivated.addListener(updateSite);
ext.tabs.onUpdated.addListener((_id, info) => { if (info.url) updateSite(); });
ext.tabs.onRemoved.addListener((tabId) => {
    mediaTabs.delete(tabId);
    meetingTabs.delete(tabId);
    push();
});

// --- la pestaña con multimedia / reunión más relevante ---
// La más reciente, dando prioridad a las que cumplen `prefer` (ej: las que están sonando).
function freshest(map, maxAgeMs, prefer = () => false) {
    const now = Date.now();
    let best = null;
    for (const [tabId, v] of map) {
        if (now - v.at > maxAgeMs) { map.delete(tabId); continue; }
        const rank = [prefer(v) ? 1 : 0, v.at];
        if (!best || rank[0] > best.rank[0] || (rank[0] === best.rank[0] && rank[1] > best.rank[1])) {
            best = { tabId, v, rank };
        }
    }
    return best;
}
// Primero la pestaña que el navegador marca con sonido (el parlantito), después las que reproducen.
const mediaTab = () => freshest(mediaTabs, 5000, (v) => v.media.playing && v.audible) ||
    freshest(mediaTabs, 5000, (v) => v.media.playing);
const meetingTab = () => freshest(meetingTabs, 5000);

// --- estado -> servicio ---
async function push() {
    await keyReady;
    if (!key) return;
    const m = mediaTab();
    const meet = meetingTab();
    const body = JSON.stringify({
        site, media: m ? m.v.media : null, meeting: meet ? meet.v.meeting : null,
        targets: targets.map((t) => ({ id: t.id, name: t.name, site: t.site })),
    });
    // Sin cambios no se manda, salvo un refresco cada 2 s (para que el servicio sepa que sigue).
    if (body === lastSent && Date.now() - lastSentAt < 2000) return;
    lastSent = body;
    lastSentAt = Date.now();
    try {
        const r = await fetch(API + "/browser/state", {
            method: "POST", headers: { "Content-Type": "application/json", "X-Macropad-Key": key }, body,
        });
        if (r.status === 401) await forgetKey();
    } catch { /* servicio apagado: se reintenta con el próximo cambio */ }
}

ext.runtime.onMessage.addListener((msg, sender, reply) => {
    const tabId = sender.tab && sender.tab.id;
    if (msg.type === "media" && tabId !== undefined) {
        if (msg.media) mediaTabs.set(tabId, { media: msg.media, audible: !!sender.tab.audible, at: Date.now() });
        else mediaTabs.delete(tabId);
        push();
    } else if (msg.type === "meeting" && tabId !== undefined) {
        if (msg.meeting) meetingTabs.set(tabId, { meeting: msg.meeting, at: Date.now() });
        else meetingTabs.delete(tabId);
        push();
    } else if (msg.type === "target_saved") {
        const t = { ...msg.target, id: Date.now().toString(36) };
        targets.push(t);
        saveTargets();
    } else if (msg.type === "targets") {
        keyReady.then(() => reply(targets));
        return true;
    } else if (msg.type === "delete_target") {
        targets = targets.filter((t) => t.id !== msg.id);
        saveTargets().then(() => reply(true));
        return true;
    } else if (msg.type === "start_pick") {
        ext.tabs.query({ active: true, currentWindow: true }).then(([tab]) => {
            if (tab) ext.tabs.sendMessage(tab.id, { type: "command", action: "pick" }).catch(() => {});
        });
    } else if (msg.type === "pair") {
        pair().then(reply);
        return true;
    } else if (msg.type === "unpair") {
        forgetKey().then(() => reply(true));
        return true;
    } else if (msg.type === "status") {
        status().then(reply);
        return true;
    }
});

// --- vinculación ---
async function pair() {
    try {
        const r = await fetch(API + "/browser/pair", { method: "POST" });
        if (!r.ok) return false;
        const data = await r.json();
        key = data.key;
        await ext.storage.local.set({ key });
        lastSent = "";
        updateSite();
        return true;
    } catch {
        return false;
    }
}

async function forgetKey() {
    key = null;
    await ext.storage.local.remove("key");
}

async function status() {
    await keyReady;
    if (!key) return "notPaired";
    try {
        const r = await fetch(API + "/browser/state", {
            method: "POST", headers: { "Content-Type": "application/json", "X-Macropad-Key": key },
            body: JSON.stringify({ site }),
        });
        if (r.status === 401) { await forgetKey(); return "notPaired"; }
        return r.ok ? "connected" : "offline";
    } catch {
        return "offline";
    }
}

// --- órdenes del macropad ---
// Pestaña donde hacer clic en un botón guardado: la que está al frente si es
// de ese sitio, si no la que está sonando, si no cualquiera del sitio.
async function tabForSite(siteName) {
    const matches = (tab) => { const h = hostOf(tab.url || ""); return h === siteName || h.endsWith("." + siteName); };
    try {
        const win = await ext.windows.getLastFocused();
        const [active] = await ext.tabs.query({ active: true, windowId: win.id });
        if (active && matches(active)) return active.id;
    } catch { /* sin ventanas */ }
    const m = mediaTab();
    if (m) {
        const tab = await ext.tabs.get(m.tabId).catch(() => null);
        if (tab && matches(tab)) return tab.id;
    }
    const any = (await ext.tabs.query({})).find(matches);
    return any ? any.id : null;
}

async function runCommand(action, command = {}) {
    if (action === "click") {
        await keyReady;
        const t = targets.find((x) => x.id === command.target);
        const tabId = t && await tabForSite(t.site);
        if (tabId !== null && tabId !== undefined) ext.tabs.sendMessage(tabId, { type: "command", action: "click", target: t }).catch(() => {});
        return;
    }
    if (action === "focus_media_tab") {
        const m = mediaTab();
        if (!m) return;
        const tab = await ext.tabs.update(m.tabId, { active: true });
        await ext.windows.update(tab.windowId, { focused: true });
        return;
    }
    const target = action === "toggle_mic" || action === "toggle_cam" ? meetingTab() : mediaTab();
    if (target) ext.tabs.sendMessage(target.tabId, { type: "command", action }).catch(() => {});
}

async function pollLoop() {
    await keyReady;
    for (;;) {
        // Llamar a una API de la extensión mantiene vivo el service worker (Chrome 110+).
        ext.runtime.getPlatformInfo();
        if (!key) { await sleep(3000); continue; }
        try {
            const r = await fetch(API + "/browser/poll", { headers: { "X-Macropad-Key": key } });
            if (r.status === 401) { await forgetKey(); continue; }
            const data = await r.json();
            for (const c of data.commands || []) runCommand(c.action, c);
        } catch {
            await sleep(3000); // servicio apagado
        }
    }
}

pollLoop();
updateSite();
