import json
import os
import time
from pathlib import Path

import unreal


PROJECT_DIR = Path(unreal.Paths.project_dir())
DATA_DIR = PROJECT_DIR / "Data"
CAST_PATH = DATA_DIR / "character_cast.json"
AMBIENT_PATH = DATA_DIR / "ambient_profiles.json"
MANIFEST_PATH = DATA_DIR / "metahuman_manifest.json"
STATUS_PATH = DATA_DIR / "metahuman_pipeline_status.json"

AUTHORING_ROOT = "/Game/Characters/MetaHumans"
BUILD_ROOT = "/Game/MetaHumans"
COMMON_ROOT = "/Game/MetaHumans/Common"
PIPELINE_ROOT = "/Game/MetaHuman/Pipelines"
DEFAULT_PIPELINE_SOURCE = "/MetaHumanCharacter/BuildPipeline/BP_DefaultLegacyPipeline_Medium"
DEFAULT_PIPELINE_ASSET = f"{PIPELINE_ROOT}/BP_ManyNamesLegacyPipeline_Medium_NoBake"
DEFAULT_PIPELINE_CLASS = (
    "/Game/MetaHuman/Pipelines/BP_ManyNamesLegacyPipeline_Medium_NoBake."
    "BP_ManyNamesLegacyPipeline_Medium_NoBake_C"
)
SKIP_IDS = {"ConvergenceCore", "LegacyProjection", "OracleFragment"}
BALANCED_BATCH = {
    "OpeningHealer",
    "OpeningQuartermaster",
    "TempleCantor",
    "GranarySteward",
    "SanctuaryKeeper",
    "WarSinger",
    "ForgeMatron",
    "RoadSurveyor",
    "CompanionChorus",
}
PIPELINE_OVERRIDE_ENV = "MH_PIPELINE_OVERRIDE_CLASS"


def _load_json(path: Path):
    return json.loads(path.read_text(encoding="utf-8"))


def _save_json(path: Path, payload) -> None:
    path.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")


def _engine_version() -> str:
    try:
        return unreal.SystemLibrary.get_engine_version()
    except Exception:
        return ""


def _save_status(payload: dict) -> None:
    _save_json(STATUS_PATH, payload)


def _ensure_directory(package_path: str) -> None:
    if not unreal.EditorAssetLibrary.does_directory_exist(package_path):
        unreal.EditorAssetLibrary.make_directory(package_path)


def _get_editor_pipeline(pipeline_asset) -> object:
    generated_class = pipeline_asset.generated_class() if hasattr(pipeline_asset, "generated_class") else None
    if not generated_class:
        return None
    cdo = unreal.get_default_object(generated_class)
    for candidate in ("editor_pipeline", "EditorPipeline"):
        try:
            editor_pipeline = cdo.get_editor_property(candidate)
            if editor_pipeline:
                return editor_pipeline
        except Exception:
            continue
    return None


