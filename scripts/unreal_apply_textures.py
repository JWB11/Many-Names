import unreal


TARGET_MAPS = (
    "/Game/Maps/L_OpeningCatastrophe",
    "/Game/Maps/L_EgyptHub",
)


def log(message):
    unreal.log(f"[ManyNamesTexturePass] {message}")


def get_mesh_component(actor):
    if actor.get_class().get_name() == "StaticMeshActor":
        return actor.get_editor_property("static_mesh_component")
    return actor.get_editor_property("mesh_component")


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
        raise RuntimeError(f"Failed to create {asset_path}")
    return material


def create_surface_material(asset_name, tint, roughness, metallic=0.0, emissive=None, emissive_strength=0.0):
    unreal.EditorAssetLibrary.make_directory("/Game/Materials")
    material = get_or_create_material(asset_name)
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
    return material


def build_material_library():
    return {
        "OpeningFloor": create_surface_material("M_MN_AshStone", unreal.LinearColor(0.22, 0.22, 0.24, 1.0), 0.92),
        "EgyptFloor": create_surface_material("M_MN_SandStone", unreal.LinearColor(0.67, 0.56, 0.38, 1.0), 0.88),
        "ArchiveWallLeft": create_surface_material("M_MN_SandStone", unreal.LinearColor(0.67, 0.56, 0.38, 1.0), 0.88),
        "ArchiveWallRight": create_surface_material("M_MN_SandStone", unreal.LinearColor(0.67, 0.56, 0.38, 1.0), 0.88),
        "WreckShardA": create_surface_material("M_MN_WreckMetal", unreal.LinearColor(0.25, 0.19, 0.16, 1.0), 0.55, 0.45, unreal.LinearColor(0.7, 0.18, 0.08, 1.0), 0.3),
        "WreckShardB": create_surface_material("M_MN_WreckMetal", unreal.LinearColor(0.25, 0.19, 0.16, 1.0), 0.55, 0.45, unreal.LinearColor(0.7, 0.18, 0.08, 1.0), 0.3),
        "ArchiveThreshold": create_surface_material("M_MN_Bronze", unreal.LinearColor(0.60, 0.42, 0.18, 1.0), 0.34, 0.9),
        "EgyptGate": create_surface_material("M_MN_Bronze", unreal.LinearColor(0.60, 0.42, 0.18, 1.0), 0.34, 0.9),
        "FirstMiracleAnchor": create_surface_material("M_MN_Miracle", unreal.LinearColor(0.10, 0.04, 0.03, 1.0), 0.18, 0.05, unreal.LinearColor(1.0, 0.34, 0.10, 1.0), 12.0),
        "WitnessAnchor": create_surface_material("M_MN_Oracle", unreal.LinearColor(0.08, 0.11, 0.16, 1.0), 0.22, 0.15, unreal.LinearColor(0.96, 0.76, 0.22, 1.0), 4.0),
        "ArchiveEntry": create_surface_material("M_MN_Oracle", unreal.LinearColor(0.08, 0.11, 0.16, 1.0), 0.22, 0.15, unreal.LinearColor(0.96, 0.76, 0.22, 1.0), 4.0),
    }


def apply_materials_to_map(asset_path, materials):
    world = unreal.EditorLoadingAndSavingUtils.load_map(asset_path)
    if not world:
        raise RuntimeError(f"Failed to load {asset_path}")

    for actor in unreal.EditorLevelLibrary.get_all_level_actors():
        label = actor.get_actor_label()
        material = materials.get(label)
        if not material:
            continue
        get_mesh_component(actor).set_material(0, material)
        log(f"Applied {material.get_name()} to {label}")

    if not unreal.EditorLoadingAndSavingUtils.save_map(world, asset_path):
        raise RuntimeError(f"Failed to save {asset_path}")


def main():
    materials = build_material_library()
    for asset_path in TARGET_MAPS:
        apply_materials_to_map(asset_path, materials)


if __name__ == "__main__":
    main()
