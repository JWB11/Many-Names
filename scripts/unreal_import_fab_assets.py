import glob
import json
import os
from pathlib import Path
import zipfile

import unreal


PROJECT_ROOT = Path(unreal.SystemLibrary.get_project_directory()).resolve()
VAULT_ROOT = Path("/Users/Shared/UnrealEngine/Launcher/VaultCache/FabLibrary")
STAGING_ROOT = PROJECT_ROOT / "Saved" / "FabImportStaging"
MANIFEST_PATH = PROJECT_ROOT / "Data" / "fab_asset_manifest.json"
INVENTORY_PATH = PROJECT_ROOT / "Docs" / "fab_asset_inventory.md"


ASSET_SELECTIONS = [
    {
        "id": "ancient_egypt_temple_collection",
        "title": "Ancient Egypt Temple Collection 3D model",
        "bucket": "EnvironmentEgypt",
        "destination": "/Game/Marketplace/Fab/EnvironmentEgypt/AncientEgyptTempleCollection",
        "patterns": [
            str(VAULT_ROOT / "Ancient_Egypt_Temple_Collection_3D_model-378dbcb6" / "fbx" / "source_extracted" / "*.fbx"),
        ],
    },
    {
        "id": "ancient_carved_stone",
        "title": "Ancient Carved Stone",
        "bucket": "PropsShared",
        "destination": "/Game/Marketplace/Fab/PropsShared/AncientCarvedStone",
        "patterns": [
            str(VAULT_ROOT / "Ancient_Carved_Stone-8adfac00" / "fbx" / "carvedstone01.fbx"),
        ],
    },
    {
        "id": "bent_pyramid_temple",
        "title": "Bent pyramid Temple | EGYPT",
        "bucket": "EnvironmentEgypt",
        "destination": "/Game/Marketplace/Fab/EnvironmentEgypt/BentPyramidTemple",
        "patterns": [
            str(VAULT_ROOT / "Bent_pyramid_Temple___EGYPT-ae5b97b7" / "obj" / "source_extracted" / "DAHSHUR.obj"),
        ],
    },
    {
        "id": "ptahshepses_mastaba_entrance",
        "title": "Ptahshepses Mastaba Entrance | EGYPT",
        "bucket": "EnvironmentEgypt",
        "destination": "/Game/Marketplace/Fab/EnvironmentEgypt/PtahshepsesMastabaEntrance",
        "patterns": [
            str(VAULT_ROOT / "Ptahshepses_Mastaba_Entrance___EGYPT-4326da7c" / "obj" / "source_extracted" / "ABUSIR.obj"),
        ],
    },
    {
        "id": "pyramid_shape_stone",
        "title": "Pyramid Shape Stone",
        "bucket": "PropsShared",
        "destination": "/Game/Marketplace/Fab/PropsShared/PyramidShapeStone",
        "patterns": [
            str(VAULT_ROOT / "Pyramid_Shape_Stone-b6ae8c5e" / "fbx" / "sm_pyramid_stone.fbx"),
        ],
    },
    {
        "id": "cave_rock",
        "title": "Cave Rock",
        "bucket": "PropsShared",
        "destination": "/Game/Marketplace/Fab/PropsShared/CaveRock",
        "patterns": [
            str(VAULT_ROOT / "Cave_Rock-345e1bb3" / "fbx" / "sm_stone_ga312251958.fbx"),
        ],
    },
    {
        "id": "highdetail_rock",
        "title": "HighDetail Rock",
        "bucket": "PropsShared",
        "destination": "/Game/Marketplace/Fab/PropsShared/HighDetailRock",
        "patterns": [
            str(VAULT_ROOT / "HighDetail_Rock-103df512" / "fbx" / "sm_stone_ga3120037.fbx"),
        ],
    },
    {
        "id": "realistic_rock",
        "title": "Realistic Rock",
        "bucket": "PropsShared",
        "destination": "/Game/Marketplace/Fab/PropsShared/RealisticRock",
        "patterns": [
            str(VAULT_ROOT / "Realistic_Rock-949dfcde" / "fbx" / "make_a_photorealistic_04_extracted" / "*" / "*.fbx"),
        ],
    },
    {
        "id": "rock_001",
        "title": "Rock_001",
        "bucket": "PropsShared",
        "destination": "/Game/Marketplace/Fab/PropsShared/Rock001",
        "patterns": [
            str(VAULT_ROOT / "Rock_001-32de8e98" / "fbx" / "source_extracted" / "Rock_001.fbx"),
        ],
    },
    {
        "id": "stone_age_dolmen",
        "title": "Stone Age Dolmen grave Klausebolle, Denmark",
        "bucket": "EnvironmentGreece",
        "destination": "/Game/Marketplace/Fab/EnvironmentGreece/StoneAgeDolmen",
        "patterns": [
            str(VAULT_ROOT / "Stone_Age_Dolmen_grave_Klauseb_lle__Denmark-ea0f7bdf" / "fbx" / "capeshj_lowpoly.fbx"),
        ],
    },
    {
        "id": "church_rock",
        "title": "Church rock",
        "bucket": "EnvironmentItalic",
        "destination": "/Game/Marketplace/Fab/EnvironmentItalic/ChurchRock",
        "zips": [
            {
                "zip_path": str(VAULT_ROOT / "Church_rock-f50bcb26" / "fbx" / "church_rock_extracted" / "source" / "church rock.zip"),
                "extract_dir": str(STAGING_ROOT / "church_rock"),
                "patterns": ["*.fbx"],
            },
        ],
    },
    {
        "id": "rounded_corner_bollard",
        "title": "Rounded corner bollard",
        "bucket": "EnvironmentItalic",
        "destination": "/Game/Marketplace/Fab/EnvironmentItalic/RoundedCornerBollard",
        "zips": [
            {
                "zip_path": str(VAULT_ROOT / "Rounded_corner_bollard-b595d70a" / "fbx" / "rounded_corner_bollard_extracted" / "source" / "round-corner.zip"),
                "extract_dir": str(STAGING_ROOT / "rounded_corner_bollard"),
                "patterns": ["*.fbx"],
            },
        ],
    },
    {
        "id": "cutted_destroyed_wood",
        "title": "Cutted And Destroyed Wood",
        "bucket": "EnvironmentConvergence",
        "destination": "/Game/Marketplace/Fab/EnvironmentConvergence/CuttedDestroyedWood",
        "zips": [
            {
                "zip_path": str(VAULT_ROOT / "Cutted_And_Destroyed_Wood-939419c2" / "obj" / "cutted_and_destroyed_woo_extracted" / "source" / "model.zip"),
                "extract_dir": str(STAGING_ROOT / "cutted_destroyed_wood"),
                "patterns": ["*.obj"],
            },
        ],
    },
    {
        "id": "human_fry_pose",
        "title": "External Wing -human fry pose-",
        "bucket": "ArtifactsHero",
        "destination": "/Game/Marketplace/Fab/ArtifactsHero/HumanFryPose",
        "patterns": [
            str(VAULT_ROOT / "External_Wing_-human_fry_pose--2c381dc6" / "fbx" / "ew_frypose_extracted" / "EW_frypose" / "AS.fbx"),
        ],
        "skeletal": True,
    },
]


