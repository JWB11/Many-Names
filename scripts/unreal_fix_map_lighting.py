import unreal


TARGET_MAPS = (
    "/Game/Maps/L_OpeningCatastrophe",
    "/Game/Maps/L_EgyptHub",
)


def log(message):
    unreal.log(f"[ManyNamesLightingFix] {message}")


def set_actor_mobility(actor, property_name):
    component = actor.get_editor_property(property_name)
    component.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)


def fix_current_map(asset_path):
    world = unreal.EditorLoadingAndSavingUtils.load_map(asset_path)
    if not world:
        raise RuntimeError(f"Failed to load map {asset_path}")

    world_settings = world.get_world_settings()
    world_settings.set_editor_property("force_no_precomputed_lighting", True)

    for actor in unreal.EditorLevelLibrary.get_all_level_actors():
        class_name = actor.get_class().get_name()
        if class_name == "DirectionalLight":
            set_actor_mobility(actor, "directional_light_component")
        elif class_name == "SkyLight":
            set_actor_mobility(actor, "light_component")

    if not unreal.EditorLoadingAndSavingUtils.save_map(world, asset_path):
        raise RuntimeError(f"Failed to save map {asset_path}")

    log(f"Updated dynamic lighting settings for {asset_path}")


def main():
    for asset_path in TARGET_MAPS:
        fix_current_map(asset_path)


if __name__ == "__main__":
    main()
