import json
import os

import unreal


PROJECT_DIR = unreal.Paths.project_dir()
IMPORT_DIR = os.path.join(PROJECT_DIR, "Saved", "BootstrapImports")


def log(message):
    unreal.log(f"[ManyNamesBootstrap] {message}")


def fail(message):
    raise RuntimeError(message)


def make_text(value):
    try:
        return unreal.Text(value)
    except Exception:
        return value


def ensure_directory(path):
    if not unreal.EditorAssetLibrary.does_directory_exist(path):
        unreal.EditorAssetLibrary.make_directory(path)
        log(f"Created directory {path}")


def ensure_import_dir():
    os.makedirs(IMPORT_DIR, exist_ok=True)


def make_tag_struct(tag_name):
    return f'(TagName="{tag_name}")'


def make_tag_array(tag_names):
    return [make_tag_struct(tag_name) for tag_name in tag_names]


def make_tag_container(tag_names):
    if not tag_names:
        return "(GameplayTags=)"
    return f'(GameplayTags=({",".join(make_tag_struct(tag_name) for tag_name in tag_names)}))'


def write_import_json(source_filename, transform_fn):
    ensure_import_dir()
    source_path = os.path.join(PROJECT_DIR, "Data", source_filename)
    with open(source_path, "r", encoding="utf-8") as handle:
        rows = json.load(handle)

    transformed_rows = [transform_fn(dict(row)) for row in rows]
    output_path = os.path.join(IMPORT_DIR, source_filename)
    with open(output_path, "w", encoding="utf-8") as handle:
        json.dump(transformed_rows, handle, indent=2)

    return output_path


def prepare_regions_import_json():
    def transform(row):
        row["EntryConditions"] = make_tag_container(row.get("EntryConditions", []))
        return row

    return write_import_json("regions.json", transform)


def prepare_quests_import_json():
    def transform(row):
        row["RequiredDomains"] = make_tag_array(row.get("RequiredDomains", []))
        row["RewardDomains"] = make_tag_array(row.get("RewardDomains", []))
        return row

    return write_import_json("quests.json", transform)


def prepare_dialogue_import_json():
    def transform(row):
        row["RequiredDomains"] = make_tag_array(row.get("RequiredDomains", []))
        row["GrantedDomains"] = make_tag_array(row.get("GrantedDomains", []))
        row["ResultTags"] = make_tag_container(row.get("ResultTags", []))
        return row

    return write_import_json("dialogue_choices.json", transform)


def load_native_class(script_name):
    cls = getattr(unreal, script_name, None)
    if cls and hasattr(cls, "static_class"):
        return cls.static_class()

    loaded = unreal.load_class(None, f"/Script/ManyNames.{script_name}")
    if not loaded:
        fail(f"Failed to load native class /Script/ManyNames.{script_name}")
    return loaded


def load_native_struct(script_name):
    struct_type = getattr(unreal, script_name, None)
    if struct_type and hasattr(struct_type, "static_struct"):
        return struct_type.static_struct()

    loaded = unreal.load_object(None, f"/Script/ManyNames.{script_name}")
    if not loaded:
        fail(f"Failed to load native struct /Script/ManyNames.{script_name}")
    return loaded


def get_or_create_data_table(asset_dir, asset_name, row_struct_name, import_json_path):
    asset_path = f"{asset_dir}/{asset_name}"
    row_struct = load_native_struct(row_struct_name)
    datatable = unreal.EditorAssetLibrary.load_asset(asset_path)
    if not datatable:
        factory = unreal.DataTableFactory()
        factory.set_editor_property("struct", row_struct)
        datatable = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            asset_name,
            asset_dir,
            unreal.DataTable,
            factory,
        )

    if not datatable:
        fail(f"Failed to create or load DataTable {asset_path}")

    if not os.path.isfile(import_json_path):
        fail(f"Missing transformed JSON data file {import_json_path}")

    ok = unreal.DataTableFunctionLibrary.fill_data_table_from_json_file(datatable, import_json_path, row_struct)
    if not ok:
        fail(f"Failed to populate {asset_path} from {import_json_path}")

    unreal.EditorAssetLibrary.save_asset(asset_path)
    row_names = unreal.DataTableFunctionLibrary.get_data_table_row_names(datatable)
    log(f"Imported {asset_path} with {len(row_names)} rows")
    return datatable


