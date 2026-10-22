// Corre en el mundo de la página (world: MAIN) porque los datos que la página
// le da al navegador para los controles multimedia (navigator.mediaSession)
// no se ven desde content.js. Solo los lee y se los pasa a content.js.
(() => {
    let last = "";
    setInterval(() => {
        const md = navigator.mediaSession && navigator.mediaSession.metadata;
        if (!md) return;
        // La portada más grande que ofrezca la página
        let artwork = "";
        let best = 0;
        for (const a of md.artwork || []) {
            const size = parseInt(String(a.sizes || "0").split("x")[0], 10) || 1;
            if (size >= best) { best = size; artwork = a.src; }
        }
        const data = { title: md.title || "", artist: md.artist || "", album: md.album || "", artwork };
        const json = JSON.stringify(data);
        if (json === last) return;
        last = json;
        window.postMessage({ __ttgoMacropad: "mediaSession", data }, "*");
    }, 1000);
})();