def log(message: str) -> None:
    unreal.log(f"[FabImport] {message}")


def warn(message: str) -> None:
    unreal.log_warning(f"[FabImport] {message}")


def extract_zip(zip_path: Path, extract_dir: Path) -> None:
    extract_dir.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(zip_path, "r") as archive:
        archive.extractall(extract_dir)


def resolve_files(selection: dict) -> list[str]:
    resolved: list[str] = []

    for pattern in selection.get("patterns", []):
        resolved.extend(sorted(glob.glob(pattern)))

    for zip_spec in selection.get("zips", []):
        zip_path = Path(zip_spec["zip_path"])
        extract_dir = Path(zip_spec["extract_dir"])
        if not zip_path.exists():
            warn(f"Zip source missing: {zip_path}")
            continue
        extract_zip(zip_path, extract_dir)
        for pattern in zip_spec["patterns"]:
            resolved.extend(sorted(glob.glob(str(extract_dir / pattern))))

    return [path for path in resolved if Path(path).is_file()]


def build_task(source_file: str, destination_path: str, skeletal: bool) -> unreal.AssetImportTask:
    task = unreal.AssetImportTask()
    task.filename = source_file
    task.destination_path = destination_path
    task.automated = True
    task.replace_existing = True
    task.save = True

    extension = Path(source_file).suffix.lower()
    if extension == ".fbx":
        options = unreal.FbxImportUI()
        options.import_mesh = True
        options.import_textures = True
        options.import_materials = True
        options.import_as_skeletal = skeletal
        options.import_animations = False
        if skeletal:
            options.mesh_type_to_import = unreal.FBXImportType.FBXIT_SKELETAL_MESH
        else:
            options.mesh_type_to_import = unreal.FBXImportType.FBXIT_STATIC_MESH
            options.static_mesh_import_data.combine_meshes = False
            options.static_mesh_import_data.generate_lightmap_u_vs = True
        task.options = options

    return task