def get_or_create_blueprint(asset_dir, asset_name, parent_class_name, widget=False):
    asset_path = f"{asset_dir}/{asset_name}"
    blueprint = unreal.EditorAssetLibrary.load_asset(asset_path)
    if blueprint:
        log(f"Blueprint already exists: {asset_path}")
        return blueprint

    parent_class = load_native_class(parent_class_name)
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()

    if widget:
        factory = unreal.WidgetBlueprintFactory()
        factory.set_editor_property("parent_class", parent_class)
        widget_blueprint_class = getattr(unreal, "WidgetBlueprint", unreal.Blueprint)
        blueprint = asset_tools.create_asset(asset_name, asset_dir, widget_blueprint_class, factory)
    else:
        factory = unreal.BlueprintFactory()
        factory.set_editor_property("parent_class", parent_class)
        blueprint = asset_tools.create_asset(asset_name, asset_dir, unreal.Blueprint, factory)

    if not blueprint:
        fail(f"Failed to create blueprint {asset_path}")

    if hasattr(unreal, "KismetEditorUtilities") and hasattr(unreal.KismetEditorUtilities, "compile_blueprint"):
        unreal.KismetEditorUtilities.compile_blueprint(blueprint)

    unreal.EditorAssetLibrary.save_asset(asset_path)
    log(f"Created blueprint {asset_path}")
    return blueprint


def get_mesh_component(actor):
    if actor.get_class().get_name() == "StaticMeshActor":
        return actor.get_editor_property("static_mesh_component")
    return actor.get_editor_property("mesh_component")


def get_or_create_material(asset_dir, asset_name):
    asset_path = f"{asset_dir}/{asset_name}"
    material = unreal.EditorAssetLibrary.load_asset(asset_path)
    if material:
        return material

    factory = unreal.MaterialFactoryNew()
    material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        asset_name,
        asset_dir,
        unreal.Material,
        factory,
    )
    if not material:
        fail(f"Failed to create material {asset_path}")

    unreal.EditorAssetLibrary.save_asset(asset_path)
    return material


