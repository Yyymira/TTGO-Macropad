#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>

// Lo que suena en la PC (MPRIS en Linux), para la pantalla de multimedia.
// La PC hace todo el trabajo: limpia los textos (solo ASCII, las fuentes no
// tienen tildes) y achica la tapa a ART_SIZE x ART_SIZE en RGB565, que manda
// en pedazos en base64 (mensajes "art"). La placa solo la guarda y la dibuja.
constexpr int ART_SIZE = 96;
constexpr size_t ART_BYTES = ART_SIZE * ART_SIZE * 2;
constexpr size_t ART_CHUNK = 2400;  // bytes por pedazo (igual que ART_CHUNK en desktop-app/macropad/media.py)
constexpr size_t ART_CHUNKS = (ART_BYTES + ART_CHUNK - 1) / ART_CHUNK;

struct MediaInfo {
    bool active = false;   // hay un reproductor con algo cargado
    bool playing = false;  // reproduciendo (en pausa = false)
    String title, artist, album;
    int32_t pos = 0, len = 0;  // segundos
    uint32_t artId = 0;        // 0 = sin tapa
};

void mediaSetInfo(JsonDocument &doc);   // mensaje "media"
// Mensaje "art": guarda un pedazo de la tapa. Devuelve si llegó bien, para
// confirmárselo a la PC (que si no, lo reenvía).
bool mediaArtChunk(JsonDocument &doc);

const MediaInfo &mediaInfo();
bool mediaPlaying();
const uint16_t *mediaArt();  // nullptr si la tapa del tema actual no llegó entera

// Qué cambió desde la última consulta: el tema/tapa (redibujar todo) o solo
// la posición/estado (redibujar la barra).
bool mediaConsumeChange(bool &trackChanged);

// Diagnóstico: tapa pedida, tapa completa, bytes recibidos de la que llega.
void mediaDebug(JsonObject out);
