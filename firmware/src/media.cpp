#include "media.h"

#include <mbedtls/base64.h>

static MediaInfo info;
static uint16_t artBuf[ART_SIZE * ART_SIZE];
static uint32_t receivingId = 0;  // tapa que se está recibiendo
static size_t receivedBytes = 0;
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

void mediaArtChunk(JsonDocument &doc) {
    uint32_t id = doc["id"] | 0;
    size_t off = doc["off"] | 0;
    const char *data = doc["d"] | "";
    if (id == 0) return;
    if (id != receivingId) {
        receivingId = id;
        receivedBytes = 0;
    }
    if (off >= ART_BYTES) return;
    size_t written = 0;
    uint8_t *dst = reinterpret_cast<uint8_t *>(artBuf) + off;
    if (mbedtls_base64_decode(dst, ART_BYTES - off, &written, reinterpret_cast<const unsigned char *>(data),
                              strlen(data)) != 0) {
        return;  // pedazo corrupto: la tapa queda incompleta y no se muestra
    }
    receivedBytes += written;
    if (receivedBytes >= ART_BYTES && readyId != id) {
        readyId = id;
        if (info.artId == id) changed = trackChanged = true;
    }
}

const MediaInfo &mediaInfo() { return info; }

bool mediaPlaying() { return info.active && info.playing; }

const uint16_t *mediaArt() {
    return (info.artId != 0 && readyId == info.artId && receivingId == readyId) ? artBuf : nullptr;
}

bool mediaConsumeChange(bool &track) {
    bool c = changed;
    track = trackChanged;
    changed = trackChanged = false;
    return c;
}
