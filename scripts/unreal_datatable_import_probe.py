import os
import unreal


PROJECT_DIR = unreal.Paths.project_dir()
JSON_PATH = os.path.join(PROJECT_DIR, "Data", "regions.json")
DEST_PATH = "/Game/Data"
DEST_NAME = "DT_ImportProbeRegions"


def load_region_struct():
    if hasattr(unreal, "ManyNamesRegionRow"):
        try:
            return unreal.ManyNamesRegionRow.static_struct()
        except Exception:
            pass
    return unreal.load_object(None, "/Script/ManyNames.ManyNamesRegionRow")


unreal.EditorAssetLibrary.make_directory(DEST_PATH)
asset_path = f"{DEST_PATH}/{DEST_NAME}"
if unreal.EditorAssetLibrary.does_asset_exist(asset_path):
    unreal.EditorAssetLibrary.delete_asset(asset_path)

factory = unreal.CSVImportFactory()
settings = factory.get_editor_property("automated_import_settings")
settings.set_editor_property("import_row_struct", load_region_struct())
factory.set_editor_property("automated_import_settings", settings)

task = unreal.AssetImportTask()
task.set_editor_property("filename", JSON_PATH)
task.set_editor_property("destination_path", DEST_PATH)
task.set_editor_property("destination_name", DEST_NAME)
task.set_editor_property("replace_existing", True)
task.set_editor_property("automated", True)
task.set_editor_property("save", True)
task.set_editor_property("factory", factory)

unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
print("imported:", task.get_editor_property("imported_object_paths"))
print("errors:", task.get_editor_property("result"))