def _ensure_no_bake_pipeline_override() -> tuple[str, dict]:
    status = {
        "engine_version": _engine_version(),
        "override_asset_path": DEFAULT_PIPELINE_ASSET,
        "override_class_path": DEFAULT_PIPELINE_CLASS,
        "source_asset_path": DEFAULT_PIPELINE_SOURCE,
        "created_override_asset": False,
        "saved_override_asset": False,
        "bake_materials_disabled": False,
        "verified_bake_materials_disabled": False,
        "error": "",
    }

    try:
        _ensure_directory(PIPELINE_ROOT)
        pipeline_asset = unreal.load_asset(DEFAULT_PIPELINE_ASSET)
        if not pipeline_asset:
            source_asset = unreal.load_asset(DEFAULT_PIPELINE_SOURCE)
            if not source_asset:
                status["error"] = "failed to load source legacy pipeline asset"
                return "", status
            pipeline_asset = unreal.AssetToolsHelpers.get_asset_tools().duplicate_asset(
                "BP_ManyNamesLegacyPipeline_Medium_NoBake",
                PIPELINE_ROOT,
                source_asset,
            )
            status["created_override_asset"] = bool(pipeline_asset)
        if not pipeline_asset:
            status["error"] = "failed to duplicate no-bake pipeline override asset"
            return "", status

        editor_pipeline = _get_editor_pipeline(pipeline_asset)
        if not editor_pipeline:
            status["error"] = "failed to resolve nested EditorPipeline object on override asset"
            return "", status

        editor_pipeline.set_editor_property("bBakeMaterials", False)
        status["bake_materials_disabled"] = True
        status["saved_override_asset"] = unreal.EditorAssetLibrary.save_loaded_asset(pipeline_asset, only_if_is_dirty=False)

        reloaded = unreal.load_asset(DEFAULT_PIPELINE_ASSET)
        if reloaded:
            reloaded_editor_pipeline = _get_editor_pipeline(reloaded)
            if reloaded_editor_pipeline:
                status["verified_bake_materials_disabled"] = (
                    reloaded_editor_pipeline.get_editor_property("bBakeMaterials") is False
                )
    except Exception as exc:
        status["error"] = str(exc)
        return "", status

    if not status["verified_bake_materials_disabled"]:
        status["error"] = status["error"] or "override asset saved, but no-bake verification failed"
        return "", status

    return DEFAULT_PIPELINE_CLASS, status


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
    params.pipeline_quality = unreal.MetaHumanQualityLevel.MEDIUM
    params.absolute_build_path = f"{BUILD_ROOT}/{character.get_name()}"
    params.common_folder_path = COMMON_ROOT
    params.enable_wardrobe_item_validation = False
    params.bake_makeup = True
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


def _refresh_ambient_manifest(ambient_profiles, manifest: dict) -> None:
    manifest["ambient"] = {}
    manifest["ambient_character_assets"] = {}
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


def _persist_manifest(manifest: dict, ambient_profiles) -> None:
    _refresh_ambient_manifest(ambient_profiles, manifest)
    _save_json(MANIFEST_PATH, manifest)
    unreal.log(f"[MetaHumanCastComplete] Persisted manifest to {MANIFEST_PATH}")


def main():
    cast_records = _load_json(CAST_PATH)
    ambient_profiles = _load_json(AMBIENT_PATH)
    subsystem = unreal.get_editor_subsystem(unreal.MetaHumanCharacterEditorSubsystem)
    existing_manifest = _load_json(MANIFEST_PATH) if MANIFEST_PATH.exists() else {}
    override_class, pipeline_status = _ensure_no_bake_pipeline_override()
    _save_status(pipeline_status)
    if override_class:
        unreal.log(
            "[MetaHumanCastComplete] No-bake pipeline asset verified. "
            "Build will rely on project MetaHumanCharacter config to select it."
        )
    else:
        unreal.log_warning(
            f"[MetaHumanCastComplete] No-bake pipeline override unavailable: {pipeline_status.get('error', 'unknown error')}"
        )

    manifest = {
        "named": dict(existing_manifest.get("named", {})),
        "ambient": {},
        "named_character_assets": dict(existing_manifest.get("named_character_assets", {})),
        "ambient_character_assets": dict(existing_manifest.get("ambient_character_assets", {})),
    }

    requested_ids = {
        item.strip()
        for item in os.environ.get("MH_CHARACTER_IDS", "").split(",")
        if item.strip()
    }

    prioritized_records = []
    deferred_records = []
    for record in cast_records:
        if requested_ids and record["CharacterId"] not in requested_ids:
            continue
        if record["CharacterId"] in BALANCED_BATCH:
            prioritized_records.append(record)
        else:
            deferred_records.append(record)

    for record in prioritized_records + deferred_records:
        character_id = record["CharacterId"]
        if character_id in SKIP_IDS:
            continue
        if character_id in manifest["named"]:
            authored_asset = manifest["named_character_assets"].get(character_id, f"{AUTHORING_ROOT}/{character_id}.{character_id}")
            manifest["named_character_assets"][character_id] = authored_asset
            _persist_manifest(manifest, ambient_profiles)
            continue
        _complete_character(character_id, manifest, subsystem)
        _persist_manifest(manifest, ambient_profiles)

    _persist_manifest(manifest, ambient_profiles)


if __name__ == "__main__":
    main()
