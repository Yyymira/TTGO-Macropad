#include "media.h"

#include <mbedtls/base64.h>

static MediaInfo info;
static uint16_t artBuf[ART_SIZE * ART_SIZE];
static uint32_t receivingId = 0;  // tapa que se está recibiendo
static uint32_t gotChunks = 0;    // un bit por pedazo recibido bien (los repetidos no molestan)
static uint32_t readyId = 0;      // tapa completa en artBuf
static bool changed = false, trackChanged = false;

void mediaSetInfo(JsonDocument &doc) {
    MediaInfo next;
    next.active = doc["active"] | false;
    next.playing = doc["playing"] | false;
    next.title = doc["title"] | "";
    next.artist = doc["artist"] | "";
    next.album = doc["album"] | "";
    next.pos = doc["pos"] | 0;
    next.len = doc["len"] | 0;
    next.artId = doc["art"] | 0;

    bool track = next.active != info.active || next.title != info.title || next.artist != info.artist ||
                 next.album != info.album || next.artId != info.artId;
    bool any = track || next.playing != info.playing || next.pos != info.pos || next.len != info.len;
    info = next;
    if (any) changed = true;
    if (track) trackChanged = true;
}

bool mediaArtChunk(JsonDocument &doc) {
    uint32_t id = doc["id"] | 0;
    size_t off = doc["off"] | 0;
    const char *data = doc["d"] | "";
    if (id == 0 || off >= ART_BYTES || off % ART_CHUNK != 0) return false;
    if (id != receivingId) {
        receivingId = id;
        gotChunks = 0;
    }
    size_t expected = min(ART_CHUNK, ART_BYTES - off);
    size_t written = 0;
    uint8_t *dst = reinterpret_cast<uint8_t *>(artBuf) + off;
    if (mbedtls_base64_decode(dst, ART_BYTES - off, &written, reinterpret_cast<const unsigned char *>(data),
                              strlen(data)) != 0 || written != expected) {
        return false;  // pedazo dañado: la PC lo reenvía
    }
    gotChunks |= 1UL << (off / ART_CHUNK);
    if (gotChunks == (1UL << ART_CHUNKS) - 1 && readyId != id) {
        readyId = id;
        if (info.artId == id) changed = trackChanged = true;
    }
    return true;
}

const MediaInfo &mediaInfo() { return info; }

bool mediaPlaying() { return info.active && info.playing; }

const uint16_t *mediaArt() {
    return (info.artId != 0 && readyId == info.artId && receivingId == readyId) ? artBuf : nullptr;
}

void mediaDebug(JsonObject out) {
    out["active"] = info.active;
    out["art"] = info.artId;
    out["ready"] = readyId;
    out["rx_id"] = receivingId;
    out["chunks"] = __builtin_popcount(gotChunks);
}

bool mediaConsumeChange(bool &track) {
    bool c = changed;
    track = trackChanged;
    changed = trackChanged = false;
    return c;
}
