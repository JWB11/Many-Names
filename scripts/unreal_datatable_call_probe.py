import os
import unreal


PROJECT_DIR = unreal.Paths.project_dir()
JSON_PATH = os.path.join(PROJECT_DIR, "Data", "regions.json")
ASSET_PATH = "/Game/Data/DT_ProbeRegions"


def load_region_struct():
    if hasattr(unreal, "ManyNamesRegionRow"):
        try:
            return unreal.ManyNamesRegionRow.static_struct()
        except Exception as exc:
            print("static_struct failed:", exc)
    return unreal.load_object(None, "/Script/ManyNames.ManyNamesRegionRow")


struct = load_region_struct()
print("row_struct:", struct)

unreal.EditorAssetLibrary.make_directory("/Game/Data")
if unreal.EditorAssetLibrary.does_asset_exist(ASSET_PATH):
    unreal.EditorAssetLibrary.delete_asset(ASSET_PATH)

factory = unreal.DataTableFactory()
factory.set_editor_property("struct", struct)
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
datatable = asset_tools.create_asset("DT_ProbeRegions", "/Game/Data", unreal.DataTable, factory)
print("datatable:", datatable)

with open(JSON_PATH, "r", encoding="utf-8") as handle:
    json_text = handle.read()

result = datatable.call_method("CreateTableFromJSONString", args=(json_text,))
print("result:", result)
unreal.EditorAssetLibrary.save_asset(ASSET_PATH)
