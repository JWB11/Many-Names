import json
from pathlib import Path

import unreal


PROJECT_DIR = Path(unreal.Paths.project_dir())
GENERATED = PROJECT_DIR / "Generated" / "AudioSources"
DEST_ROOT = "/Game/Audio/Generated"
AUDIO_PROFILES_PATH = PROJECT_DIR / "Data" / "audio_profiles.json"
VOICE_MANIFEST_PATH = GENERATED / "voice_manifest.json"


def ensure_directory(path: str) -> None:
    if not unreal.EditorAssetLibrary.does_directory_exist(path):
        unreal.EditorAssetLibrary.make_directory(path)


def import_file(source_path: Path, dest_path: str, dest_name: str) -> None:
    task = unreal.AssetImportTask()
    task.filename = str(source_path)
    task.destination_path = dest_path
    task.destination_name = dest_name
    task.automated = True
    task.replace_existing = True
    task.save = True
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])


def parse_asset_path(asset_path: str) -> tuple[str, str]:
    package_path = asset_path.split(".", 1)[0]
    destination_path, asset_name = package_path.rsplit("/", 1)
    return destination_path, asset_name


def load_audio_profiles() -> list[dict]:
    return json.loads(AUDIO_PROFILES_PATH.read_text())


def load_voice_manifest() -> dict:
    if not VOICE_MANIFEST_PATH.exists():
        return []
    return json.loads(VOICE_MANIFEST_PATH.read_text())


def main():
    ensure_directory(DEST_ROOT)

    for profile in load_audio_profiles():
        wav_path = GENERATED / profile["SourceFile"]
        if not wav_path.exists():
            unreal.log_warning(f"[ManyNames] Missing generated source for {profile['AudioId']}: {wav_path}")
            continue
        dest_path, dest_name = parse_asset_path(profile["SoundAsset"])
        ensure_directory(dest_path)
        import_file(wav_path, dest_path, dest_name)

    voice_manifest = load_voice_manifest()
    voice_dest = f"{DEST_ROOT}/Voices"
    ensure_directory(voice_dest)
    for metadata in voice_manifest:
        scene_id = metadata["SceneId"]
        wav_path = GENERATED / "voices" / metadata["File"]
        if wav_path.exists():
            import_file(wav_path, voice_dest, scene_id)

    unreal.EditorAssetLibrary.save_directory(DEST_ROOT, only_if_is_dirty=False, recursive=True)
    unreal.log(f"[ManyNames] Imported generated audio from {GENERATED}")


if __name__ == "__main__":
    main()
