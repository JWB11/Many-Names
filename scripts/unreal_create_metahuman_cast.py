import hashlib
import json
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

OUTFIT_SLOT = "Outfits"
GROOM_SLOTS = ["Hair", "Eyebrows", "Beard", "Mustache", "Eyelashes", "Peachfuzz"]
SKIP_FULL_STYLE_PASS = True
SKIP_TEXTURE_SOURCES = True
NAMED_SKIP_IDS = {"OracleFragment", "LegacyProjection", "ConvergenceCore"}


def _stable_index(seed: str, size: int) -> int:
    if size <= 0:
        return 0
    digest = hashlib.sha256(seed.encode("utf-8")).hexdigest()
    return int(digest[:8], 16) % size


def _load_json(path: Path):
    return json.loads(path.read_text(encoding="utf-8"))


def _save_json(path: Path, payload) -> None:
    path.write_text(json.dumps(payload, indent=2), encoding="utf-8")


def _asset_registry():
    return unreal.AssetRegistryHelpers.get_asset_registry()


def _asset_tools():
    return unreal.AssetToolsHelpers.get_asset_tools()


def _ensure_folder(package_path: str) -> None:
    if not unreal.EditorAssetLibrary.does_directory_exist(package_path):
        unreal.EditorAssetLibrary.make_directory(package_path)


def _load_or_create_character(asset_name: str):
    asset_path = f"{AUTHORING_ROOT}/{asset_name}.{asset_name}"
    asset = unreal.load_asset(asset_path)
    if asset:
        return asset
    _ensure_folder(AUTHORING_ROOT)
    return _asset_tools().create_asset(
        asset_name=asset_name,
        package_path=AUTHORING_ROOT,
        asset_class=unreal.MetaHumanCharacter,
        factory=unreal.new_object(type=unreal.MetaHumanCharacterFactoryNew),
    )


def _get_wardrobe_assets(package_path: str):
    assets = _asset_registry().get_assets_by_path(package_path=package_path, recursive=True)
    return [asset.get_asset() for asset in assets if asset.asset_class_path.asset_name == "MetaHumanWardrobeItem"]


def _select_asset(seed: str, assets, preferred_keywords=None, rejected_keywords=None):
    preferred_keywords = preferred_keywords or []
    rejected_keywords = rejected_keywords or []

    def ok(asset):
        name = asset.get_name().lower()
        return not any(word in name for word in rejected_keywords)

    filtered = [asset for asset in assets if ok(asset)]
    if preferred_keywords:
        preferred = [asset for asset in filtered if any(word in asset.get_name().lower() for word in preferred_keywords)]
        if preferred:
            filtered = preferred

    if not filtered:
        filtered = assets

    return filtered[_stable_index(seed, len(filtered))] if filtered else None


def _pick_groom_assets(seed: str, presentation: str):
    presentation = presentation.lower()
    registry = _asset_registry()
    slot_map = {}

    groom_packages = {
        "Hair": "/MetaHumanCharacter/Optional/Grooms/Bindings/Hair",
        "Eyebrows": "/MetaHumanCharacter/Optional/Grooms/Bindings/Eyebrows",
        "Beard": "/MetaHumanCharacter/Optional/Grooms/Bindings/Beards",
        "Mustache": "/MetaHumanCharacter/Optional/Grooms/Bindings/Mustaches",
        "Eyelashes": "/MetaHumanCharacter/Optional/Grooms/Bindings/Eyelashes",
        "Peachfuzz": "/MetaHumanCharacter/Optional/Grooms/Bindings/Peachfuzz",
    }

    male_hair = ["short", "crop", "tight", "wavy", "curly"]
    female_hair = ["braid", "ponytail", "straight", "wavy", "bob", "curly"]
    reject = ["punk", "mohawk", "modern", "styled", "perm", "fade"]

    for slot, package_path in groom_packages.items():
        assets = [
            asset.get_asset()
            for asset in registry.get_assets(
                filter=unreal.ARFilter(
                    package_paths=[package_path],
                    class_paths=[unreal.MetaHumanWardrobeItem.static_class().get_class_path_name()],
                )
            )
        ]
        if not assets:
            continue

        preferred = []
        if slot == "Hair":
            preferred = female_hair if "woman" in presentation else male_hair
        elif slot in ("Beard", "Mustache") and "woman" in presentation:
            continue

        chosen = _select_asset(f"{seed}:{slot}", assets, preferred, reject)
        if chosen:
            slot_map[slot] = chosen

    return slot_map


