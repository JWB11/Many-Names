import unreal


ASSET_PATHS = [
    "/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple",
    "/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple",
    "/Game/Characters/Mannequins/Meshes/SK_Mannequin",
    "/Game/Marketplace/Fab/EnvironmentEgypt/AncientEgyptTempleCollection/EGYPCIAN_FLOOR1",
    "/Game/Marketplace/Fab/EnvironmentEgypt/PtahshepsesMastabaEntrance/ABUSIR2",
    "/Game/Marketplace/Fab/EnvironmentGreece/StoneAgeDolmen/capeshj_lowpoly",
    "/Game/Marketplace/Fab/EnvironmentItalic/ChurchRock/church_rock1",
    "/Game/Marketplace/Fab/EnvironmentItalic/RoundedCornerBollard/round-corner1",
    "/Game/Marketplace/Fab/EnvironmentConvergence/CuttedDestroyedWood/shareModel",
]

MAP_PATHS = [
    "/Game/Maps/L_OpeningCatastrophe",
    "/Game/Maps/L_EgyptHub",
    "/Game/Maps/L_GreeceHub",
    "/Game/Maps/L_ItalicHub",
    "/Game/Maps/L_Convergence",
]


def describe_asset(path: str) -> None:
    asset = unreal.EditorAssetLibrary.load_asset(path)
    if not asset:
        unreal.log_warning(f"[Inspect] missing asset {path}")
        return

    asset_class = asset.get_class().get_name()
    extras = []

    if hasattr(asset, "get_editor_property"):
        for field in ("skeleton", "physics_asset", "anim_class"):
            try:
                value = asset.get_editor_property(field)
            except Exception:
                value = None
            if value:
                extras.append(f"{field}={value.get_name() if hasattr(value, 'get_name') else value}")

    extra_text = f" ({', '.join(extras)})" if extras else ""
    unreal.log(f"[Inspect] asset={path} class={asset_class}{extra_text}")


def describe_map(path: str) -> None:
    world = unreal.EditorLoadingAndSavingUtils.load_map(path)
    if not world:
        unreal.log_warning(f"[Inspect] failed to load map {path}")
        return

    actors = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor)
    labels = []
    for actor in actors[:20]:
        try:
            labels.append(actor.get_actor_label())
        except Exception:
            labels.append(actor.get_name())

    unreal.log(f"[Inspect] map={path} actor_count={len(actors)} sample={labels}")


for asset_path in ASSET_PATHS:
    describe_asset(asset_path)

for map_path in MAP_PATHS:
    describe_map(map_path)
