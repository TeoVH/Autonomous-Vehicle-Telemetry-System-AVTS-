# Soporta:
# 1) "STATUS speed=... battery=... dir=... temp=..."
# 2) "DATA <speed> <battery> <dir> <temp>"

from typing import Dict

def parse(line: str) -> Dict[str, str]:
    result: Dict[str, str] = {}
    if not line:
        return result
    line = line.strip()

    if line.startswith("STATUS "):
        body = line.split(" ", 1)[1]
        for token in body.split():
            if "=" in token:
                k, v = token.split("=", 1)
                result[k.strip().lower()] = v.strip()
        return result

    if line.startswith("DATA "):
        parts = line.split()
        if len(parts) >= 5:
            result["speed"] = parts[1]
            result["battery"] = parts[2]
            result["dir"] = parts[3]
            result["temp"] = parts[4]
        return result

    return result