def _add_wardrobe_selection(character, slot_name: str, wardrobe_item) -> None:
    item_key = character.internal_collection.try_add_item_from_wardrobe_item(slot_name=slot_name, wardrobe_item=wardrobe_item)
    if item_key is None:
        unreal.log_warning(f"Failed to add wardrobe item {wardrobe_item.get_name()} to slot {slot_name}")
        return

    selection = unreal.MetaHumanPipelineSlotSelection(slot_name=slot_name, selected_item=item_key)
    if not character.internal_collection.default_instance.try_add_slot_selection(selection=selection):
        unreal.log_warning(f"Failed to select wardrobe item {wardrobe_item.get_name()} for slot {slot_name}")


def _apply_outfit(character, seed: str, palette_name: str) -> None:
    if SKIP_FULL_STYLE_PASS:
        return
    assets = _get_wardrobe_assets("/MetaHumanCharacter/Optional/Clothing")
    outfit = _select_asset(seed, assets, ["default", "garment"], ["jacket", "hoodie", "modern"])
    if outfit:
        _add_wardrobe_selection(character, OUTFIT_SLOT, outfit)


def _apply_grooms(character, seed: str, presentation: str) -> None:
    if SKIP_FULL_STYLE_PASS:
        return
    for slot_name, wardrobe_item in _pick_groom_assets(seed, presentation).items():
        _add_wardrobe_selection(character, slot_name, wardrobe_item)


def _region_palette(region_name: str):
    region = region_name.lower()
    if region == "egypt":
        return unreal.LinearColor(0.78, 0.63, 0.42, 1.0), unreal.LinearColor(0.58, 0.46, 0.27, 1.0)
    if region == "greece":
        return unreal.LinearColor(0.76, 0.78, 0.72, 1.0), unreal.LinearColor(0.47, 0.53, 0.56, 1.0)
    if region == "italicwest":
        return unreal.LinearColor(0.55, 0.48, 0.40, 1.0), unreal.LinearColor(0.28, 0.25, 0.22, 1.0)
    if region == "opening":
        return unreal.LinearColor(0.42, 0.38, 0.34, 1.0), unreal.LinearColor(0.26, 0.22, 0.18, 1.0)
    return unreal.LinearColor(0.35, 0.35, 0.37, 1.0), unreal.LinearColor(0.16, 0.16, 0.18, 1.0)


def _configure_instance_params(character, seed: str, region_name: str) -> None:
    subsystem = unreal.get_editor_subsystem(unreal.MetaHumanCharacterEditorSubsystem)
    subsystem.assemble_for_preview(character=character)

    primary, secondary = _region_palette(region_name)

    for slot_data in character.internal_collection.default_instance.get_slot_selection_data():
        selection = slot_data.selection
        item_path = unreal.MetaHumanPaletteItemPath(item_key=selection.selected_item)
        parameters = character.internal_collection.default_instance.get_instance_parameters(item_path=item_path)
        for parameter in parameters:
            name = str(parameter.name)
            lname = name.lower()
            if parameter.type == unreal.MetaHumanCharacterInstanceParameterType.COLOR:
                if "primary" in lname:
                    parameter.set_color(primary)
                elif "secondary" in lname:
                    parameter.set_color(secondary)
                elif "color" == lname:
                    parameter.set_color(primary)
            elif parameter.type == unreal.MetaHumanCharacterInstanceParameterType.FLOAT:
                if "melanin" in lname:
                    parameter.set_float(0.35 + (_stable_index(seed + name, 100) / 200.0))
                elif "roughness" in lname:
                    parameter.set_float(0.55)


def _apply_body_constraints(character, build_notes: str, presentation: str) -> None:
    subsystem = unreal.get_editor_subsystem(unreal.MetaHumanCharacterEditorSubsystem)
    constraints = subsystem.get_body_constraints(character)
    constraint_map = {str(c.name).lower().replace(" ", "_"): c for c in constraints}

    height = 170.0
    if "woman" in presentation.lower():
        height = 165.0
    if "broad" in build_notes.lower() or "sturdy" in build_notes.lower():
        height += 4.0
    if "lean" in build_notes.lower() or "slender" in build_notes.lower():
        height -= 2.0

    if "height" in constraint_map:
        constraint_map["height"].is_active = True
        constraint_map["height"].target_measurement = height
    if "chest" in constraint_map and ("broad" in build_notes.lower() or "strong" in build_notes.lower()):
        constraint_map["chest"].is_active = True
        constraint_map["chest"].target_measurement *= 1.03

    subsystem.set_body_constraints(character, list(constraint_map.values()))
    subsystem.commit_body_state(character)


