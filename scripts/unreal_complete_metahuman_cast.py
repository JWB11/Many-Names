import json
import time
from pathlib import Path

import unreal


PROJECT_DIR = Path(unreal.Paths.project_dir())
DATA_DIR = PROJECT_DIR / "Data"
CAST_PATH = DATA_DIR / "character_cast.json"
AMBIENT_PATH = DATA_DIR / "ambient_profiles.json"
MANIFEST_PATH = DATA_DIR / "metahuman_manifest.json"

AUTHORING_ROOT = "/Game/Characters/MetaHumans"
BUILD_ROOT = "/Game/MetaHumans"
COMMON_ROOT = "/Game/MetaHumans/Common"
SKIP_IDS = {"ConvergenceCore", "LegacyProjection", "OracleFragment"}


def _load_json(path: Path):
    return json.loads(path.read_text(encoding="utf-8"))


def _save_json(path: Path, payload) -> None:
    path.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")


def _find_first_skeletal_mesh(package_path: str) -> str:
    asset_registry = unreal.AssetRegistryHelpers.get_asset_registry()
    assets = asset_registry.get_assets_by_path(package_path=package_path, recursive=True)
    meshes = [asset for asset in assets if asset.asset_class_path.asset_name == "SkeletalMesh"]
    meshes.sort(key=lambda item: str(item.package_name))
    if not meshes:
        return ""
    mesh = meshes[0]
    return f"{mesh.package_name}.{mesh.asset_name}"


def _open_for_edit(character, subsystem) -> bool:
    return subsystem.try_add_object_to_edit(character=character)


def _close_for_edit(character, subsystem) -> None:
    if subsystem.is_object_added_for_editing(character=character):
        subsystem.remove_object_to_edit(character=character)


def _request_auto_rig(character, subsystem) -> None:
    params = unreal.MetaHumanCharacterAutoRiggingRequestParams()
    params.blocking = True
    params.report_progress = False
    params.rig_type = unreal.MetaHumanRigType.JOINTS_ONLY
    subsystem.request_auto_rigging(character=character, params=params)


def _request_textures(character, subsystem) -> None:
    params = unreal.MetaHumanCharacterTextureRequestParams()
    params.blocking = True
    params.report_progress = False
    subsystem.request_texture_sources(character=character, params=params)


def _build_character(character, subsystem) -> None:
    params = unreal.MetaHumanCharacterEditorBuildParameters()
    params.pipeline_type = unreal.MetaHumanDefaultPipelineType.OPTIMIZED
    params.pipeline_quality = unreal.MetaHumanQualityLevel.HIGH
    params.absolute_build_path = f"{BUILD_ROOT}/{character.get_name()}"
    params.common_folder_path = COMMON_ROOT
    params.enable_wardrobe_item_validation = False
    subsystem.build_meta_human(character=character, params=params)


def _complete_character(character_id: str, manifest: dict, subsystem) -> None:
    asset_path = f"{AUTHORING_ROOT}/{character_id}.{character_id}"
    character = unreal.load_asset(asset_path)
    if not character:
        unreal.log_error(f"[MetaHumanCastComplete] Missing character asset {asset_path}")
        return

    manifest["named_character_assets"][character_id] = asset_path

    if not _open_for_edit(character, subsystem):
        unreal.log_error(f"[MetaHumanCastComplete] Failed to edit {character_id}")
        return

    try:
        for attempt in range(1, 4):
            unreal.log(f"[MetaHumanCastComplete] Auto-rigging {character_id} attempt {attempt}")
            _request_auto_rig(character, subsystem)
            if subsystem.can_build_meta_human(character=character) or character.has_high_resolution_textures:
                break
            time.sleep(5)

        for attempt in range(1, 4):
            if not character.has_high_resolution_textures:
                unreal.log(f"[MetaHumanCastComplete] Downloading textures for {character_id} attempt {attempt}")
                _request_textures(character, subsystem)
                if not character.has_high_resolution_textures:
                    time.sleep(5)
                    continue
            break

        if subsystem.can_build_meta_human(character=character):
            unreal.log(f"[MetaHumanCastComplete] Building assembled MetaHuman for {character_id}")
            _build_character(character, subsystem)
            mesh_path = _find_first_skeletal_mesh(f"{BUILD_ROOT}/{character_id}")
            if mesh_path:
                manifest["named"][character_id] = mesh_path
                unreal.log(f"[MetaHumanCastComplete] Built {character_id} -> {mesh_path}")
            else:
                unreal.log_warning(f"[MetaHumanCastComplete] No skeletal mesh found after build for {character_id}")
        else:
            unreal.log_warning(
                f"[MetaHumanCastComplete] {character_id} still not ready for assembly. "
                f"Textures={character.has_high_resolution_textures} FaceDNA={character.has_face_dna()}"
            )

        unreal.EditorAssetLibrary.save_loaded_asset(character)
    finally:
        _close_for_edit(character, subsystem)


def main():
    cast_records = _load_json(CAST_PATH)
    ambient_profiles = _load_json(AMBIENT_PATH)
    subsystem = unreal.get_editor_subsystem(unreal.MetaHumanCharacterEditorSubsystem)

    manifest = {
        "named": {},
        "ambient": {},
        "named_character_assets": {},
        "ambient_character_assets": {},
    }

    for record in cast_records:
        character_id = record["CharacterId"]
        if character_id in SKIP_IDS:
            continue
        _complete_character(character_id, manifest, subsystem)

    for profile in ambient_profiles:
        profile_id = profile["ProfileId"]
        for character_id in profile.get("CharacterIds", []):
            authored_asset = manifest["named_character_assets"].get(character_id)
            built_mesh = manifest["named"].get(character_id)
            if built_mesh:
                manifest["ambient"][profile_id] = built_mesh
                break
            if authored_asset:
                manifest["ambient_character_assets"][profile_id] = authored_asset
                break

    _save_json(MANIFEST_PATH, manifest)
    unreal.log(f"[MetaHumanCastComplete] Wrote manifest to {MANIFEST_PATH}")


if __name__ == "__main__":
    main()
