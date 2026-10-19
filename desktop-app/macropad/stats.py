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
        gpu, gpu_temp = self._read_gpu()
        return {
            "cpu": int(round(psutil.cpu_percent(interval=None))),
            "ram": int(round(psutil.virtual_memory().percent)),
            "cpu_temp": self._read_cpu_temp(),
            "gpu": gpu,
            "gpu_temp": gpu_temp,
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

    def _read_gpu(self) -> tuple[int, int]:
        if not self._nvidia_smi:
            return 0, 0
        try:
            out = subprocess.run(
                [
                    self._nvidia_smi,
                    "--query-gpu=utilization.gpu,temperature.gpu",
                    "--format=csv,noheader,nounits",
                ],
                capture_output=True,
                text=True,
                timeout=1.0,
                check=True,
            ).stdout.strip()
            util_str, temp_str = out.split(",")
            return int(util_str.strip()), int(temp_str.strip())
        except Exception:
            return 0, 0
