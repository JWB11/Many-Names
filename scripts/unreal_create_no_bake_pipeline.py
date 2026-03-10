import json
from pathlib import Path

import unreal


PROJECT_DIR = Path(unreal.Paths.project_dir())
DATA_DIR = PROJECT_DIR / "Data"
STATUS_PATH = DATA_DIR / "metahuman_pipeline_status.json"

PIPELINE_ROOT = "/Game/MetaHuman/Pipelines"
SOURCE_ASSET = "/MetaHumanCharacter/BuildPipeline/BP_DefaultLegacyPipeline_Medium"
TARGET_ASSET = f"{PIPELINE_ROOT}/BP_ManyNamesLegacyPipeline_Medium_NoBake"
TARGET_CLASS = (
    "/Game/MetaHuman/Pipelines/BP_ManyNamesLegacyPipeline_Medium_NoBake."
    "BP_ManyNamesLegacyPipeline_Medium_NoBake_C"
)


def _engine_version() -> str:
    try:
        return unreal.SystemLibrary.get_engine_version()
    except Exception:
        return ""


def _save_status(payload: dict) -> None:
    STATUS_PATH.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")


def _ensure_directory(package_path: str) -> None:
    if not unreal.EditorAssetLibrary.does_directory_exist(package_path):
        unreal.EditorAssetLibrary.make_directory(package_path)


def _get_editor_pipeline(pipeline_asset):
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


def main():
    status = {
        "engine_version": _engine_version(),
        "override_asset_path": TARGET_ASSET,
        "override_class_path": TARGET_CLASS,
        "source_asset_path": SOURCE_ASSET,
        "created_override_asset": False,
        "saved_override_asset": False,
        "bake_materials_disabled": False,
        "verified_bake_materials_disabled": False,
        "error": "",
    }

    try:
        _ensure_directory(PIPELINE_ROOT)
        pipeline_asset = unreal.load_asset(TARGET_ASSET)
        if not pipeline_asset:
            source_asset = unreal.load_asset(SOURCE_ASSET)
            if not source_asset:
                raise RuntimeError(f"could not load source pipeline asset {SOURCE_ASSET}")
            pipeline_asset = unreal.AssetToolsHelpers.get_asset_tools().duplicate_asset(
                "BP_ManyNamesLegacyPipeline_Medium_NoBake",
                PIPELINE_ROOT,
                source_asset,
            )
            status["created_override_asset"] = bool(pipeline_asset)
        if not pipeline_asset:
            raise RuntimeError("failed to create project-owned no-bake pipeline asset")

        editor_pipeline = _get_editor_pipeline(pipeline_asset)
        if not editor_pipeline:
            raise RuntimeError("failed to resolve nested EditorPipeline object")

        editor_pipeline.set_editor_property("bBakeMaterials", False)
        status["bake_materials_disabled"] = True
        status["saved_override_asset"] = unreal.EditorAssetLibrary.save_loaded_asset(
            pipeline_asset, only_if_is_dirty=False
        )

        reloaded = unreal.load_asset(TARGET_ASSET)
        if not reloaded:
            raise RuntimeError("failed to reload saved no-bake pipeline asset")
        reloaded_editor_pipeline = _get_editor_pipeline(reloaded)
        if not reloaded_editor_pipeline:
            raise RuntimeError("failed to resolve nested EditorPipeline on reloaded asset")
        status["verified_bake_materials_disabled"] = (
            reloaded_editor_pipeline.get_editor_property("bBakeMaterials") is False
        )
        if not status["verified_bake_materials_disabled"]:
            raise RuntimeError("reloaded pipeline still has bBakeMaterials enabled")
    except Exception as exc:
        status["error"] = str(exc)
        _save_status(status)
        unreal.log_error(f"[MetaHumanNoBakePipeline] {exc}")
        raise

    _save_status(status)
    unreal.log(f"[MetaHumanNoBakePipeline] Ready: {TARGET_CLASS}")


if __name__ == "__main__":
    main()
