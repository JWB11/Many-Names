import unreal


def dump_methods(label, obj):
    names = [name for name in dir(obj) if not name.startswith("_")]
    print(f"{label}:")
    for name in names:
        print(f"  {name}")


dump_methods("AssetTools", unreal.AssetToolsHelpers.get_asset_tools())
dump_methods("EditorAssetLibrary", unreal.EditorAssetLibrary)
dump_methods("EditorLevelLibrary", unreal.EditorLevelLibrary)
dump_methods("EditorLoadingAndSavingUtils", unreal.EditorLoadingAndSavingUtils)
dump_methods("CSVImportFactory", unreal.CSVImportFactory)
dump_methods("DataTableFactory", unreal.DataTableFactory)
print("Has DataTableFunctionLibrary:", hasattr(unreal, "DataTableFunctionLibrary"))
if hasattr(unreal, "DataTableFunctionLibrary"):
    dump_methods("DataTableFunctionLibrary", unreal.DataTableFunctionLibrary)
