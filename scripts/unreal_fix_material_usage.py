import unreal


MATERIAL_PATHS = [
    "/Game/Materials/M_MN_Bronze",
    "/Game/Materials/M_MN_AshStone",
    "/Game/Materials/M_MN_SandStone",
    "/Game/Materials/M_MN_Oracle",
    "/Game/Materials/M_MN_Miracle",
]


def main():
    fixed = 0
    for material_path in MATERIAL_PATHS:
        material = unreal.load_asset(material_path)
        if not material:
            unreal.log_warning(f"[ManyNames] Missing material: {material_path}")
            continue

        changed = False
        for property_name in ("used_with_nanite", "used_with_instanced_static_meshes"):
            try:
                if not material.get_editor_property(property_name):
                    material.set_editor_property(property_name, True)
                    changed = True
            except Exception:
                pass

        if changed:
            unreal.EditorAssetLibrary.save_loaded_asset(material, only_if_is_dirty=False)
            fixed += 1

    unreal.log(f"[ManyNames] Material usage fix complete. Updated {fixed} materials.")


if __name__ == "__main__":
    main()
