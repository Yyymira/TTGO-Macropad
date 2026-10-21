#include "i18n.h"

#include "settings.h"

// Sin tildes ni eñes: las fuentes de TFT_eSPI solo tienen ASCII.
static const char *const LANG_CODES[] = {"en", "es"};

static const char *const STRINGS[][static_cast<int>(Lang::Count)] = {
    // En                              Es
    {"Set up WiFi and city",          "Configura WiFi y ciudad"},
    {"from the PC panel",             "desde el panel de la PC"},
    {"Connecting to WiFi...",         "Conectando a WiFi..."},
    {"City not set",                  "Falta configurar la ciudad"},
    {"Getting time and weather...",   "Buscando hora y clima..."},
    {"No weather data",               "Sin datos del clima"},
    {"Hum",                           "Hum"},
    {"Today",                         "Hoy"},
    {"Waiting for data...",           "Esperando datos..."},
    {"Layer",                         "Capa"},
    {"Layer >>",                      "Capa >>"},
    {"<< Layer",                      "<< Capa"},
    {"Low battery",                   "Bateria baja"},
    {"left - plug in the charger",    "restante - conecta el cargador"},
    {"Updating...",                   "Actualizando..."},
    {"Done, restarting",              "Listo, reiniciando"},
    {"Update failed",                 "Error en la subida"},
    {"Clear",                         "Despejado"},
    {"Mostly clear",                  "Casi despejado"},
    {"Partly cloudy",                 "Algo nublado"},
    {"Cloudy",                        "Nublado"},
    {"Fog",                           "Niebla"},
    {"Drizzle",                       "Llovizna"},
    {"Rain",                          "Lluvia"},
    {"Showers",                       "Chaparrones"},
    {"Snow",                          "Nieve"},
    {"Storm",                         "Tormenta"},
};
static_assert(sizeof(STRINGS) / sizeof(STRINGS[0]) == static_cast<int>(Str::Count), "falta un texto en STRINGS");

static const char *const DAYS[][7] = {
    {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"},
    {"Dom", "Lun", "Mar", "Mie", "Jue", "Vie", "Sab"},
};

Lang langFromCode(const char *code) {
    if (!code) return Lang::En;
    for (int i = 0; i < static_cast<int>(Lang::Count); i++) {
        if (strncmp(code, LANG_CODES[i], 2) == 0) return static_cast<Lang>(i);
    }
    return Lang::En;
}

static int langIndex() {
    int i = static_cast<int>(settings.lang);
    return i < static_cast<int>(Lang::Count) ? i : 0;
}

const char *tr(Str s) { return STRINGS[static_cast<int>(s)][langIndex()]; }

const char *dayName(int wday) { return DAYS[langIndex()][wday % 7]; }

bool dayFirstDates() { return settings.lang != Lang::En; }
