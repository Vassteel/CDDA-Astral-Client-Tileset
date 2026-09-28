"""Copy the standard artwork and audio assets, excluding optional Astral artwork."""
from pathlib import Path
import shutil

STANDARD_TILESETS = (
    'ASCIITileset', 'ASCII_Overmap', 'Altica', 'BrownLikeBears', 'ChibiUltica',
    'Cuteclysm', 'GiantDays', 'HollowMoon', 'Larwick_Overmap', 'MshockXotto+',
    'NeoDaysTileset', 'PenAndPaper', 'RetroDaysTileset', 'SmashButton_iso',
    'SurveyorsMap', 'Test_Portrait_Pack', 'Ultica_iso', 'UltimateCataclysm',
)


def copy_standard_assets(runtime: Path, destination: Path):
    for name in STANDARD_TILESETS:
        source = runtime / 'gfx' / name
        if source.is_dir():
            shutil.copytree(source, destination / 'gfx' / name, dirs_exist_ok=True)
    for name in ('Basic', 'CC-Sounds'):
        candidates = (runtime / 'data/sound' / name, runtime / 'sound' / name,
                      runtime / 'sound' / name / name)
        source = next((p for p in candidates if (p / 'soundpack.txt').is_file()), None)
        if source is None:
            raise ValueError('Missing sound pack: ' + name)
        shutil.copytree(source, destination / 'data/sound' / name, dirs_exist_ok=True)
