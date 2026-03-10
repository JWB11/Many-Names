import os
import shutil

import unreal


PROJECT_DIR = unreal.Paths.project_dir()
MANNEQUIN_SOURCE_DIR = "/Users/Shared/Epic Games/UE_5.7/Templates/TemplateResources/High/Characters/Content/Mannequins"
MANNEQUIN_DEST_DIR = os.path.join(PROJECT_DIR, "Content", "Mannequins")

OPENING_MAP = "/Game/Maps/L_OpeningCatastrophe"
EGYPT_MAP = "/Game/Maps/L_EgyptHub"


def log(message):
    unreal.log(f"[ManyNamesExpandedWorld] {message}")


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


def ensure_project_directories():
    for path in (
        "/Game/Materials",
        "/Game/Maps",
        "/Game/Characters/Mannequins",
    ):
        ensure_directory(path)


def ensure_mannequin_assets():
    if not os.path.isdir(MANNEQUIN_SOURCE_DIR):
        fail(f"Missing mannequin source directory: {MANNEQUIN_SOURCE_DIR}")

    os.makedirs(os.path.dirname(MANNEQUIN_DEST_DIR), exist_ok=True)
    shutil.copytree(MANNEQUIN_SOURCE_DIR, MANNEQUIN_DEST_DIR, dirs_exist_ok=True)

    asset_registry = unreal.AssetRegistryHelpers.get_asset_registry()
    asset_registry.scan_paths_synchronous(["/Game/Characters/Mannequins"], True, True)

    for asset_path in (
        "/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple",
        "/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple",
    ):
        if not unreal.EditorAssetLibrary.does_asset_exist(asset_path):
            fail(f"Expected mannequin asset missing after copy: {asset_path}")

    log("Imported mannequin assets into /Game/Characters/Mannequins")


def get_or_create_material(asset_name):
    asset_path = f"/Game/Materials/{asset_name}"
    material = unreal.EditorAssetLibrary.load_asset(asset_path)
    if material:
        return material

    factory = unreal.MaterialFactoryNew()
    material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        asset_name,
        "/Game/Materials",
        unreal.Material,
        factory,
    )
    if not material:
        fail(f"Failed to create material {asset_path}")
    return material