def create_surface_material(asset_name, tint, roughness, metallic=0.0, emissive=None, emissive_strength=0.0):
    material = get_or_create_material("/Game/Materials", asset_name)
    texture = unreal.EditorAssetLibrary.load_asset("/Engine/EngineMaterials/T_Default_MacroVariation")
    material_editing = unreal.MaterialEditingLibrary

    if hasattr(material_editing, "delete_all_material_expressions"):
        material_editing.delete_all_material_expressions(material)

    texture_sample = material_editing.create_material_expression(material, unreal.MaterialExpressionTextureSample, -560, -40)
    if texture:
        texture_sample.texture = texture

    tint_expr = material_editing.create_material_expression(material, unreal.MaterialExpressionConstant3Vector, -560, 140)
    tint_expr.set_editor_property("constant", tint)

    multiply_expr = material_editing.create_material_expression(material, unreal.MaterialExpressionMultiply, -320, 40)
    material_editing.connect_material_expressions(texture_sample, "", multiply_expr, "A")
    material_editing.connect_material_expressions(tint_expr, "", multiply_expr, "B")
    material_editing.connect_material_property(multiply_expr, "", unreal.MaterialProperty.MP_BASE_COLOR)

    roughness_expr = material_editing.create_material_expression(material, unreal.MaterialExpressionConstant, -320, 220)
    roughness_expr.set_editor_property("r", roughness)
    material_editing.connect_material_property(roughness_expr, "", unreal.MaterialProperty.MP_ROUGHNESS)

    metallic_expr = material_editing.create_material_expression(material, unreal.MaterialExpressionConstant, -320, 300)
    metallic_expr.set_editor_property("r", metallic)
    material_editing.connect_material_property(metallic_expr, "", unreal.MaterialProperty.MP_METALLIC)

    if emissive and emissive_strength > 0.0:
        emissive_color_expr = material_editing.create_material_expression(material, unreal.MaterialExpressionConstant3Vector, -320, -180)
        emissive_color_expr.set_editor_property("constant", emissive)
        emissive_strength_expr = material_editing.create_material_expression(material, unreal.MaterialExpressionConstant, -320, -260)
        emissive_strength_expr.set_editor_property("r", emissive_strength)
        emissive_multiply = material_editing.create_material_expression(material, unreal.MaterialExpressionMultiply, -120, -220)
        material_editing.connect_material_expressions(emissive_color_expr, "", emissive_multiply, "A")
        material_editing.connect_material_expressions(emissive_strength_expr, "", emissive_multiply, "B")
        material_editing.connect_material_property(emissive_multiply, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

    material_editing.recompile_material(material)
    unreal.EditorAssetLibrary.save_asset(material.get_path_name())
    log(f"Prepared material {material.get_path_name()}")
    return material


def build_material_library():
    ensure_directory("/Game/Materials")
    return {
        "ash_stone": create_surface_material(
            "M_MN_AshStone",
            unreal.LinearColor(0.22, 0.22, 0.24, 1.0),
            roughness=0.92,
        ),
        "sand_stone": create_surface_material(
            "M_MN_SandStone",
            unreal.LinearColor(0.67, 0.56, 0.38, 1.0),
            roughness=0.88,
        ),
        "wreck_metal": create_surface_material(
            "M_MN_WreckMetal",
            unreal.LinearColor(0.25, 0.19, 0.16, 1.0),
            roughness=0.55,
            metallic=0.45,
            emissive=unreal.LinearColor(0.7, 0.18, 0.08, 1.0),
            emissive_strength=0.3,
        ),
        "bronze": create_surface_material(
            "M_MN_Bronze",
            unreal.LinearColor(0.60, 0.42, 0.18, 1.0),
            roughness=0.34,
            metallic=0.9,
        ),
        "miracle": create_surface_material(
            "M_MN_Miracle",
            unreal.LinearColor(0.10, 0.04, 0.03, 1.0),
            roughness=0.18,
            metallic=0.05,
            emissive=unreal.LinearColor(1.0, 0.34, 0.10, 1.0),
            emissive_strength=12.0,
        ),
        "oracle": create_surface_material(
            "M_MN_Oracle",
            unreal.LinearColor(0.08, 0.11, 0.16, 1.0),
            roughness=0.22,
            metallic=0.15,
            emissive=unreal.LinearColor(0.96, 0.76, 0.22, 1.0),
            emissive_strength=4.0,
        ),
    }


def apply_material(actor, material):
    mesh_component = get_mesh_component(actor)
    mesh_component.set_material(0, material)


def set_mesh(actor, mesh_path):
    mesh = unreal.load_object(None, mesh_path)
    if not mesh:
        fail(f"Missing engine mesh {mesh_path}")

    mesh_component = get_mesh_component(actor)
    mesh_component.set_editor_property("static_mesh", mesh)


def spawn_actor(actor_class, location, rotation=None, scale=None, label=None):
    rotation = rotation or unreal.Rotator(0.0, 0.0, 0.0)
    actor = unreal.EditorLevelLibrary.spawn_actor_from_class(actor_class, location, rotation)
    if not actor:
        fail(f"Failed to spawn actor of class {actor_class}")

    if scale:
        actor.set_actor_scale3d(scale)

    if label:
        actor.set_actor_label(label)

    return actor


def configure_interactable(actor, label, interaction_type, quest_id="", target_region=None, required_outputs=None, single_use=True):
    actor.set_editor_property("interaction_label", make_text(label))
    actor.set_editor_property("interaction_type", interaction_type)
    actor.set_editor_property("quest_id", quest_id)
    actor.set_editor_property("single_use", single_use)
    actor.set_editor_property("required_outputs", required_outputs or [])
    if target_region is not None:
        actor.set_editor_property("target_region_id", target_region)


def spawn_floor(label, location, scale):
    floor_actor = spawn_actor(unreal.StaticMeshActor.static_class(), location, scale=scale, label=label)
    set_mesh(floor_actor, "/Engine/BasicShapes/Plane.Plane")
    return floor_actor


def spawn_block(label, location, scale):
    block_actor = spawn_actor(unreal.StaticMeshActor.static_class(), location, scale=scale, label=label)
    set_mesh(block_actor, "/Engine/BasicShapes/Cube.Cube")
    return block_actor


def create_lighting():
    sun = spawn_actor(
        unreal.DirectionalLight.static_class(),
        unreal.Vector(0.0, 0.0, 600.0),
        unreal.Rotator(-45.0, -35.0, 0.0),
        label="SunLight",
    )
    sky = spawn_actor(unreal.SkyLight.static_class(), unreal.Vector(0.0, 0.0, 200.0), label="SkyLight")

    sun_component = sun.get_editor_property("directional_light_component")
    sun_component.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
    sun_component.set_editor_property("intensity", 8.0)

    sky_component = sky.get_editor_property("light_component")
    sky_component.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
    sky_component.set_editor_property("intensity", 1.0)


def configure_world_for_dynamic_lighting():
    editor_world = unreal.EditorLevelLibrary.get_editor_world()
    if not editor_world:
        fail("Failed to resolve editor world")

    world_settings = editor_world.get_world_settings()
    world_settings.set_editor_property("force_no_precomputed_lighting", True)


def clear_level():
    protected_names = {"WorldSettings", "Brush"}
    for actor in unreal.EditorLevelLibrary.get_all_level_actors():
        class_name = actor.get_class().get_name()
        if class_name in protected_names:
            continue
        if actor.get_name() == "Brush":
            continue
        unreal.EditorLevelLibrary.destroy_actor(actor)


def save_map(asset_path):
    editor_world = unreal.EditorLevelLibrary.get_editor_world()
    if not unreal.EditorLoadingAndSavingUtils.save_map(editor_world, asset_path):
        fail(f"Failed to save map {asset_path}")
    log(f"Saved map {asset_path}")


def build_opening_map(interactable_class, materials):
    unreal.EditorLoadingAndSavingUtils.new_blank_map(False)
    clear_level()
    configure_world_for_dynamic_lighting()
    create_lighting()

    opening_floor = spawn_floor("OpeningFloor", unreal.Vector(0.0, 0.0, -10.0), unreal.Vector(20.0, 20.0, 1.0))
    wreck_a = spawn_block("WreckShardA", unreal.Vector(450.0, -180.0, 80.0), unreal.Vector(2.0, 2.0, 3.0))
    wreck_b = spawn_block("WreckShardB", unreal.Vector(-350.0, 260.0, 60.0), unreal.Vector(1.5, 1.5, 2.5))
    apply_material(opening_floor, materials["ash_stone"])
    apply_material(wreck_a, materials["wreck_metal"])
    apply_material(wreck_b, materials["wreck_metal"])

    spawn_actor(
        unreal.PlayerStart.static_class(),
        unreal.Vector(-650.0, 0.0, 120.0),
        unreal.Rotator(0.0, 0.0, 0.0),
        label="PlayerStart",
    )

    miracle_actor = spawn_actor(
        interactable_class,
        unreal.Vector(0.0, 0.0, 90.0),
        label="FirstMiracleAnchor",
    )
    set_mesh(miracle_actor, "/Engine/BasicShapes/Cube.Cube")
    miracle_actor.set_actor_scale3d(unreal.Vector(1.0, 1.0, 2.0))
    apply_material(miracle_actor, materials["miracle"])
    configure_interactable(
        miracle_actor,
        "Trigger the first miracle",
        unreal.ManyNamesInteractionActionType.FIRST_MIRACLE,
        single_use=True,
    )

    witness_actor = spawn_actor(
        interactable_class,
        unreal.Vector(320.0, 260.0, 70.0),
        label="WitnessAnchor",
    )
    set_mesh(witness_actor, "/Engine/BasicShapes/Cube.Cube")
    witness_actor.set_actor_scale3d(unreal.Vector(0.8, 0.8, 1.8))
    apply_material(witness_actor, materials["oracle"])
    configure_interactable(
        witness_actor,
        "Speak to the witness",
        unreal.ManyNamesInteractionActionType.QUEST_DIALOGUE,
        quest_id="opening_side_01",
        required_outputs=["Story.Prologue.Complete"],
        single_use=True,
    )

    egypt_gate = spawn_actor(
        interactable_class,
        unreal.Vector(820.0, -140.0, 110.0),
        label="EgyptGate",
    )
    set_mesh(egypt_gate, "/Engine/BasicShapes/Cube.Cube")
    egypt_gate.set_actor_scale3d(unreal.Vector(1.0, 1.0, 3.0))
    apply_material(egypt_gate, materials["bronze"])
    configure_interactable(
        egypt_gate,
        "Travel to Egypt",
        unreal.ManyNamesInteractionActionType.REGION_TRAVEL,
        target_region=unreal.ManyNamesRegionId.EGYPT,
        required_outputs=["State.Region.Opening.Complete"],
        single_use=False,
    )

    save_map("/Game/Maps/L_OpeningCatastrophe")


def build_egypt_map(interactable_class, materials):
    unreal.EditorLoadingAndSavingUtils.new_blank_map(False)
    clear_level()
    configure_world_for_dynamic_lighting()
    create_lighting()

    egypt_floor = spawn_floor("EgyptFloor", unreal.Vector(0.0, 0.0, -10.0), unreal.Vector(24.0, 18.0, 1.0))
    archive_wall_left = spawn_block("ArchiveWallLeft", unreal.Vector(0.0, -620.0, 180.0), unreal.Vector(8.0, 0.5, 4.0))
    archive_wall_right = spawn_block("ArchiveWallRight", unreal.Vector(0.0, 620.0, 180.0), unreal.Vector(8.0, 0.5, 4.0))
    archive_threshold = spawn_block("ArchiveThreshold", unreal.Vector(720.0, 0.0, 220.0), unreal.Vector(0.8, 4.0, 5.0))
    apply_material(egypt_floor, materials["sand_stone"])
    apply_material(archive_wall_left, materials["sand_stone"])
    apply_material(archive_wall_right, materials["sand_stone"])
    apply_material(archive_threshold, materials["bronze"])

    spawn_actor(
        unreal.PlayerStart.static_class(),
        unreal.Vector(-800.0, 0.0, 120.0),
        unreal.Rotator(0.0, 0.0, 0.0),
        label="PlayerStart",
    )

    archive_actor = spawn_actor(
        interactable_class,
        unreal.Vector(220.0, 0.0, 90.0),
        label="ArchiveEntry",
    )
    set_mesh(archive_actor, "/Engine/BasicShapes/Cube.Cube")
    archive_actor.set_actor_scale3d(unreal.Vector(1.2, 1.2, 2.4))
    apply_material(archive_actor, materials["oracle"])
    configure_interactable(
        archive_actor,
        "Enter the sealed archive",
        unreal.ManyNamesInteractionActionType.QUEST_DIALOGUE,
        quest_id="egypt_main_01",
        required_outputs=["State.Region.Opening.Complete"],
        single_use=True,
    )

    save_map("/Game/Maps/L_EgyptHub")


def main():
    log("Bootstrapping Many Names editor assets")

    for directory in (
        "/Game/Data",
        "/Game/Blueprints",
        "/Game/Blueprints/Core",
        "/Game/Blueprints/UI",
        "/Game/Blueprints/Interaction",
        "/Game/Maps",
    ):
        ensure_directory(directory)

    regions_import_path = prepare_regions_import_json()
    quests_import_path = prepare_quests_import_json()
    dialogue_import_path = prepare_dialogue_import_json()

    get_or_create_data_table("/Game/Data", "DT_Regions", "ManyNamesRegionRow", regions_import_path)
    get_or_create_data_table("/Game/Data", "DT_Quests", "ManyNamesQuestRow", quests_import_path)
    get_or_create_data_table("/Game/Data", "DT_DialogueChoices", "ManyNamesDialogueChoiceRow", dialogue_import_path)

    get_or_create_blueprint("/Game/Blueprints/Core", "BP_ManyNamesGameMode", "ManyNamesPrototypeGameMode")
    get_or_create_blueprint("/Game/Blueprints/Core", "BP_FirstPersonCharacter", "ManyNamesFirstPersonCharacter")
    get_or_create_blueprint("/Game/Blueprints/Core", "BP_DialogueController", "ManyNamesDialogueController")
    get_or_create_blueprint("/Game/Blueprints/UI", "BP_PlayerJournalWidget", "ManyNamesPlayerJournalWidget", widget=True)
    get_or_create_blueprint("/Game/Blueprints/Interaction", "BP_Interactable", "ManyNamesInteractableActor")
    materials = build_material_library()

    interactable_class = unreal.EditorAssetLibrary.load_blueprint_class("/Game/Blueprints/Interaction/BP_Interactable")
    if not interactable_class:
        fail("Failed to load /Game/Blueprints/Interaction/BP_Interactable")

    build_opening_map(interactable_class, materials)
    build_egypt_map(interactable_class, materials)

    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)

    for asset_path in (
        "/Game/Data/DT_Regions",
        "/Game/Data/DT_Quests",
        "/Game/Data/DT_DialogueChoices",
        "/Game/Blueprints/Core/BP_ManyNamesGameMode",
        "/Game/Blueprints/Core/BP_FirstPersonCharacter",
        "/Game/Blueprints/Core/BP_DialogueController",
        "/Game/Blueprints/UI/BP_PlayerJournalWidget",
        "/Game/Blueprints/Interaction/BP_Interactable",
        "/Game/Materials/M_MN_AshStone",
        "/Game/Materials/M_MN_SandStone",
        "/Game/Materials/M_MN_WreckMetal",
        "/Game/Materials/M_MN_Bronze",
        "/Game/Materials/M_MN_Miracle",
        "/Game/Materials/M_MN_Oracle",
        "/Game/Maps/L_OpeningCatastrophe",
        "/Game/Maps/L_EgyptHub",
    ):
        if unreal.EditorAssetLibrary.does_asset_exist(asset_path):
            log(f"Verified asset {asset_path}")
        else:
            fail(f"Expected asset missing: {asset_path}")

    log("Bootstrap complete")


if __name__ == "__main__":
    main()
