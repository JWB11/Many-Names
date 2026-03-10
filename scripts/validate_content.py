#!/usr/bin/env python3

import json
from pathlib import Path
import sys


ROOT = Path(__file__).resolve().parents[1]
DATA = ROOT / "Data"
CONTENT = ROOT / "Content" / "Characters" / "MetaHumans"
MANIFEST_PATH = DATA / "metahuman_manifest.json"
FORBIDDEN_RUNTIME_MARKERS = ("Manny", "Quinn", "SKM_Manny", "SKM_Quinn", "MetaHumanA", "MetaHumanB", "MetaHumanC")


def load_json(name: str):
    with open(DATA / name, "r", encoding="utf-8") as handle:
        return json.load(handle)


def unique(values, label):
    seen = set()
    for value in values:
        if value in seen:
            raise SystemExit(f"duplicate {label}: {value}")
        seen.add(value)


def main():
    quests = load_json("quests.json")
    dialogue = load_json("dialogue_choices.json")
    steps = load_json("quest_steps.json")
    consequences = load_json("choice_consequences.json")
    endings = load_json("ending_gates.json")
    scenes = load_json("dialogue_scenes.json")
    cast = load_json("character_cast.json")
    ambient = load_json("ambient_profiles.json")
    manifest = load_json("metahuman_manifest.json")

    quest_ids = [row["QuestId"] for row in quests]
    choice_ids = [row["ChoiceId"] for row in dialogue]
    consequence_choice_ids = [row["ChoiceId"] for row in consequences]

    unique(quest_ids, "quest id")
    unique(choice_ids, "choice id")
    unique(consequence_choice_ids, "consequence choice id")
    unique([row["StepId"] for row in steps], "step id")
    unique([row["EndingId"] for row in endings], "ending id")
    unique([row["SceneId"] for row in scenes], "scene id")
    unique([row["CharacterId"] for row in cast], "character id")
    unique([row["ProfileId"] for row in ambient], "ambient profile id")

    quest_id_set = set(quest_ids)
    choice_id_set = set(choice_ids)
    consequence_choice_id_set = set(consequence_choice_ids)
    scene_quest_ids = {row["QuestId"] for row in scenes if row.get("QuestId")}
    cast_id_set = {row["CharacterId"] for row in cast}
    non_human_projection_ids = {"ConvergenceCore", "LegacyProjection", "OracleFragment"}
    required_named_runtime_ids = sorted(cast_id_set - non_human_projection_ids)

    for row in steps:
        if row["QuestId"] not in quest_id_set:
            raise SystemExit(f"quest_steps references unknown quest: {row['QuestId']}")
        for choice_id in row.get("UnlockChoiceIds", []):
            if choice_id not in choice_id_set:
                raise SystemExit(f"quest_steps references unknown choice: {choice_id}")

    for row in dialogue:
        if row["QuestId"] not in quest_id_set:
            raise SystemExit(f"dialogue choice references unknown quest: {row['QuestId']}")

    for row in scenes:
        quest_id = row.get("QuestId")
        if quest_id and quest_id not in quest_id_set:
            raise SystemExit(f"dialogue scene references unknown quest: {quest_id}")
        if row.get("CharacterId") and row["CharacterId"] not in cast_id_set:
            raise SystemExit(f"dialogue scene references unknown character: {row['CharacterId']}")
        for choice_id in row.get("ChoiceIds", []):
            if choice_id not in choice_id_set:
                raise SystemExit(f"dialogue scene references unknown choice: {choice_id}")

    for row in ambient:
        for character_id in row.get("CharacterIds", []):
            if character_id not in cast_id_set:
                raise SystemExit(f"ambient profile references unknown character: {character_id}")

    for row in cast:
        character_id = row["CharacterId"]
        if character_id in non_human_projection_ids:
            continue
        asset_path = CONTENT / f"{character_id}.uasset"
        if not asset_path.exists():
            raise SystemExit(f"missing MetaHumanCharacter asset for: {character_id}")

    manifest_named = manifest.get("named", {})
    manifest_authored = manifest.get("named_character_assets", {})
    for character_id in required_named_runtime_ids:
        if character_id not in manifest_authored or not manifest_authored[character_id]:
            raise SystemExit(f"manifest missing authored named character asset: {character_id}")
        if character_id not in manifest_named or not manifest_named[character_id]:
            raise SystemExit(f"manifest missing runtime named character mesh: {character_id}")
        if any(marker in manifest_named[character_id] for marker in FORBIDDEN_RUNTIME_MARKERS):
            raise SystemExit(f"manifest uses fallback runtime mesh for named character: {character_id}")

    for row in ambient:
        profile_id = row["ProfileId"]
        if not row.get("CharacterIds"):
            raise SystemExit(f"ambient profile missing character links: {profile_id}")
        if not any((CONTENT / f"{character_id}.uasset").exists() for character_id in row["CharacterIds"]):
            raise SystemExit(f"ambient profile has no authored human asset available: {profile_id}")

    for choice_id in choice_id_set:
        if choice_id not in consequence_choice_id_set:
            raise SystemExit(f"missing choice consequence for: {choice_id}")

    for row in consequences:
        if row["QuestId"] not in quest_id_set:
            raise SystemExit(f"choice consequence references unknown quest: {row['QuestId']}")

    required_regions = {"Opening", "Egypt", "Greece", "ItalicWest", "Convergence"}
    regions_seen = {row["RegionId"] for row in quests}
    if regions_seen != required_regions:
        raise SystemExit(f"unexpected quest region coverage: {sorted(regions_seen)}")

    main_side_expectations = {
        "Opening": {"main": False, "side": False},
        "Egypt": {"main": False, "side": False},
        "Greece": {"main": False, "side": False},
        "ItalicWest": {"main": False, "side": False},
    }
    for row in quests:
        quest_id = row["QuestId"]
        region_id = row["RegionId"]
        if region_id in main_side_expectations:
            if "_main_" in quest_id:
                main_side_expectations[region_id]["main"] = True
            if "_side_" in quest_id:
                main_side_expectations[region_id]["side"] = True
        if not row["WorldStateOutputId"]:
            raise SystemExit(f"quest missing world-state output: {quest_id}")
        if quest_id not in scene_quest_ids:
            raise SystemExit(f"quest missing dialogue scene: {quest_id}")

    for region_id, coverage in main_side_expectations.items():
        if not coverage["main"] or not coverage["side"]:
            raise SystemExit(f"incomplete main/side coverage for region: {region_id}")

    consequence_by_quest = {}
    for row in consequences:
        consequence_by_quest.setdefault(row["QuestId"], []).append(row)

    for region in ["Egypt", "Greece", "ItalicWest"]:
        side_quest_id = next(row["QuestId"] for row in quests if row["RegionId"] == region and "_side_" in row["QuestId"])
        side_consequences = consequence_by_quest[side_quest_id]
        if not any(item["CanAvoidCombat"] for item in side_consequences):
            raise SystemExit(f"no non-combat path for {side_quest_id}")
        if not any(item["CanTriggerCombat"] for item in side_consequences):
            raise SystemExit(f"no combat-capable path for {side_quest_id}")

    print("Content validation passed.")
    print(f"Quests: {len(quests)}")
    print(f"Quest steps: {len(steps)}")
    print(f"Dialogue choices: {len(dialogue)}")
    print(f"Choice consequences: {len(consequences)}")
    print(f"Ending gates: {len(endings)}")
    print(f"Dialogue scenes: {len(scenes)}")
    print(f"Character cast: {len(cast)}")
    print(f"Ambient profiles: {len(ambient)}")


if __name__ == "__main__":
    try:
        main()
    except SystemExit:
        raise
    except Exception as exc:
        print(f"validation failed: {exc}", file=sys.stderr)
        raise SystemExit(1)