def create_surface_material(asset_name, tint, roughness, metallic=0.0, emissive=None, emissive_strength=0.0):
    material = get_or_create_material(asset_name)
    material_editing = unreal.MaterialEditingLibrary
    texture = unreal.EditorAssetLibrary.load_asset("/Engine/EngineMaterials/T_Default_MacroVariation")

    if hasattr(material_editing, "delete_all_material_expressions"):
        material_editing.delete_all_material_expressions(material)

    texture_sample = material_editing.create_material_expression(material, unreal.MaterialExpressionTextureSample, -680, -20)
    if texture:
        texture_sample.texture = texture

    tint_expr = material_editing.create_material_expression(material, unreal.MaterialExpressionConstant3Vector, -680, 150)
    tint_expr.set_editor_property("constant", tint)

    base_color_mul = material_editing.create_material_expression(material, unreal.MaterialExpressionMultiply, -420, 40)
    material_editing.connect_material_expressions(texture_sample, "", base_color_mul, "A")
    material_editing.connect_material_expressions(tint_expr, "", base_color_mul, "B")
    material_editing.connect_material_property(base_color_mul, "", unreal.MaterialProperty.MP_BASE_COLOR)

    roughness_expr = material_editing.create_material_expression(material, unreal.MaterialExpressionConstant, -420, 230)
    roughness_expr.set_editor_property("r", roughness)
    material_editing.connect_material_property(roughness_expr, "", unreal.MaterialProperty.MP_ROUGHNESS)

    metallic_expr = material_editing.create_material_expression(material, unreal.MaterialExpressionConstant, -420, 310)
    metallic_expr.set_editor_property("r", metallic)
    material_editing.connect_material_property(metallic_expr, "", unreal.MaterialProperty.MP_METALLIC)

    if emissive and emissive_strength > 0.0:
        emissive_color_expr = material_editing.create_material_expression(material, unreal.MaterialExpressionConstant3Vector, -420, -180)
        emissive_color_expr.set_editor_property("constant", emissive)
        emissive_strength_expr = material_editing.create_material_expression(material, unreal.MaterialExpressionConstant, -420, -260)
        emissive_strength_expr.set_editor_property("r", emissive_strength)
        emissive_mul = material_editing.create_material_expression(material, unreal.MaterialExpressionMultiply, -180, -220)
        material_editing.connect_material_expressions(emissive_color_expr, "", emissive_mul, "A")
        material_editing.connect_material_expressions(emissive_strength_expr, "", emissive_mul, "B")
        material_editing.connect_material_property(emissive_mul, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

    material_editing.recompile_material(material)
    unreal.EditorAssetLibrary.save_asset(material.get_path_name())
    return material


def build_material_library():
    return {
        "ash_stone": create_surface_material("M_MN_AshStone", unreal.LinearColor(0.23, 0.23, 0.25, 1.0), 0.92),
        "wreck_metal": create_surface_material("M_MN_WreckMetal", unreal.LinearColor(0.27, 0.18, 0.14, 1.0), 0.54, 0.45, unreal.LinearColor(0.72, 0.16, 0.08, 1.0), 0.25),
        "sand_stone": create_surface_material("M_MN_SandStone", unreal.LinearColor(0.71, 0.60, 0.40, 1.0), 0.86),
        "plaster": create_surface_material("M_MN_Plaster", unreal.LinearColor(0.83, 0.77, 0.62, 1.0), 0.80),
        "basalt": create_surface_material("M_MN_Basalt", unreal.LinearColor(0.11, 0.10, 0.12, 1.0), 0.78),
        "bronze": create_surface_material("M_MN_Bronze", unreal.LinearColor(0.61, 0.44, 0.20, 1.0), 0.32, 0.92),
        "linen": create_surface_material("M_MN_Linen", unreal.LinearColor(0.79, 0.71, 0.53, 1.0), 0.95),
        "water": create_surface_material("M_MN_Water", unreal.LinearColor(0.08, 0.18, 0.20, 1.0), 0.06, 0.0, unreal.LinearColor(0.02, 0.16, 0.22, 1.0), 1.0),
        "miracle": create_surface_material("M_MN_Miracle", unreal.LinearColor(0.10, 0.05, 0.03, 1.0), 0.16, 0.05, unreal.LinearColor(1.0, 0.35, 0.10, 1.0), 12.0),
        "oracle": create_surface_material("M_MN_Oracle", unreal.LinearColor(0.08, 0.11, 0.16, 1.0), 0.24, 0.18, unreal.LinearColor(0.98, 0.76, 0.22, 1.0), 4.0),
        "guard": create_surface_material("M_MN_GuardCloth", unreal.LinearColor(0.21, 0.18, 0.14, 1.0), 0.92),
        "priest": create_surface_material("M_MN_PriestLinen", unreal.LinearColor(0.72, 0.66, 0.53, 1.0), 0.93),
    }


def get_static_mesh_component(actor):
    if hasattr(actor, "get_editor_property"):
        class_name = actor.get_class().get_name()
    else:
        class_name = ""
    if class_name == "StaticMeshActor":
        return actor.get_editor_property("static_mesh_component")
    if class_name == "ManyNamesScenicActor":
        return actor.get_editor_property("static_mesh_component")
    return actor.get_editor_property("mesh_component")


def set_static_mesh(actor, mesh_path):
    mesh = unreal.EditorAssetLibrary.load_asset(mesh_path)
    if not mesh:
        fail(f"Missing static mesh {mesh_path}")
    get_static_mesh_component(actor).set_editor_property("static_mesh", mesh)


def set_skeletal_mesh(component, mesh):
    for prop_name in ("skeletal_mesh_asset", "skeletal_mesh"):
        try:
            component.set_editor_property(prop_name, mesh)
            return
        except Exception:
            pass

    if hasattr(component, "set_skeletal_mesh_asset"):
        component.set_skeletal_mesh_asset(mesh)
        return
    if hasattr(component, "set_skeletal_mesh"):
        component.set_skeletal_mesh(mesh)
        return

    fail("Unable to assign skeletal mesh to component")


def spawn_actor(actor_class, location, rotation=None, scale=None, label=None):
    rotation = rotation or unreal.Rotator(0.0, 0.0, 0.0)
    actor = unreal.EditorLevelLibrary.spawn_actor_from_class(actor_class, location, rotation)
    if not actor:
        fail(f"Failed to spawn actor of class {actor_class}")

    if label:
        actor.set_actor_label(label)
    if scale:
        actor.set_actor_scale3d(scale)
    return actor


def spawn_mesh_actor(label, mesh_path, location, scale, rotation=None, material=None):
    actor = spawn_actor(unreal.ManyNamesScenicActor.static_class(), location, rotation=rotation, scale=scale, label=label)
    set_static_mesh(actor, mesh_path)
    actor.get_editor_property("skeletal_mesh_component").set_visibility(False)
    if material:
        get_static_mesh_component(actor).set_material(0, material)
    return actor


def spawn_floor(label, location, scale, material):
    return spawn_mesh_actor(label, "/Engine/BasicShapes/Plane.Plane", location, scale, material=material)


def spawn_block(label, location, scale, material, rotation=None):
    return spawn_mesh_actor(label, "/Engine/BasicShapes/Cube.Cube", location, scale, rotation=rotation, material=material)


def spawn_cylinder(label, location, scale, material, rotation=None):
    return spawn_mesh_actor(label, "/Engine/BasicShapes/Cylinder.Cylinder", location, scale, rotation=rotation, material=material)


def spawn_cone(label, location, scale, material, rotation=None):
    return spawn_mesh_actor(label, "/Engine/BasicShapes/Cone.Cone", location, scale, rotation=rotation, material=material)


def spawn_sphere(label, location, scale, material):
    return spawn_mesh_actor(label, "/Engine/BasicShapes/Sphere.Sphere", location, scale, material=material)


def create_point_light(label, location, intensity=2500.0, color=unreal.LinearColor(1.0, 0.75, 0.4, 1.0), attenuation=1800.0):
    light = spawn_actor(unreal.PointLight.static_class(), location, label=label)
    component = light.get_editor_property("point_light_component")
    component.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
    component.set_editor_property("intensity", intensity)
    component.set_editor_property("light_color", color)
    component.set_editor_property("attenuation_radius", attenuation)
    return light


def clear_level():
    protected_names = {"WorldSettings", "Brush"}
    for actor in unreal.EditorLevelLibrary.get_all_level_actors():
        if actor.get_class().get_name() in protected_names:
            continue
        if actor.get_name() == "Brush":
            continue
        unreal.EditorLevelLibrary.destroy_actor(actor)


def configure_dynamic_world():
    editor_world = unreal.EditorLevelLibrary.get_editor_world()
    if not editor_world:
        fail("Failed to get editor world")

    world_settings = editor_world.get_world_settings()
    world_settings.set_editor_property("force_no_precomputed_lighting", True)


def create_sky_and_lighting():
    sun = spawn_actor(unreal.DirectionalLight.static_class(), unreal.Vector(0.0, 0.0, 1200.0), unreal.Rotator(-38.0, -28.0, 0.0), label="SunLight")
    sun_comp = sun.get_editor_property("directional_light_component")
    sun_comp.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
    sun_comp.set_editor_property("intensity", 9.5)

    sky_light = spawn_actor(unreal.SkyLight.static_class(), unreal.Vector(0.0, 0.0, 240.0), label="SkyLight")
    sky_comp = sky_light.get_editor_property("light_component")
    sky_comp.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
    sky_comp.set_editor_property("intensity", 1.2)

    spawn_actor(unreal.SkyAtmosphere.static_class(), unreal.Vector(0.0, 0.0, 0.0), label="SkyAtmosphere")

    fog = spawn_actor(unreal.ExponentialHeightFog.static_class(), unreal.Vector(0.0, 0.0, 0.0), label="HeightFog")
    fog_comp = fog.get_editor_property("component")
    fog_comp.set_editor_property("fog_density", 0.01)
    fog_comp.set_editor_property("fog_height_falloff", 0.2)


def create_player_start(location, yaw=0.0):
    spawn_actor(unreal.PlayerStart.static_class(), location, unreal.Rotator(0.0, yaw, 0.0), label="PlayerStart")


def configure_interactable(actor, label, interaction_type, quest_id="", target_region=None, required_outputs=None, single_use=True, material=None):
    actor.set_editor_property("interaction_label", make_text(label))
    actor.set_editor_property("interaction_type", interaction_type)
    actor.set_editor_property("quest_id", quest_id)
    actor.set_editor_property("single_use", single_use)
    actor.set_editor_property("required_outputs", required_outputs or [])
    if target_region is not None:
        actor.set_editor_property("target_region_id", target_region)

    mesh_component = actor.get_editor_property("mesh_component")
    mesh_component.set_editor_property("hidden_in_game", True)
    mesh_component.set_visibility(False)
    mesh_component.set_relative_scale3d(unreal.Vector(0.75, 0.75, 2.2))
    if material:
        mesh_component.set_material(0, material)


def assign_all_materials(component, material):
    if not material:
        return
    try:
        material_count = component.get_num_materials()
    except Exception:
        material_count = 4
    for index in range(max(material_count, 1)):
        try:
            component.set_material(index, material)
        except Exception:
            pass


def create_humanoid_interactable(actor_class, label, location, rotation, mesh_asset, visual_material, interaction_label, interaction_type, quest_id="", required_outputs=None, single_use=True):
    actor = spawn_actor(actor_class, location, rotation=rotation, label=label)
    configure_interactable(
        actor,
        interaction_label,
        interaction_type,
        quest_id=quest_id,
        required_outputs=required_outputs,
        single_use=single_use,
    )

    skeletal_component = actor.get_editor_property("skeletal_mesh_component")
    set_skeletal_mesh(skeletal_component, mesh_asset)
    assign_all_materials(skeletal_component, visual_material)
    return actor


def create_npc(label, mesh_asset, location, yaw, material, scale=1.0):
    actor = spawn_actor(unreal.ManyNamesScenicActor.static_class(), location, unreal.Rotator(0.0, yaw, 0.0), label=label)
    actor.get_editor_property("static_mesh_component").set_visibility(False)
    actor.get_editor_property("static_mesh_component").set_hidden_in_game(True)
    component = actor.get_editor_property("skeletal_mesh_component")
    set_skeletal_mesh(component, mesh_asset)
    actor.set_actor_scale3d(unreal.Vector(scale, scale, scale))
    assign_all_materials(component, material)
    component.set_editor_property("mobility", unreal.ComponentMobility.STATIC)
    component.set_visibility(True)
    return actor


def build_stairs(base_label, start_location, steps, step_width, step_depth, step_height, material, axis="x", direction=1.0):
    for index in range(steps):
        location = unreal.Vector(start_location.x, start_location.y, start_location.z)
        offset = step_depth * index * direction
        if axis == "x":
            location.x += offset
        else:
            location.y += offset
        location.z += step_height * index
        spawn_block(
            f"{base_label}_{index + 1}",
            location,
            unreal.Vector(step_depth / 100.0, step_width / 100.0, step_height / 100.0),
            material,
        )


def build_market_stall(label_prefix, center, materials):
    post_positions = (
        unreal.Vector(center.x - 110.0, center.y - 90.0, center.z + 120.0),
        unreal.Vector(center.x - 110.0, center.y + 90.0, center.z + 120.0),
        unreal.Vector(center.x + 110.0, center.y - 90.0, center.z + 120.0),
        unreal.Vector(center.x + 110.0, center.y + 90.0, center.z + 120.0),
    )
    for idx, post_pos in enumerate(post_positions):
        spawn_cylinder(f"{label_prefix}_Post_{idx + 1}", post_pos, unreal.Vector(0.18, 0.18, 2.6), materials["bronze"])

    canopy = spawn_floor(f"{label_prefix}_Canopy", unreal.Vector(center.x, center.y, center.z + 260.0), unreal.Vector(2.7, 2.1, 1.0), materials["linen"])
    canopy.set_actor_rotation(unreal.Rotator(8.0, 0.0, 0.0), False)
    spawn_block(f"{label_prefix}_Table", unreal.Vector(center.x, center.y, center.z + 70.0), unreal.Vector(1.4, 0.6, 0.4), materials["plaster"])
    spawn_cylinder(f"{label_prefix}_Jar_A", unreal.Vector(center.x - 45.0, center.y - 20.0, center.z + 120.0), unreal.Vector(0.22, 0.22, 0.5), materials["sand_stone"])
    spawn_cylinder(f"{label_prefix}_Jar_B", unreal.Vector(center.x + 45.0, center.y + 10.0, center.z + 118.0), unreal.Vector(0.18, 0.18, 0.45), materials["basalt"])


def build_obelisk(label, center, materials):
    spawn_block(f"{label}_Base", unreal.Vector(center.x, center.y, center.z + 35.0), unreal.Vector(0.9, 0.9, 0.35), materials["sand_stone"])
    spawn_cylinder(f"{label}_Body", unreal.Vector(center.x, center.y, center.z + 230.0), unreal.Vector(0.45, 0.45, 4.5), materials["plaster"])
    spawn_cone(f"{label}_Top", unreal.Vector(center.x, center.y, center.z + 485.0), unreal.Vector(0.48, 0.48, 1.0), materials["bronze"])


def build_opening_map(interactable_class, manny_mesh, quinn_mesh, materials):
    unreal.EditorLoadingAndSavingUtils.new_blank_map(False)
    clear_level()
    configure_dynamic_world()
    create_sky_and_lighting()

    spawn_floor("OpeningFloor_Main", unreal.Vector(0.0, 0.0, -15.0), unreal.Vector(60.0, 45.0, 1.0), materials["ash_stone"])
    spawn_floor("OpeningFloor_Ridge", unreal.Vector(-3400.0, 0.0, 220.0), unreal.Vector(20.0, 14.0, 1.0), materials["ash_stone"])

    for idx, x in enumerate((-5200.0, -3400.0, -1600.0, 1600.0, 3600.0, 5200.0)):
        spawn_block(f"OpeningCliff_{idx + 1}", unreal.Vector(x, -2400.0 if idx % 2 == 0 else 2400.0, 380.0), unreal.Vector(7.0, 2.4, 7.5), materials["basalt"])

    spawn_block("CrashTrench", unreal.Vector(400.0, 0.0, -100.0), unreal.Vector(8.0, 2.0, 0.6), materials["wreck_metal"], rotation=unreal.Rotator(0.0, 25.0, -8.0))
    spawn_block("ImpactShard_A", unreal.Vector(950.0, -220.0, 140.0), unreal.Vector(2.6, 1.2, 4.2), materials["wreck_metal"], rotation=unreal.Rotator(24.0, 12.0, 40.0))
    spawn_block("ImpactShard_B", unreal.Vector(-150.0, 340.0, 120.0), unreal.Vector(2.2, 1.4, 3.0), materials["wreck_metal"], rotation=unreal.Rotator(-15.0, -18.0, -30.0))
    spawn_block("ImpactShard_C", unreal.Vector(1450.0, 540.0, 210.0), unreal.Vector(1.6, 1.4, 5.2), materials["wreck_metal"], rotation=unreal.Rotator(30.0, -12.0, 18.0))

    spawn_sphere("MiracleCore", unreal.Vector(250.0, 0.0, 130.0), unreal.Vector(0.55, 0.55, 0.55), materials["miracle"])
    create_point_light("MiracleGlow", unreal.Vector(250.0, 0.0, 160.0), intensity=9000.0, attenuation=2600.0)

    spawn_block("WitnessCamp_Base", unreal.Vector(-1900.0, 900.0, 55.0), unreal.Vector(2.0, 1.4, 0.2), materials["plaster"])
    camp_tarp = spawn_floor("WitnessCamp_Tarp", unreal.Vector(-1900.0, 900.0, 310.0), unreal.Vector(2.0, 1.4, 1.0), materials["linen"])
    camp_tarp.set_actor_rotation(unreal.Rotator(12.0, 0.0, 0.0), False)
    create_point_light("WitnessCampFire", unreal.Vector(-1960.0, 860.0, 70.0), intensity=4200.0, attenuation=1800.0)

    create_player_start(unreal.Vector(-4700.0, -200.0, 180.0), yaw=0.0)

    miracle_anchor = spawn_actor(interactable_class, unreal.Vector(250.0, 0.0, 120.0), label="FirstMiracleAnchor")
    configure_interactable(
        miracle_anchor,
        "Trigger the first miracle",
        unreal.ManyNamesInteractionActionType.FIRST_MIRACLE,
        single_use=True,
    )
    miracle_anchor.get_editor_property("mesh_component").set_material(0, materials["miracle"])

    create_humanoid_interactable(
        interactable_class,
        "WitnessAnchor",
        unreal.Vector(-1840.0, 960.0, 88.0),
        unreal.Rotator(0.0, -35.0, 0.0),
        quinn_mesh,
        materials["priest"],
        "Speak to the witness",
        unreal.ManyNamesInteractionActionType.QUEST_DIALOGUE,
        quest_id="opening_side_01",
        required_outputs=["Story.Prologue.Complete"],
        single_use=True,
    )

    create_npc("FallenCrew_01", manny_mesh, unreal.Vector(-420.0, -520.0, 88.0), 65.0, materials["guard"])
    create_npc("FallenCrew_02", quinn_mesh, unreal.Vector(-760.0, 680.0, 88.0), -55.0, materials["priest"])

    travel_gate = spawn_actor(interactable_class, unreal.Vector(3200.0, 0.0, 150.0), label="EgyptGate")
    configure_interactable(
        travel_gate,
        "Travel to Egypt",
        unreal.ManyNamesInteractionActionType.REGION_TRAVEL,
        target_region=unreal.ManyNamesRegionId.EGYPT,
        required_outputs=["State.Region.Opening.Complete"],
        single_use=False,
    )
    gate_mesh = travel_gate.get_editor_property("mesh_component")
    gate_mesh.set_relative_scale3d(unreal.Vector(2.0, 0.8, 4.0))
    gate_mesh.set_material(0, materials["bronze"])

    spawn_block("EgyptGateFrame_Left", unreal.Vector(3210.0, -180.0, 280.0), unreal.Vector(0.4, 0.6, 5.0), materials["sand_stone"])
    spawn_block("EgyptGateFrame_Right", unreal.Vector(3210.0, 180.0, 280.0), unreal.Vector(0.4, 0.6, 5.0), materials["sand_stone"])
    spawn_block("EgyptGateLintel", unreal.Vector(3210.0, 0.0, 520.0), unreal.Vector(0.4, 2.2, 0.5), materials["bronze"])

    world = unreal.EditorLevelLibrary.get_editor_world()
    if not unreal.EditorLoadingAndSavingUtils.save_map(world, OPENING_MAP):
        fail(f"Failed to save {OPENING_MAP}")


def build_egypt_map(interactable_class, manny_mesh, quinn_mesh, materials):
    unreal.EditorLoadingAndSavingUtils.new_blank_map(False)
    clear_level()
    configure_dynamic_world()
    create_sky_and_lighting()

    spawn_floor("EgyptFloor_Main", unreal.Vector(0.0, 0.0, -15.0), unreal.Vector(100.0, 70.0, 1.0), materials["sand_stone"])
    spawn_floor("EgyptFloor_Necropolis", unreal.Vector(0.0, 4200.0, -10.0), unreal.Vector(36.0, 16.0, 1.0), materials["basalt"])

    for idx, y in enumerate((-3200.0, 3200.0)):
        spawn_block(f"PerimeterWall_EW_{idx + 1}", unreal.Vector(0.0, y, 420.0), unreal.Vector(18.0, 0.7, 8.0), materials["plaster"])
    for idx, x in enumerate((-4700.0, 4700.0)):
        spawn_block(f"PerimeterWall_NS_{idx + 1}", unreal.Vector(x, 0.0, 420.0), unreal.Vector(0.7, 16.0, 8.0), materials["plaster"])

    spawn_block("TempleDais", unreal.Vector(0.0, -150.0, 100.0), unreal.Vector(8.0, 6.0, 1.2), materials["plaster"])
    build_stairs("TempleStairs", unreal.Vector(0.0, -1040.0, 15.0), 6, 700.0, 180.0, 30.0, materials["sand_stone"], axis="y", direction=1.0)

    pool = spawn_floor("TemplePool", unreal.Vector(0.0, -320.0, 32.0), unreal.Vector(4.0, 2.0, 1.0), materials["water"])
    pool.set_actor_rotation(unreal.Rotator(0.0, 0.0, 0.0), False)
    spawn_block("PoolBorder_N", unreal.Vector(0.0, -120.0, 54.0), unreal.Vector(4.2, 0.15, 0.24), materials["bronze"])
    spawn_block("PoolBorder_S", unreal.Vector(0.0, -520.0, 54.0), unreal.Vector(4.2, 0.15, 0.24), materials["bronze"])
    spawn_block("PoolBorder_E", unreal.Vector(420.0, -320.0, 54.0), unreal.Vector(0.15, 2.15, 0.24), materials["bronze"])
    spawn_block("PoolBorder_W", unreal.Vector(-420.0, -320.0, 54.0), unreal.Vector(0.15, 2.15, 0.24), materials["bronze"])

    for idx, x in enumerate((-1250.0, -650.0, 650.0, 1250.0)):
        spawn_cylinder(f"TempleColumn_{idx + 1}", unreal.Vector(x, -180.0, 360.0), unreal.Vector(0.35, 0.35, 7.0), materials["plaster"])
        spawn_block(f"TempleCapital_{idx + 1}", unreal.Vector(x, -180.0, 700.0), unreal.Vector(0.7, 0.7, 0.3), materials["bronze"])

    spawn_block("ArchiveHall", unreal.Vector(2650.0, 0.0, 300.0), unreal.Vector(10.0, 5.0, 6.0), materials["basalt"])
    spawn_block("ArchiveEntryFrame_Left", unreal.Vector(1830.0, -420.0, 220.0), unreal.Vector(0.45, 0.5, 4.0), materials["bronze"])
    spawn_block("ArchiveEntryFrame_Right", unreal.Vector(1830.0, 420.0, 220.0), unreal.Vector(0.45, 0.5, 4.0), materials["bronze"])
    spawn_block("ArchiveLintel", unreal.Vector(1830.0, 0.0, 420.0), unreal.Vector(0.45, 4.7, 0.4), materials["oracle"])

    for idx, y in enumerate((-1400.0, -700.0, 0.0, 700.0, 1400.0)):
        build_market_stall(f"MarketStall_{idx + 1}", unreal.Vector(-2500.0, y, 0.0), materials)

    spawn_block("MarketWall_Back", unreal.Vector(-3650.0, 0.0, 240.0), unreal.Vector(1.2, 10.0, 5.0), materials["sand_stone"])
    spawn_block("MarketWall_Front", unreal.Vector(-1550.0, 0.0, 220.0), unreal.Vector(0.4, 10.0, 4.5), materials["plaster"])

    spawn_block("NecropolisRoad", unreal.Vector(0.0, 3350.0, 40.0), unreal.Vector(4.8, 12.0, 0.2), materials["basalt"])
    build_obelisk("NecropolisObelisk_A", unreal.Vector(-620.0, 2850.0, 0.0), materials)
    build_obelisk("NecropolisObelisk_B", unreal.Vector(620.0, 2850.0, 0.0), materials)
    build_obelisk("NecropolisObelisk_C", unreal.Vector(-620.0, 3850.0, 0.0), materials)
    build_obelisk("NecropolisObelisk_D", unreal.Vector(620.0, 3850.0, 0.0), materials)
    spawn_block("NecropolisGate_Left", unreal.Vector(-320.0, 4550.0, 260.0), unreal.Vector(0.4, 0.6, 4.8), materials["sand_stone"])
    spawn_block("NecropolisGate_Right", unreal.Vector(320.0, 4550.0, 260.0), unreal.Vector(0.4, 0.6, 4.8), materials["sand_stone"])
    spawn_block("NecropolisGate_Top", unreal.Vector(0.0, 4550.0, 500.0), unreal.Vector(1.2, 0.7, 0.4), materials["bronze"])

    for idx, pos in enumerate((
        unreal.Vector(-650.0, -1150.0, 100.0),
        unreal.Vector(650.0, -1150.0, 100.0),
        unreal.Vector(2100.0, -700.0, 88.0),
        unreal.Vector(2100.0, 700.0, 88.0),
        unreal.Vector(-3100.0, -1700.0, 88.0),
        unreal.Vector(-3100.0, 1700.0, 88.0),
    )):
        create_point_light(f"BrazierLight_{idx + 1}", unreal.Vector(pos.x, pos.y, pos.z + 120.0), intensity=3600.0, attenuation=1800.0)
        spawn_sphere(f"BrazierCore_{idx + 1}", unreal.Vector(pos.x, pos.y, pos.z + 80.0), unreal.Vector(0.15, 0.15, 0.15), materials["miracle"])

    create_player_start(unreal.Vector(-4200.0, 0.0, 160.0), yaw=0.0)

    create_humanoid_interactable(
        interactable_class,
        "ArchiveEntry",
        unreal.Vector(1550.0, 0.0, 88.0),
        unreal.Rotator(0.0, 90.0, 0.0),
        manny_mesh,
        materials["oracle"],
        "Speak with the archive keeper",
        unreal.ManyNamesInteractionActionType.QUEST_DIALOGUE,
        quest_id="egypt_main_01",
        required_outputs=["State.Region.Opening.Complete"],
        single_use=True,
    )

    create_humanoid_interactable(
        interactable_class,
        "FloodplainPetitioner",
        unreal.Vector(-2550.0, -950.0, 88.0),
        unreal.Rotator(0.0, 20.0, 0.0),
        quinn_mesh,
        materials["priest"],
        "Hear the floodplain petition",
        unreal.ManyNamesInteractionActionType.QUEST_DIALOGUE,
        quest_id="egypt_side_01",
        required_outputs=["State.Region.Opening.Complete"],
        single_use=True,
    )

    create_npc("TemplePriest_01", quinn_mesh, unreal.Vector(-350.0, -520.0, 88.0), 20.0, materials["priest"])
    create_npc("TemplePriest_02", manny_mesh, unreal.Vector(420.0, -620.0, 88.0), -25.0, materials["priest"])
    create_npc("ArchiveScribe_01", quinn_mesh, unreal.Vector(2200.0, -280.0, 88.0), -90.0, materials["oracle"])
    create_npc("ArchiveScribe_02", manny_mesh, unreal.Vector(2260.0, 320.0, 88.0), -100.0, materials["oracle"])
    create_npc("MarketVendor_01", quinn_mesh, unreal.Vector(-2500.0, -1400.0, 88.0), 90.0, materials["guard"])
    create_npc("MarketVendor_02", manny_mesh, unreal.Vector(-2400.0, 700.0, 88.0), -90.0, materials["guard"])
    create_npc("TempleGuard_01", manny_mesh, unreal.Vector(1050.0, -860.0, 88.0), 180.0, materials["guard"])
    create_npc("TempleGuard_02", manny_mesh, unreal.Vector(1050.0, 860.0, 88.0), 180.0, materials["guard"])
    create_npc("NecropolisWatcher", quinn_mesh, unreal.Vector(0.0, 3650.0, 88.0), 180.0, materials["guard"])
    create_npc("Citizen_01", quinn_mesh, unreal.Vector(-1200.0, 220.0, 88.0), 15.0, materials["priest"])
    create_npc("Citizen_02", manny_mesh, unreal.Vector(-900.0, -40.0, 88.0), 120.0, materials["guard"])

    world = unreal.EditorLevelLibrary.get_editor_world()
    if not unreal.EditorLoadingAndSavingUtils.save_map(world, EGYPT_MAP):
        fail(f"Failed to save {EGYPT_MAP}")


def main():
    ensure_project_directories()
    ensure_mannequin_assets()
    materials = build_material_library()

    interactable_class = unreal.EditorAssetLibrary.load_blueprint_class("/Game/Blueprints/Interaction/BP_Interactable")
    if not interactable_class:
        fail("Failed to load /Game/Blueprints/Interaction/BP_Interactable")

    manny_mesh = unreal.EditorAssetLibrary.load_asset("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple")
    quinn_mesh = unreal.EditorAssetLibrary.load_asset("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple")
    if not manny_mesh or not quinn_mesh:
        fail("Failed to load mannequin skeletal meshes")

    build_opening_map(interactable_class, manny_mesh, quinn_mesh, materials)
    build_egypt_map(interactable_class, manny_mesh, quinn_mesh, materials)
    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    log("Expanded world build complete")


if __name__ == "__main__":
    main()
