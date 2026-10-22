// Corre en cada pestaña: informa qué suena (con los datos de page.js) y el
// estado de las reuniones, y ejecuta las órdenes del macropad sobre el video
// o la reunión de esta pestaña.
(() => {
    const ext = globalThis.browser ?? chrome;
    const host = location.hostname;
    let session = null; // datos de navigator.mediaSession que manda page.js
    let hadMedia = false;
    let hadMeeting = false;

    window.addEventListener("message", (e) => {
        if (e.source === window && e.data && e.data.__ttgoMacropad === "mediaSession") session = e.data.data;
    });

    const send = (msg) => { try { ext.runtime.sendMessage(msg).catch(() => {}); } catch { /* extensión recargada */ } };

    // --- multimedia ---
    // "Sonando" de verdad: en marcha y con sonido. Las vistas previas que
    // YouTube reproduce al pasar el mouse van silenciadas, y no cuentan.
    const isPlaying = (m) => !m.paused && !m.ended && !m.muted && m.volume > 0;

    function mediaElement() {
        const all = [...document.querySelectorAll("video, audio")].filter((m) => m.duration > 0 || !m.paused);
        if (!all.length) return null;
        // El que está sonando; si no, el video más grande
        const playing = all.find(isPlaying);
        if (playing) return playing;
        return all.sort((a, b) => (b.clientWidth * b.clientHeight) - (a.clientWidth * a.clientHeight))[0];
    }

    function cleanTitle(t) {
        return (t || "").replace(/^\(\d+\)\s*/, "").replace(/\s+-\s+YouTube( Music)?$/, "").trim();
    }

    function reportMedia() {
        const m = mediaElement();
        if (!m) {
            if (hadMedia) send({ type: "media", media: null });
            hadMedia = false;
            return;
        }
        hadMedia = true;
        const s = session || {};
        send({
            type: "media",
            media: {
                title: cleanTitle(s.title || document.title),
                artist: s.artist || "",
                album: s.album || "",
                artwork: s.artwork || "",
                playing: isPlaying(m),
                pos: Math.floor(m.currentTime || 0),
                len: Number.isFinite(m.duration) ? Math.floor(m.duration) : 0,
                rate: m.playbackRate,
            },
        });
    }

    // --- reuniones ---
    // Google Meet: los botones de micrófono y cámara llevan data-is-muted y su
    // atajo en el aria-label ("... (ctrl + d)" / "(ctrl + e)"), igual en todos
    // los idiomas. Teams web: experimental, por los ids de sus botones.
    function meetingButtons() {
        if (host === "meet.google.com") {
            const buttons = [...document.querySelectorAll("button[data-is-muted]")];
            const by = (k) => buttons.find((b) =>
                (b.getAttribute("aria-label") || "").toLowerCase().replace(/\s/g, "").includes("+" + k + ")"));
            const mic = by("d"), cam = by("e");
            if (!mic) return null;
            return { mic, cam, isOn: (b) => b && b.getAttribute("data-is-muted") === "false" };
        }
        if (/(^|\.)teams\.(microsoft|live)\.com$/.test(host)) {
            const mic = document.querySelector("#microphone-button, [data-tid='toggle-mute']");
            const cam = document.querySelector("#video-button, [data-tid='toggle-video']");
            if (!mic) return null;
            const isOn = (b) => b && (b.getAttribute("data-state") === "mic" || b.getAttribute("data-state") === "call-video" ||
                b.getAttribute("aria-checked") === "true" || b.getAttribute("aria-pressed") === "true");
            return { mic, cam, isOn };
        }
        return null;
    }

    function reportMeeting() {
        const m = meetingButtons();
        if (!m) {
            if (hadMeeting) send({ type: "meeting", meeting: null });
            hadMeeting = false;
            return;
        }
        hadMeeting = true;
        send({ type: "meeting", meeting: { mic: m.isOn(m.mic), cam: m.isOn(m.cam) } });
    }

    setInterval(() => { reportMedia(); reportMeeting(); }, 1000);

    // --- cartelito sobre la página (velocidad, volumen) ---
    let toastEl = null, toastTimer = 0;
    function toast(text, ms = 900) {
        if (!toastEl) {
            toastEl = document.createElement("div");
            toastEl.style.cssText = "position:fixed;left:50%;bottom:12%;transform:translateX(-50%);z-index:2147483647;" +
                "background:rgba(0,0,0,.8);color:#fff;font:600 18px system-ui,sans-serif;padding:8px 16px;" +
                "border-radius:8px;pointer-events:none;transition:opacity .2s";
            document.documentElement.appendChild(toastEl);
        }
        toastEl.textContent = text;
        toastEl.style.opacity = "1";
        clearTimeout(toastTimer);
        toastTimer = setTimeout(() => { toastEl.style.opacity = "0"; }, ms);
    }
    const msg = (name, value) => ext.i18n.getMessage(name, [String(value)]);

    // --- botones de la página elegidos por el usuario ("Clic en…") ---
    // No se guarda "el botón con este texto" (el de Me gusta incluye la cantidad
    // de likes, que cambia), sino una descripción estable: el tipo de elemento y
    // los contenedores con nombre propio que lo rodean (en YouTube, por ejemplo,
    // like-button-view-model). Si el sitio se rediseña, puede hacer falta volver
    // a elegirlo.
    const looksStable = (id) => id && !/\d{3,}|^[a-f0-9-]{10,}$|[:]/i.test(id);
    const STABLE_ATTRS = ["data-testid", "data-test-id", "data-tid", "data-action", "name"];

    function segment(el) {
        const tag = el.localName;
        if (looksStable(el.id)) return { sel: "#" + CSS.escape(el.id), stop: true };
        for (const a of STABLE_ATTRS) {
            const v = el.getAttribute(a);
            if (v && v.length < 60) return { sel: `${tag}[${a}="${CSS.escape(v)}"]`, named: true };
        }
        return { sel: tag, named: tag.includes("-") };
    }

    function describe(target) {
        const el = target.closest("button, a, [role=button], input, [tabindex]") || target;
        const parts = [];
        for (let node = el; node && node !== document.documentElement && parts.length < 6; node = node.parentElement) {
            const seg = segment(node);
            if (node === el || seg.named || seg.stop) parts.unshift(seg.sel);
            if (seg.stop) break;
        }
        const selector = parts.join(" ");
        const matches = [...document.querySelectorAll(selector)];
        const label = (el.getAttribute("aria-label") || el.getAttribute("title") || "").trim();
        return {
            selector,
            index: Math.max(0, matches.indexOf(el)),
            hint: { tag: el.localName, label: label.split(/\s+/).slice(0, 3).join(" "), text: (el.innerText || "").trim().slice(0, 30) },
            suggestedName: (label || el.innerText || "").trim().split(/\s+/).slice(0, 3).join(" ").slice(0, 24),
        };
    }

    function findTarget(t) {
        const matches = document.querySelectorAll(t.selector);
        if (matches.length) return matches[Math.min(t.index || 0, matches.length - 1)];
        // Plan B: mismo tipo de elemento y misma descripción o texto
        const h = t.hint || {};
        return [...document.querySelectorAll(h.tag || "button")].find((e) =>
            (h.label && (e.getAttribute("aria-label") || "").startsWith(h.label)) ||
            (h.text && (e.innerText || "").trim().startsWith(h.text))) || null;
    }

    // Modo selección: resalta lo que está bajo el mouse; clic = elegir, Esc = cancelar
    let picking = false;
    function startPicker() {
        if (picking) return;
        picking = true;
        const box = document.createElement("div");
        box.style.cssText = "position:fixed;z-index:2147483646;pointer-events:none;border:2px solid #c8363c;" +
            "background:rgba(200,54,60,.15);border-radius:4px;transition:all .05s";
        document.documentElement.appendChild(box);
        toast(ext.i18n.getMessage("pickHelp"), 4000);
        const over = (e) => {
            const r = (e.target.closest("button, a, [role=button], input, [tabindex]") || e.target).getBoundingClientRect();
            Object.assign(box.style, { left: r.left + "px", top: r.top + "px", width: r.width + "px", height: r.height + "px" });
        };
        const stop = () => {
            picking = false;
            box.remove();
            document.removeEventListener("mouseover", over, true);
            document.removeEventListener("click", click, true);
            document.removeEventListener("keydown", key, true);
        };
        const click = (e) => {
            e.preventDefault();
            e.stopPropagation();
            stop();
            const d = describe(e.target);
            const name = prompt(ext.i18n.getMessage("pickName"), d.suggestedName || location.hostname);
            if (name === null) return;
            send({ type: "target_saved", target: { name: name.trim().slice(0, 24) || d.suggestedName,
                site: location.hostname.replace(/^www\./, ""), selector: d.selector, index: d.index, hint: d.hint } });
            toast(ext.i18n.getMessage("pickSaved", [name.trim() || d.suggestedName]));
        };
        const key = (e) => { if (e.key === "Escape") { e.preventDefault(); stop(); } };
        document.addEventListener("mouseover", over, true);
        document.addEventListener("click", click, true);
        document.addEventListener("keydown", key, true);
    }

    // --- órdenes del macropad ---
    const SPEEDS = [0.25, 0.5, 0.75, 1, 1.25, 1.5, 1.75, 2, 2.5, 3];
    ext.runtime.onMessage.addListener((m) => {
        if (!m || m.type !== "command") return;
        const a = m.action;
        if (a === "pick") { startPicker(); return; }
        if (a === "click") {
            const el = m.target && findTarget(m.target);
            if (el) el.click();
            else toast(ext.i18n.getMessage("clickMissing", [m.target ? m.target.name : "?"]), 3000);
            return;
        }
        if (a === "toggle_mic" || a === "toggle_cam") {
            const meet = meetingButtons();
            const b = meet && (a === "toggle_mic" ? meet.mic : meet.cam);
            if (b) b.click();
            setTimeout(reportMeeting, 300);
            return;
        }
        if (a === "skip_ad") {
            const skip = document.querySelector(".ytp-skip-ad-button, .ytp-ad-skip-button, .ytp-ad-skip-button-modern");
            if (skip) skip.click();
            return;
        }
        const v = mediaElement();
        if (!v) return;
        if (a === "speed_up" || a === "speed_down") {
            const r = v.playbackRate;
            const next = a === "speed_up" ? (SPEEDS.find((s) => s > r + 0.001) ?? SPEEDS[SPEEDS.length - 1])
                                          : ([...SPEEDS].reverse().find((s) => s < r - 0.001) ?? SPEEDS[0]);
            v.playbackRate = next;
            toast(msg("toastSpeed", next));
        } else if (a === "speed_reset") {
            v.playbackRate = 1;
            toast(msg("toastSpeed", 1));
        } else if (a === "seek_forward" || a === "seek_back") {
            const t = v.currentTime + (a === "seek_forward" ? 10 : -10);
            v.currentTime = Math.max(0, Number.isFinite(v.duration) ? Math.min(v.duration - 0.5, t) : t);
        } else if (a === "tab_volume_up" || a === "tab_volume_down") {
            v.volume = Math.round(Math.min(1, Math.max(0, v.volume + (a === "tab_volume_up" ? 0.1 : -0.1))) * 10) / 10;
            toast(msg("toastVolume", Math.round(v.volume * 100)));
        }
        reportMedia();
    });
})();
