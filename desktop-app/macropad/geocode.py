"""Busca las coordenadas de una ciudad con el geocoder de Open-Meteo (gratis,
sin API key), para que la placa pida el clima de ese lugar.

Acepta "Ciudad" o "Ciudad, provincia/país" — lo que va después de la coma
sirve para elegir entre homónimas (ej: "Córdoba, Argentina" vs España)."""

import json
import urllib.parse
import urllib.request

_URL = "https://geocoding-api.open-meteo.com/v1/search"


class CityNotFound(LookupError):
    pass


def geocode(query: str) -> tuple[float, float, str]:
    name, _, qualifier = query.partition(",")
    name, qualifier = name.strip(), qualifier.strip().lower()
    params = urllib.parse.urlencode({"name": name, "count": 10, "language": "es", "format": "json"})
    with urllib.request.urlopen(f"{_URL}?{params}", timeout=8) as resp:
        results = json.load(resp).get("results") or []
    if not results:
        raise CityNotFound(query)

    def matches(r: dict) -> bool:
        fields = (r.get("country", ""), r.get("admin1", ""), r.get("admin2", ""), r.get("country_code", ""))
        return any(qualifier in f.lower() for f in fields if f)

    best = next((r for r in results if matches(r)), None) if qualifier else results[0]
    if best is None:
        raise CityNotFound(query)
    place = ", ".join(p for p in (best.get("name"), best.get("admin1"), best.get("country")) if p)
    return best["latitude"], best["longitude"], place
