"""Lectura de recursos del sistema (CPU/RAM/temperaturas/GPU) para empujar
a la pantalla idle de la placa. GPU vía `nvidia-smi` si está disponible;
si no hay GPU NVIDIA, se reporta 0 sin romper nada."""

import shutil
import subprocess

import psutil


class StatsReader:
    def __init__(self) -> None:
        psutil.cpu_percent(interval=None)  # primera llamada "calienta" el contador
        self._nvidia_smi = shutil.which("nvidia-smi")

    def read(self) -> dict:
        gpu, gpu_temp, vram = self._read_gpu()
        return {
            "cpu": int(round(psutil.cpu_percent(interval=None))),
            "ram": int(round(psutil.virtual_memory().percent)),
            "cpu_temp": self._read_cpu_temp(),
            "gpu": gpu,
            "gpu_temp": gpu_temp,
            "vram": vram,
        }

    @staticmethod
    def _read_cpu_temp() -> int:
        try:
            temps = psutil.sensors_temperatures()
        except Exception:
            return 0
        for label in ("k10temp", "coretemp", "cpu_thermal"):
            entries = temps.get(label)
            if entries:
                return int(round(entries[0].current))
        for entries in temps.values():
            if entries:
                return int(round(entries[0].current))
        return 0

    def _read_gpu(self) -> tuple[int, int, int]:
        """(uso %, temperatura °C, VRAM usada %). VRAM -1 = sin GPU NVIDIA."""
        if not self._nvidia_smi:
            return 0, 0, -1
        try:
            out = subprocess.run(
                [
                    self._nvidia_smi,
                    "--query-gpu=utilization.gpu,temperature.gpu,memory.used,memory.total",
                    "--format=csv,noheader,nounits",
                ],
                capture_output=True,
                text=True,
                timeout=1.0,
                check=True,
            ).stdout.strip().splitlines()[0]
            util, temp, mem_used, mem_total = (float(v) for v in out.split(","))
            vram = int(round(mem_used * 100 / mem_total)) if mem_total > 0 else -1
            return int(util), int(temp), vram
        except Exception:
            return 0, 0, -1
