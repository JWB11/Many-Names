import json
from pathlib import Path

import unreal


PROJECT_DIR = Path(unreal.Paths.project_dir())
DATA = PROJECT_DIR / "Data"


def load_json(name: str):
    return json.loads((DATA / name).read_text(encoding="utf-8"))


def ensure_directory(path: str) -> None:
    if not unreal.EditorAssetLibrary.does_directory_exist(path):
        unreal.EditorAssetLibrary.make_directory(path)


def create_sequence(asset_path: str, duration_seconds: float) -> None:
    package_path, asset_name = asset_path.rsplit("/", 1)
    package_path = package_path.rsplit(".", 1)[0]
    asset_name = asset_name.rsplit(".", 1)[0]
    if unreal.EditorAssetLibrary.does_asset_exist(f"{package_path}/{asset_name}"):
        sequence = unreal.load_asset(f"{package_path}/{asset_name}")
    else:
        ensure_directory(package_path)
        sequence = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            asset_name=asset_name,
            package_path=package_path,
            asset_class=unreal.LevelSequence,
            factory=unreal.LevelSequenceFactoryNew(),
        )
    sequence.set_playback_start(0)
    sequence.set_playback_end(int(duration_seconds * 24.0))
    unreal.EditorAssetLibrary.save_loaded_asset(sequence, only_if_is_dirty=False)


def main():
    scenes = load_json("cinematic_scenes.json")
    for scene in scenes:
        asset_path = scene.get("SequenceAsset", "")
        if asset_path.startswith("/Game/"):
            create_sequence(asset_path, float(scene.get("EstimatedDurationSeconds", 20.0)))
    unreal.log(f"[ManyNames] Generated placeholder Level Sequences for {len(scenes)} cinematic scenes")


if __name__ == "__main__":
    main()
