import unreal


def log(message):
    unreal.log(f"[ManyNamesVerify] {message}")


def load_table(asset_path):
    table = unreal.EditorAssetLibrary.load_asset(asset_path)
    if not table:
        raise RuntimeError(f"Missing DataTable {asset_path}")
    return table


def preview_column(table, column_name, preview_count=3):
    values = unreal.DataTableFunctionLibrary.get_data_table_column_as_string(table, column_name)
    return values[:preview_count]


def verify_regions():
    table = load_table("/Game/Data/DT_Regions")
    row_names = unreal.DataTableFunctionLibrary.get_data_table_row_names(table)
    entry_conditions = preview_column(table, "EntryConditions")
    quest_ids = preview_column(table, "QuestIds")
    log(f"DT_Regions rows={len(row_names)} entry_conditions={entry_conditions} quest_ids={quest_ids}")


def verify_quests():
    table = load_table("/Game/Data/DT_Quests")
    row_names = unreal.DataTableFunctionLibrary.get_data_table_row_names(table)
    required_domains = preview_column(table, "RequiredDomains", 5)
    reward_domains = preview_column(table, "RewardDomains", 5)
    log(f"DT_Quests rows={len(row_names)} required_domains={required_domains} reward_domains={reward_domains}")


def verify_dialogue_choices():
    table = load_table("/Game/Data/DT_DialogueChoices")
    row_names = unreal.DataTableFunctionLibrary.get_data_table_row_names(table)
    required_domains = preview_column(table, "RequiredDomains", 5)
    granted_domains = preview_column(table, "GrantedDomains", 5)
    result_tags = preview_column(table, "ResultTags", 5)
    log(
        "DT_DialogueChoices rows={} required_domains={} granted_domains={} result_tags={}".format(
            len(row_names),
            required_domains,
            granted_domains,
            result_tags,
        )
    )


def main():
    verify_regions()
    verify_quests()
    verify_dialogue_choices()


if __name__ == "__main__":
    main()
