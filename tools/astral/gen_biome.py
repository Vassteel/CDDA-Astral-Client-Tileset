#!/usr/bin/env python3
"""Compile Greenwood biome briefs; --check verifies byte-for-byte reproduction.

Content/scatter, landmarks and portal dimensions keep their existing generators.
This tool owns the files in OUTPUTS and generated/drowned_water.json.
"""
import argparse
import json
from pathlib import Path

from cddafmt import fmt
from wetland import templates

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / "tools/astral/biomes"
DEST = ROOT / "data/json/astral/dungeons"
OUTPUTS = {"regions": "region_astral_pocket.json", "overmap": "overmap.json",
           "mapgen": "mapgen_microworld.json"}
FIELDS = {"regions": "region", "overmap": "overmap", "mapgen": "mapgen"}


def compile_briefs(source=SOURCE):
    envelope = json.loads((source / "greenwood.json").read_text())
    if envelope["schema_version"] != 1:
        raise ValueError("unsupported biome schema")
    result = {}
    for section, filename in OUTPUTS.items():
        objects = []
        seen = set()
        for entry in envelope[section]:
            if "brief" in entry:
                name = entry["brief"]
                if not name.replace("_", "").isalnum():
                    raise ValueError("invalid brief name")
                brief = json.loads((source / (name + ".json")).read_text())
                if brief["schema_version"] != 1:
                    raise ValueError("unsupported brief schema")
                entry = brief[FIELDS[section]]
            identity = (entry["type"], str(entry.get("id", entry.get("om_terrain", entry.get("nested_mapgen_id", entry.get("update_mapgen_id"))))))
            if identity in seen:
                raise ValueError("duplicate biome definition: " + str(identity))
            seen.add(identity)
            objects.append(entry)
        result[filename] = fmt(objects, 0, 0) + "\n"
    lowland = json.loads((source / "lowlands.json").read_text())
    if lowland.get("wetland_variants"):
        count = lowland["wetland_variants"]
        if not isinstance(count, int) or not 1 <= count <= 32:
            raise ValueError("wetland_variants must be 1..32")
        result["generated/drowned_water.json"] = fmt(templates(count), 0, 0) + "\n"
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true")
    parser.add_argument("--output-dir", type=Path, default=DEST)
    args = parser.parse_args()
    mismatches = []
    outputs = compile_briefs()
    for filename, text in outputs.items():
        path = args.output_dir / filename
        if args.check:
            if not path.exists() or path.read_text() != text:
                mismatches.append(str(path))
        else:
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(text)
    if mismatches:
        parser.exit(1, "Biome output differs:\n" + "\n".join(mismatches) + "\n")
    print(f"Biome briefs: {len(outputs)} outputs " + ("match" if args.check else "written"))


if __name__ == "__main__":
    main()