def write_manifest(manifest: list[dict]) -> None:
    MANIFEST_PATH.parent.mkdir(parents=True, exist_ok=True)
    with MANIFEST_PATH.open("w", encoding="utf-8") as handle:
        json.dump(manifest, handle, indent=2)
        handle.write("\n")


def write_inventory(markdown_rows: list[dict]) -> None:
    INVENTORY_PATH.parent.mkdir(parents=True, exist_ok=True)
    lines = [
        "# Fab Asset Inventory",
        "",
        "Imported from `/Users/Shared/UnrealEngine/Launcher/VaultCache/FabLibrary`.",
        "",
        "| Bucket | Title | Destination | Imported Objects |",
        "| --- | --- | --- | --- |",
    ]

    for row in markdown_rows:
        imported_objects = "<br>".join(row["imported_object_paths"]) if row["imported_object_paths"] else "_none_"
        lines.append(
            f"| {row['bucket']} | {row['title']} | `{row['destination']}` | `{imported_objects}` |"
        )

    INVENTORY_PATH.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> None:
    STAGING_ROOT.mkdir(parents=True, exist_ok=True)

    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    manifest_rows: list[dict] = []

    for selection in ASSET_SELECTIONS:
        source_files = resolve_files(selection)
        if not source_files:
            warn(f"No source files resolved for {selection['title']}")
            manifest_rows.append(
                {
                    "id": selection["id"],
                    "title": selection["title"],
                    "bucket": selection["bucket"],
                    "destination": selection["destination"],
                    "source_files": [],
                    "imported_object_paths": [],
                }
            )
            continue

        log(f"Importing {selection['title']} from {len(source_files)} source files")
        tasks = [build_task(source_file, selection["destination"], selection.get("skeletal", False)) for source_file in source_files]
        asset_tools.import_asset_tasks(tasks)

        imported_object_paths: list[str] = []
        for task in tasks:
            imported_object_paths.extend([str(path) for path in task.imported_object_paths])

        imported_object_paths = sorted(set(imported_object_paths))
        manifest_rows.append(
            {
                "id": selection["id"],
                "title": selection["title"],
                "bucket": selection["bucket"],
                "destination": selection["destination"],
                "source_files": source_files,
                "imported_object_paths": imported_object_paths,
            }
        )

    write_manifest(manifest_rows)
    write_inventory(manifest_rows)
    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    log(f"Wrote manifest to {MANIFEST_PATH}")


if __name__ == "__main__":
    main()
