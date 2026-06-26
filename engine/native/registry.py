from __future__ import annotations


NATIVE_PROGRAMS = {
    "terrel": {
        "binary": "terrel",
        "suffix": ".x",
        "module": "terrain preprocessing",
        "required": True,
    },
    "ctgproc": {
        "binary": "ctgproc",
        "suffix": ".x",
        "module": "landuse preprocessing",
        "required": True,
    },
    "makegeo": {
        "binary": "makegeo",
        "suffix": ".x",
        "module": "geo preprocessing",
        "required": True,
    },
    "read62": {
        "binary": "read62",
        "suffix": ".x",
        "module": "upperair preprocessing",
        "required": True,
    },
    "smerge": {
        "binary": "smerge",
        "suffix": ".x",
        "module": "surface meteorology preprocessing",
        "required": True,
    },
    "met": {
        "binary": "calmet",
        "suffix": ".x",
        "display_name": "MET",
        "module": "diagnostic wind field",
        "required": True,
    },
    "puff": {
        "binary": "calpuff",
        "suffix": ".x",
        "display_name": "PUFF",
        "module": "puff dispersion model",
        "required": True,
    },
}


def iter_required_programs() -> list[tuple[str, dict[str, object]]]:
    return [(name, spec) for name, spec in NATIVE_PROGRAMS.items() if bool(spec.get("required", False))]
