import unreal


factory = unreal.DataTableFactory()
if hasattr(unreal, "ManyNamesRegionRow"):
    struct = unreal.ManyNamesRegionRow.static_struct()
else:
    struct = unreal.load_object(None, "/Script/ManyNames.ManyNamesRegionRow")

factory.set_editor_property("struct", struct)
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
datatable = asset_tools.create_asset("DT_MethodProbe", "/Game/Data", unreal.DataTable, factory)

for name in dir(datatable):
    if "row" in name.lower() or "table" in name.lower() or "csv" in name.lower() or "json" in name.lower():
        print(name)