def _request_sources_and_build(character, asset_name: str):
    subsystem = unreal.get_editor_subsystem(unreal.MetaHumanCharacterEditorSubsystem)

    autorig = unreal.MetaHumanCharacterAutoRiggingRequestParams()
    autorig.blocking = True
    autorig.report_progress = False
    autorig.rig_type = unreal.MetaHumanRigType.JOINTS_ONLY
    subsystem.request_auto_rigging(character=character, params=autorig)

    if not SKIP_TEXTURE_SOURCES:
        textures = unreal.MetaHumanCharacterTextureRequestParams()
        textures.blocking = True
        textures.report_progress = False
        subsystem.request_texture_sources(character=character, params=textures)

    build_params = unreal.MetaHumanCharacterEditorBuildParameters()
    build_params.pipeline_type = unreal.MetaHumanDefaultPipelineType.OPTIMIZED
    build_params.pipeline_quality = unreal.MetaHumanQualityLevel.LOW
    build_params.absolute_build_path = f"{BUILD_ROOT}/{asset_name}"
    build_params.common_folder_path = COMMON_ROOT
    build_params.enable_wardrobe_item_validation = False
    subsystem.build_meta_human(character=character, params=build_params)


def _find_first_skeletal_mesh(package_path: str):
    assets = _asset_registry().get_assets_by_path(package_path=package_path, recursive=True)
    meshes = [asset for asset in assets if asset.asset_class_path.asset_name == "SkeletalMesh"]
    if not meshes:
        return ""
    meshes.sort(key=lambda item: item.package_name)
    return f"{meshes[0].package_name}.{meshes[0].asset_name}"


def _build_character(asset_name: str, presentation: str, region_id: str, build_notes: str, palette_name: str) -> str:
    subsystem = unreal.get_editor_subsystem(unreal.MetaHumanCharacterEditorSubsystem)
    character = _load_or_create_character(asset_name)
    if not character:
        unreal.log_error(f"Failed to create/load MetaHumanCharacter {asset_name}")
        return ""

    if not subsystem.try_add_object_to_edit(character=character):
        unreal.log_error(f"Failed to open MetaHumanCharacter {asset_name} for edit")
        return ""

    try:
        _apply_outfit(character, asset_name, palette_name)
        _apply_grooms(character, asset_name, presentation)
        _configure_instance_params(character, asset_name, region_id)
        _apply_body_constraints(character, build_notes, presentation)
        _request_sources_and_build(character, asset_name)
        unreal.EditorAssetLibrary.save_loaded_asset(character)
    finally:
        if subsystem.is_object_added_for_editing(character=character):
            subsystem.remove_object_to_edit(character=character)

    return _find_first_skeletal_mesh(f"{BUILD_ROOT}/{asset_name}")


def main():
    cast_records = _load_json(CAST_PATH)
    ambient_profiles = _load_json(AMBIENT_PATH)

    manifest = {
        "named": {},
        "ambient": {},
        "named_character_assets": {},
        "ambient_character_assets": {},
    }

    for record in cast_records:
        character_id = record["CharacterId"]
        if character_id in NAMED_SKIP_IDS:
            continue
        mesh_path = _build_character(
            asset_name=character_id,
            presentation=record.get("Presentation", "Androgynous"),
            region_id=record.get("RegionId", "Opening"),
            build_notes=record.get("PhysicalBuild", ""),
            palette_name=record.get("ClothingVariantId", ""),
        )
        manifest["named_character_assets"][character_id] = f"{AUTHORING_ROOT}/{character_id}.{character_id}"
        if mesh_path:
            manifest["named"][character_id] = mesh_path
            unreal.log(f"[MetaHumanCast] Built named character {character_id} -> {mesh_path}")

    for profile in ambient_profiles:
        profile_id = profile["ProfileId"]
        for character_id in profile.get("CharacterIds", []):
            mesh_path = manifest["named"].get(character_id)
            character_asset_path = manifest["named_character_assets"].get(character_id)
            if mesh_path:
                manifest["ambient"][profile_id] = mesh_path
                unreal.log(f"[MetaHumanCast] Assigned ambient profile {profile_id} -> {character_id}")
                break
            if character_asset_path:
                manifest["ambient_character_assets"][profile_id] = character_asset_path
                unreal.log(f"[MetaHumanCast] Assigned authored ambient profile {profile_id} -> {character_id}")
                break

    _save_json(MANIFEST_PATH, manifest)
    unreal.log(f"[MetaHumanCast] Wrote manifest to {MANIFEST_PATH}")


if __name__ == "__main__":
    main()
