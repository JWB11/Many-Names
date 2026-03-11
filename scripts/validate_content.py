#!/usr/bin/env python3

import json
from pathlib import Path
import sys


ROOT = Path(__file__).resolve().parents[1]
DATA = ROOT / "Data"
CONTENT = ROOT / "Content" / "Characters" / "MetaHumans"
MANIFEST_PATH = DATA / "metahuman_manifest.json"
GENERATED_AUDIO = ROOT / "Generated" / "AudioSources"
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


def asset_path_exists(asset_path: str) -> bool:
    if not asset_path:
        return False
    package = asset_path.split(".", 1)[0]
    if not package.startswith("/Game/"):
        return False
    relative = package[len("/Game/"):]
    return (ROOT / "Content" / f"{relative}.uasset").exists()


def main():
    quests = load_json("quests.json")
    dialogue = load_json("dialogue_choices.json")
    steps = load_json("quest_steps.json")
    consequences = load_json("choice_consequences.json")
    endings = load_json("ending_gates.json")
    scenes = load_json("dialogue_scenes.json")
    cast = load_json("character_cast.json")
    ambient = load_json("ambient_profiles.json")
    region_briefs = load_json("region_briefs.json")
    court_factions = load_json("faction_courts.json")
    manifest = load_json("metahuman_manifest.json")
    cinematic_scenes = load_json("cinematic_scenes.json")
    audio_profiles = load_json("audio_profiles.json")
    external_asset_licenses = load_json("external_asset_licenses.json")

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
    unique([row["RegionId"] for row in region_briefs], "region brief region id")
    unique([row["FactionId"] for row in court_factions], "court faction id")
    unique([row["SceneId"] for row in cinematic_scenes], "cinematic scene id")
    unique([row["AudioId"] for row in audio_profiles], "audio profile id")
    unique([row["AssetId"] for row in external_asset_licenses], "external asset license id")

    quest_id_set = set(quest_ids)
    choice_id_set = set(choice_ids)
    consequence_choice_id_set = set(consequence_choice_ids)
    scene_quest_ids = {row["QuestId"] for row in scenes if row.get("QuestId")}
    dialogue_scene_id_set = {row["SceneId"] for row in scenes}
    cast_id_set = {row["CharacterId"] for row in cast}
    faction_id_set = {row["FactionId"] for row in court_factions}
    region_brief_region_ids = {row["RegionId"] for row in region_briefs}
    cinematic_scene_id_set = {row["SceneId"] for row in cinematic_scenes}
    audio_profile_id_set = {row["AudioId"] for row in audio_profiles}
    licensed_asset_ids = {row["AssetId"] for row in external_asset_licenses}
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
        if row.get("CourtId") and row["CourtId"] != "Court.Unclaimed" and not row["CourtId"].startswith("Court."):
            raise SystemExit(f"dialogue scene uses malformed court id: {row['SceneId']}")
        for faction_id in row.get("FactionIds", []):
            if faction_id not in faction_id_set:
                raise SystemExit(f"dialogue scene references unknown faction: {faction_id}")
        for choice_id in row.get("ChoiceIds", []):
            if choice_id not in choice_id_set:
                raise SystemExit(f"dialogue scene references unknown choice: {choice_id}")
        for audio_id in row.get("AudioProfileIds", []):
            if audio_id not in audio_profile_id_set:
                raise SystemExit(f"dialogue scene references unknown audio profile: {audio_id}")

    expected_brief_regions = {"Opening", "Egypt", "Greece", "ItalicWest", "Convergence"}
    if region_brief_region_ids != expected_brief_regions:
        raise SystemExit(f"unexpected region brief coverage: {sorted(region_brief_region_ids)}")

    for row in region_briefs:
        for faction_id in row.get("FactionIds", []):
            if faction_id not in faction_id_set:
                raise SystemExit(f"region brief references unknown faction: {faction_id}")

    for row in court_factions:
        if row["RegionId"] not in expected_brief_regions:
            raise SystemExit(f"court faction uses unexpected region id: {row['FactionId']}")
        if row.get("CourtId") and not row["CourtId"].startswith("Court."):
            raise SystemExit(f"court faction uses malformed court id: {row['FactionId']}")

    for row in cinematic_scenes:
        if row["QuestId"] not in quest_id_set:
            raise SystemExit(f"cinematic scene references unknown quest: {row['QuestId']}")
        if row.get("DialogueSceneId") and row["DialogueSceneId"] not in dialogue_scene_id_set:
            raise SystemExit(f"cinematic scene references unknown dialogue scene: {row['DialogueSceneId']}")
        if row.get("CharacterId") and row["CharacterId"] not in cast_id_set:
            raise SystemExit(f"cinematic scene references unknown character: {row['CharacterId']}")
        for audio_id in row.get("AudioProfileIds", []):
            if audio_id not in audio_profile_id_set:
                raise SystemExit(f"cinematic scene references unknown audio profile: {audio_id}")
        if row.get("SequenceAsset") and not asset_path_exists(row["SequenceAsset"]):
            raise SystemExit(f"cinematic scene missing sequence asset: {row['SceneId']}")

    for row in audio_profiles:
        source_file = row.get("SourceFile", "")
        if not source_file:
            raise SystemExit(f"audio profile missing source file: {row['AudioId']}")
        if not (GENERATED_AUDIO / source_file).exists():
            raise SystemExit(
                f"audio profile source file missing on disk: {source_file}. "
                "Run scripts/generate_alpha_audio.py before validating content."
            )
        if row.get("SoundAsset") and not asset_path_exists(row["SoundAsset"]):
            raise SystemExit(f"audio profile missing imported sound asset: {row['AudioId']}")

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
        if row["ChoiceId"] not in choice_id_set:
            raise SystemExit(f"choice consequence references unknown choice: {row['ChoiceId']}")
        if row["QuestId"] not in quest_id_set:
            raise SystemExit(f"choice consequence references unknown quest: {row['QuestId']}")
        matching_choice = next((choice for choice in dialogue if choice["ChoiceId"] == row["ChoiceId"]), None)
        if matching_choice and matching_choice["QuestId"] != row["QuestId"]:
            raise SystemExit(
                "choice consequence quest mismatch: "
                f"{row['ChoiceId']} choice quest={matching_choice['QuestId']} consequence quest={row['QuestId']}"
            )

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
        side_quest_ids = [row["QuestId"] for row in quests if row["RegionId"] == region and "_side_" in row["QuestId"]]
        for side_quest_id in side_quest_ids:
            side_consequences = consequence_by_quest[side_quest_id]
            if not any(item["CanAvoidCombat"] for item in side_consequences):
                raise SystemExit(f"no non-combat path for {side_quest_id}")
            if not any(item["CanTriggerCombat"] for item in side_consequences):
                raise SystemExit(f"no combat-capable path for {side_quest_id}")

    required_license_ids = {
        "tool_blender",
        "tool_audacity",
        "tool_ffmpeg",
        "tool_krita",
        "tool_gimp",
        "tool_material_maker",
        "tool_lmms",
        "tool_piper",
        "generated_audio_alpha",
        "generated_sequences_alpha",
    }
    missing_license_ids = sorted(required_license_ids - licensed_asset_ids)
    if missing_license_ids:
        raise SystemExit(f"missing external asset license records: {', '.join(missing_license_ids)}")

    print("Content validation passed.")
    print(f"Quests: {len(quests)}")
    print(f"Quest steps: {len(steps)}")
    print(f"Dialogue choices: {len(dialogue)}")
    print(f"Choice consequences: {len(consequences)}")
    print(f"Ending gates: {len(endings)}")
    print(f"Dialogue scenes: {len(scenes)}")
    print(f"Cinematic scenes: {len(cinematic_scenes)}")
    print(f"Audio profiles: {len(audio_profiles)}")
    print(f"External licenses: {len(external_asset_licenses)}")
    print(f"Character cast: {len(cast)}")
    print(f"Ambient profiles: {len(ambient)}")
    print(f"Region briefs: {len(region_briefs)}")
    print(f"Court factions: {len(court_factions)}")


if __name__ == "__main__":
    try:
        main()
    except SystemExit:
        raise
    except Exception as exc:
        print(f"validation failed: {exc}", file=sys.stderr)
        raise SystemExit(1)
