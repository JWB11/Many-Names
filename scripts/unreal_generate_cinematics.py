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


def ensure_camera_scaffold(sequence: unreal.LevelSequence, duration_frames: int) -> None:
    bindings = list(sequence.get_bindings())
    camera_binding = None
    for binding in bindings:
        try:
            if binding.get_display_name() == "HeroCamera":
                camera_binding = binding
                break
        except Exception:
            continue

    if camera_binding is None:
        camera_binding = sequence.add_spawnable_from_class(unreal.CineCameraActor)
        camera_binding.set_display_name("HeroCamera")

    has_transform_track = any(track.get_class().get_name() == "MovieScene3DTransformTrack" for track in camera_binding.get_tracks())
    if not has_transform_track:
        transform_track = camera_binding.add_track(unreal.MovieScene3DTransformTrack)
        transform_section = transform_track.add_section()
        transform_section.set_range(0, duration_frames)

    camera_cut_tracks = sequence.find_master_tracks_by_type(unreal.MovieSceneCameraCutTrack)
    if not camera_cut_tracks:
        camera_cut_track = sequence.add_master_track(unreal.MovieSceneCameraCutTrack)
        camera_cut_section = camera_cut_track.add_section()
        camera_cut_section.set_range(0, duration_frames)
        camera_binding_id = unreal.MovieSceneObjectBindingID()
        camera_binding_id.set_editor_property("guid", camera_binding.get_id())
        camera_cut_section.set_camera_binding_id(camera_binding_id)


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
    duration_frames = max(24, int(duration_seconds * 24.0))
    sequence.set_playback_start(0)
    sequence.set_playback_end(duration_frames)
    ensure_camera_scaffold(sequence, duration_frames)
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
