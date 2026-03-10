import unreal


print("DataTable.create_table_from_json_string:", hasattr(unreal.DataTable, "create_table_from_json_string"))
print("EditorLevelLibrary.new_level:", hasattr(unreal.EditorLevelLibrary, "new_level"))
print("EditorLevelLibrary.spawn_actor_from_class:", hasattr(unreal.EditorLevelLibrary, "spawn_actor_from_class"))
print("EditorLevelLibrary.save_current_level:", hasattr(unreal.EditorLevelLibrary, "save_current_level"))
print("EditorLoadingAndSavingUtils.save_dirty_packages:", hasattr(unreal.EditorLoadingAndSavingUtils, "save_dirty_packages"))

bp_factory = unreal.BlueprintFactory()
widget_factory = unreal.WidgetBlueprintFactory()
dt_factory = unreal.DataTableFactory()

print("BlueprintFactory parent_class:", hasattr(bp_factory, "parent_class"), bp_factory.get_editor_property("parent_class"))
print("WidgetBlueprintFactory parent_class:", hasattr(widget_factory, "parent_class"), widget_factory.get_editor_property("parent_class"))
print("DataTableFactory struct:", hasattr(dt_factory, "struct"), dt_factory.get_editor_property("struct"))
