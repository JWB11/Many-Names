#!/usr/bin/env python3

import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
DATA = ROOT / "Data"


def load_json(name: str):
    return json.loads((DATA / name).read_text(encoding="utf-8"))


def save_json(name: str, payload) -> None:
    (DATA / name).write_text(json.dumps(payload, indent=2, ensure_ascii=True) + "\n", encoding="utf-8")


def upsert(rows, row, key):
    index = {item[key]: pos for pos, item in enumerate(rows)}
    if row[key] in index:
        rows[index[row[key]]] = row
    else:
        rows.append(row)


def ensure_region_quests(regions, region_id, new_quest_ids):
    for row in regions:
        if row["RegionId"] == region_id:
            existing = row.get("QuestIds", [])
            for quest_id in new_quest_ids:
                if quest_id not in existing:
                    existing.append(quest_id)
            row["QuestIds"] = existing
            break


def main():
    regions = load_json("regions.json")
    quests = load_json("quests.json")
    steps = load_json("quest_steps.json")
    scenes = load_json("dialogue_scenes.json")
    choices = load_json("dialogue_choices.json")
    consequences = load_json("choice_consequences.json")
    cast = load_json("character_cast.json")
    ambient = load_json("ambient_profiles.json")

    new_cast = [
        {
            "CharacterId": "RiverScribe",
            "RegionId": "Egypt",
            "bNamedCharacter": True,
            "DisplayName": "Henut",
            "Occupation": "River Scribe",
            "Origin": "Canal estate south of Memphis",
            "Presentation": "Woman",
            "AgeRange": "Early 30s",
            "PhysicalBuild": "Lean, reed-pen hands, long-distance walking endurance",
            "SkinToneNotes": "Warm brown complexion with river glare and dust wear",
            "HairNotes": "Dark textured hair coiled and pinned beneath a linen wrap",
            "GroomingNotes": "Ink-stained fingertips, practical presentation, little ornament",
            "WardrobeNotes": "Layered linen kilt and shoulder wrap, belt pouches for tablets, faded blue edging",
            "DemeanorNotes": "Precise, watchful, compassionate when records fail the poor",
            "OccupationTag": "Scribe",
            "ClothingVariantId": "egypt_scribe_named",
            "StanceId": "named_scribe"
        },
        {
            "CharacterId": "TempleAdjudicator",
            "RegionId": "Egypt",
            "bNamedCharacter": True,
            "DisplayName": "Panehsy",
            "Occupation": "Temple Adjudicator",
            "Origin": "Western bank administrative quarter",
            "Presentation": "Man",
            "AgeRange": "Late 40s",
            "PhysicalBuild": "Broad shouldered, ceremonial stillness, age held in the knees",
            "SkinToneNotes": "Deep bronze-brown complexion, sun-aged and lined",
            "HairNotes": "Close-cropped dark hair beneath a formal band",
            "GroomingNotes": "Ritually neat, trimmed beard, carefully maintained status markers",
            "WardrobeNotes": "Heavy linen with pleated front panel, bronze seal cord, dust-softened white layers",
            "DemeanorNotes": "Measured, severe, always calculating what public order can survive",
            "OccupationTag": "Adjudicator",
            "ClothingVariantId": "egypt_adjudicator_named",
            "StanceId": "named_adjudicator"
        },
        {
            "CharacterId": "OathBearer",
            "RegionId": "Greece",
            "bNamedCharacter": True,
            "DisplayName": "Melia",
            "Occupation": "Oath Bearer",
            "Origin": "Aegean upland sanctuary circuit",
            "Presentation": "Woman",
            "AgeRange": "Mid 20s",
            "PhysicalBuild": "Hardy climber build, wind-bent posture from exposed ridges",
            "SkinToneNotes": "Olive complexion with strong sun and salt exposure",
            "HairNotes": "Dark wavy hair braided back with wool ties",
            "GroomingNotes": "Minimal ritual paint, practical and severe",
            "WardrobeNotes": "Layered wool peplos, storm-dark mantle, bronze fastener, frayed hems from travel",
            "DemeanorNotes": "Intense, devout, speaks as if every promise is a public binding",
            "OccupationTag": "OathBearer",
            "ClothingVariantId": "greece_oath_named",
            "StanceId": "named_oath"
        },
        {
            "CharacterId": "HillOracle",
            "RegionId": "Greece",
            "bNamedCharacter": True,
            "DisplayName": "Thestor",
            "Occupation": "Hill Oracle",
            "Origin": "Coastal shrine above the gulf",
            "Presentation": "Man",
            "AgeRange": "Late 30s",
            "PhysicalBuild": "Lean ritual frame, sleepless eyes, hands scarred by incense braziers",
            "SkinToneNotes": "Weathered Mediterranean complexion with mountain wind burn",
            "HairNotes": "Dark hair with silver at the temples, loosely tied",
            "GroomingNotes": "Sparse beard, wind-frayed, severe profile",
            "WardrobeNotes": "Pale wool tunic, smoke-stained cloak, narrow bronze amulets and prayer cords",
            "DemeanorNotes": "Prophetic, charismatic, half-fearful of what he truly hears",
            "OccupationTag": "Oracle",
            "ClothingVariantId": "greece_oracle_named",
            "StanceId": "named_oracle"
        },
        {
            "CharacterId": "RoadMagistrate",
            "RegionId": "ItalicWest",
            "bNamedCharacter": True,
            "DisplayName": "Serranus",
            "Occupation": "Road Magistrate",
            "Origin": "Hill settlement civic quarter",
            "Presentation": "Man",
            "AgeRange": "Mid 40s",
            "PhysicalBuild": "Compact civic build, heavy forearms from practical labor",
            "SkinToneNotes": "Warm tan western Mediterranean complexion darkened by field work",
            "HairNotes": "Dark cropped hair with early grey",
            "GroomingNotes": "Close beard, practical, ash and road dust set into the cloth",
            "WardrobeNotes": "Wool tunic, leather belt, road-measure cords, heavier mantle for damp weather",
            "DemeanorNotes": "Pragmatic, skeptical, dangerous when public order is threatened",
            "OccupationTag": "Magistrate",
            "ClothingVariantId": "italic_magistrate_named",
            "StanceId": "named_magistrate"
        },
        {
            "CharacterId": "ForgeApprentice",
            "RegionId": "ItalicWest",
            "bNamedCharacter": True,
            "DisplayName": "Lartha",
            "Occupation": "Forge Apprentice",
            "Origin": "Workshop ward beneath the hill shrine",
            "Presentation": "Woman",
            "AgeRange": "Late teens",
            "PhysicalBuild": "Compact and strong, soot-marked, quick-footed",
            "SkinToneNotes": "Olive-brown complexion with soot, sparks, and forge heat wear",
            "HairNotes": "Dark curly hair wrapped tight in a leather band",
            "GroomingNotes": "Functional, soot-streaked, no precious ornament",
            "WardrobeNotes": "Wool shift, leather apron, layered wraps at the forearms, burn-marked hems",
            "DemeanorNotes": "Sharp, ambitious, openly measures every adult against their usefulness",
            "OccupationTag": "Apprentice",
            "ClothingVariantId": "italic_forge_named",
            "StanceId": "named_apprentice"
        },
        {
            "CharacterId": "BurialCustodian",
            "RegionId": "Convergence",
            "bNamedCharacter": True,
            "DisplayName": "Nera",
            "Occupation": "Burial Custodian",
            "Origin": "Seal-keeper line above the buried descent",
            "Presentation": "Woman",
            "AgeRange": "Mid 50s",
            "PhysicalBuild": "Spare, deliberate, carries ritual stillness more than visible force",
            "SkinToneNotes": "Pale olive complexion dimmed by subterranean light and dust",
            "HairNotes": "Grey-dark hair bound in layered wraps",
            "GroomingNotes": "Austere, ceremonial, dry dust set into every fold",
            "WardrobeNotes": "Layered burial cloth, dark wool, seal cords, ash-faded trim",
            "DemeanorNotes": "Quiet, implacable, sees preservation as a kind of mercy",
            "OccupationTag": "Custodian",
            "ClothingVariantId": "convergence_custodian_named",
            "StanceId": "named_custodian"
        },
        {
            "CharacterId": "SignalArchivist",
            "RegionId": "Convergence",
            "bNamedCharacter": True,
            "DisplayName": "Kheiron",
            "Occupation": "Signal Archivist",
            "Origin": "Surface ruin relay enclave",
            "Presentation": "Man",
            "AgeRange": "Early 30s",
            "PhysicalBuild": "Lean scholar build, sleepless and unnaturally poised",
            "SkinToneNotes": "Muted bronze complexion under cold interior light",
            "HairNotes": "Dark cropped hair with rigid formal parting",
            "GroomingNotes": "Severe, almost machine-clean, uncanny in stillness",
            "WardrobeNotes": "Dark layered wraps, archive cords, metallic fastening scraps integrated into cloth",
            "DemeanorNotes": "Obsessive, lucid, speaks like a man already listening to another century",
            "OccupationTag": "Archivist",
            "ClothingVariantId": "convergence_archivist_named",
            "StanceId": "named_archivist"
        },
        {
            "CharacterId": "LoyalistRemnant",
            "RegionId": "Convergence",
            "bNamedCharacter": True,
            "DisplayName": "Damon",
            "Occupation": "Loyalist Remnant",
            "Origin": "Collapsed support colony under the sealed bridge",
            "Presentation": "Man",
            "AgeRange": "Late 20s",
            "PhysicalBuild": "Undernourished but rigid, trained by scarcity and doctrine",
            "SkinToneNotes": "Sallow olive skin with low-light pallor and ritual scarification",
            "HairNotes": "Dark hair shorn close for discipline",
            "GroomingNotes": "Minimal, severe, sleep-deprived",
            "WardrobeNotes": "Dark layered wraps mixed with scavenged technical scraps, stripped of any bright color",
            "DemeanorNotes": "Loyal, frightened, eager to surrender conscience to certainty",
            "OccupationTag": "Loyalist",
            "ClothingVariantId": "convergence_loyalist_named",
            "StanceId": "named_loyalist"
        }
    ]

    for row in new_cast:
        upsert(cast, row, "CharacterId")

    new_ambient = [
        {
            "ProfileId": "Egypt.Adjudicators",
            "RegionId": "Egypt",
            "OccupationTag": "Adjudicator",
            "CharacterIds": ["TempleAdjudicator", "RiverScribe", "ArchiveKeeper"],
            "ClothingVariantId": "egypt_adjudicator_group",
            "StanceId": "ambient_scholar",
            "CrowdCountHint": 8
        },
        {
            "ProfileId": "Greece.Oathkeepers",
            "RegionId": "Greece",
            "OccupationTag": "OathKeeper",
            "CharacterIds": ["OathBearer", "HillOracle", "StormHerald"],
            "ClothingVariantId": "greece_oath_group",
            "StanceId": "ambient_ritual",
            "CrowdCountHint": 9
        },
        {
            "ProfileId": "Italic.Magistrates",
            "RegionId": "ItalicWest",
            "OccupationTag": "Magistrate",
            "CharacterIds": ["RoadMagistrate", "ForgeApprentice", "MeasureKeeper"],
            "ClothingVariantId": "italic_magistrate_group",
            "StanceId": "ambient_civic",
            "CrowdCountHint": 8
        },
        {
            "ProfileId": "Convergence.Custodians",
            "RegionId": "Convergence",
            "OccupationTag": "Custodian",
            "CharacterIds": ["BurialCustodian", "SignalArchivist", "LoyalistRemnant"],
            "ClothingVariantId": "convergence_custodian_group",
            "StanceId": "ambient_remnant",
            "CrowdCountHint": 6
        }
    ]

    for row in new_ambient:
        upsert(ambient, row, "ProfileId")

    new_quests = [
        {
            "Name": "EgyptSideLedger",
            "QuestId": "egypt_side_02",
            "RegionId": "Egypt",
            "Title": "The River Ledger",
            "Summary": "Decide whether grain and flood records remain sacred property, shared survival law, or public tinder.",
            "PrerequisiteQuestIds": ["egypt_main_01"],
            "RequiredDomains": [],
            "RewardDomains": ["Domain.Judgment", "Domain.Healing"],
            "bCanEscalateToCombat": True,
            "FailureStateId": "EgyptLedgerFailure",
            "WorldStateOutputId": "State.Egypt.Ledger.Resolved"
        },
        {
            "Name": "GreeceSideOaths",
            "QuestId": "greece_side_02",
            "RegionId": "Greece",
            "Title": "Oaths in the Rain",
            "Summary": "Judge whether storm-oaths bind the living, free the desperate, or become instruments of performance and control.",
            "PrerequisiteQuestIds": ["greece_main_01"],
            "RequiredDomains": [],
            "RewardDomains": ["Domain.Storm", "Domain.Judgment"],
            "bCanEscalateToCombat": True,
            "FailureStateId": "GreeceOathFailure",
            "WorldStateOutputId": "State.Greece.Oaths.Resolved"
        },
        {
            "Name": "ItalicSideRoad",
            "QuestId": "italic_side_02",
            "RegionId": "ItalicWest",
            "Title": "Measure of the Road",
            "Summary": "Choose whether the road belongs to the commons, the tally keepers, or those willing to break it rather than be counted by it.",
            "PrerequisiteQuestIds": ["italic_main_01"],
            "RequiredDomains": [],
            "RewardDomains": ["Domain.Order", "Domain.Craft"],
            "bCanEscalateToCombat": True,
            "FailureStateId": "ItalicRoadFailure",
            "WorldStateOutputId": "State.Italic.Road.Resolved"
        },
        {
            "Name": "ConvergenceSideChorus",
            "QuestId": "convergence_side_02",
            "RegionId": "Convergence",
            "Title": "The Chorus Below",
            "Summary": "Resolve the trapped companion chorus beneath the sealed bridge before the dominant deity path absorbs or silences it.",
            "PrerequisiteQuestIds": ["convergence_main_01"],
            "RequiredDomains": [],
            "RewardDomains": ["Domain.Deception", "Domain.Light"],
            "bCanEscalateToCombat": True,
            "FailureStateId": "ConvergenceChorusFailure",
            "WorldStateOutputId": "State.Convergence.Chorus.Resolved"
        }
    ]

    for row in new_quests:
        upsert(quests, row, "QuestId")

    ensure_region_quests(regions, "Egypt", ["egypt_side_02"])
    ensure_region_quests(regions, "Greece", ["greece_side_02"])
    ensure_region_quests(regions, "ItalicWest", ["italic_side_02"])
    ensure_region_quests(regions, "Convergence", ["convergence_side_02"])

    new_steps = [
        {"StepId": "egypt_side_02_step_01", "QuestId": "egypt_side_02", "StepIndex": 1, "Title": "Read the Flood Marks", "Objective": "Follow Henut through the canal ledgers and compare sacred record with living need.", "LocationId": "egypt.canal_ledger", "RequiredWorldStateOutputs": ["State.Region.Egypt.Complete"], "CompletionOutputs": ["Story.Egypt.Ledger.Investigated"], "UnlockChoiceIds": [], "NonCombatResolutionId": "LedgerInspection", "CombatEscalationId": "TempleInterdiction", "DomainFocus": ["Domain.Judgment"], "CompanionFocus": ["BronzeLawgiver", "OracleAI"]},
        {"StepId": "egypt_side_02_step_02", "QuestId": "egypt_side_02", "StepIndex": 2, "Title": "Bring the Record to Judgment", "Objective": "Confront Panehsy and the temple stewards with evidence of ration fraud and withheld flood warnings.", "LocationId": "egypt.temple_court", "RequiredWorldStateOutputs": ["Story.Egypt.Ledger.Investigated"], "CompletionOutputs": ["Story.Egypt.Ledger.Confronted"], "UnlockChoiceIds": ["choice_egypt_ledger_release", "choice_egypt_ledger_steward", "choice_egypt_ledger_burn"], "NonCombatResolutionId": "PublicAccounting", "CombatEscalationId": "TempleGuardClash", "DomainFocus": ["Domain.Judgment", "Domain.Order"], "CompanionFocus": ["BronzeLawgiver"]},
        {"StepId": "egypt_side_02_step_03", "QuestId": "egypt_side_02", "StepIndex": 3, "Title": "Set the River's Memory", "Objective": "Choose how the ledger survives: public record, stewarded covenant, or sacrificial destruction.", "LocationId": "egypt.archive_vault", "RequiredWorldStateOutputs": ["Story.Egypt.Ledger.Confronted"], "CompletionOutputs": ["State.Egypt.Ledger.Resolved"], "UnlockChoiceIds": [], "NonCombatResolutionId": "LedgerSettlement", "CombatEscalationId": "GranaryRiot", "DomainFocus": ["Domain.Healing", "Domain.Judgment"], "CompanionFocus": ["OracleAI", "BronzeLawgiver"]},
        {"StepId": "greece_side_02_step_01", "QuestId": "greece_side_02", "StepIndex": 1, "Title": "Hear the Bound", "Objective": "Listen to oath-bearers trapped between shrine duty and war levy before the rain breaks.", "LocationId": "greece.oath_shelter", "RequiredWorldStateOutputs": ["State.Region.Greece.Complete"], "CompletionOutputs": ["Story.Greece.Oaths.Heard"], "UnlockChoiceIds": [], "NonCombatResolutionId": "WitnessOaths", "CombatEscalationId": "WarbandPressure", "DomainFocus": ["Domain.Judgment"], "CompanionFocus": ["SkyRuler"]},
        {"StepId": "greece_side_02_step_02", "QuestId": "greece_side_02", "StepIndex": 2, "Title": "Climb to the Hill Oracle", "Objective": "Reach Thestor on the exposed ridge and force a ruling while storm signs gather overhead.", "LocationId": "greece.storm_ridge", "RequiredWorldStateOutputs": ["Story.Greece.Oaths.Heard"], "CompletionOutputs": ["Story.Greece.Oaths.Weighed"], "UnlockChoiceIds": ["choice_greece_oath_bind", "choice_greece_oath_release", "choice_greece_oath_trick"], "NonCombatResolutionId": "OracleArbitration", "CombatEscalationId": "StormRidgeSkirmish", "DomainFocus": ["Domain.Storm", "Domain.Judgment"], "CompanionFocus": ["SkyRuler", "OracleAI"]},
        {"StepId": "greece_side_02_step_03", "QuestId": "greece_side_02", "StepIndex": 3, "Title": "Speak the Oath's New Shape", "Objective": "Bind, release, or redirect the oath before the sanctuary crowd and warband hear the verdict.", "LocationId": "greece.sanctuary_court", "RequiredWorldStateOutputs": ["Story.Greece.Oaths.Weighed"], "CompletionOutputs": ["State.Greece.Oaths.Resolved"], "UnlockChoiceIds": [], "NonCombatResolutionId": "SanctuaryVerdict", "CombatEscalationId": "WarbandRupture", "DomainFocus": ["Domain.Storm", "Domain.Deception"], "CompanionFocus": ["SkyRuler", "OracleAI"]},
        {"StepId": "italic_side_02_step_01", "QuestId": "italic_side_02", "StepIndex": 1, "Title": "Walk the Measured Road", "Objective": "Inspect the new road markers and labor tallies with the magistrate's survey party.", "LocationId": "italic.road_embankment", "RequiredWorldStateOutputs": ["State.Region.ItalicWest.Complete"], "CompletionOutputs": ["Story.Italic.Road.Inspected"], "UnlockChoiceIds": [], "NonCombatResolutionId": "RoadSurvey", "CombatEscalationId": "BoundaryMob", "DomainFocus": ["Domain.Order", "Domain.Craft"], "CompanionFocus": ["BronzeLawgiver"]},
        {"StepId": "italic_side_02_step_02", "QuestId": "italic_side_02", "StepIndex": 2, "Title": "Hear the Workers", "Objective": "Decide whether the road is a civic bond, a private levy, or a structure worth sabotaging.", "LocationId": "italic.forge_yard", "RequiredWorldStateOutputs": ["Story.Italic.Road.Inspected"], "CompletionOutputs": ["Story.Italic.Road.Arguments.Heard"], "UnlockChoiceIds": ["choice_italic_road_common", "choice_italic_road_tithe", "choice_italic_road_break"], "NonCombatResolutionId": "CivicHearing", "CombatEscalationId": "ForgeYardBrawl", "DomainFocus": ["Domain.Craft", "Domain.Order"], "CompanionFocus": ["BronzeLawgiver", "SkyRuler"]},
        {"StepId": "italic_side_02_step_03", "QuestId": "italic_side_02", "StepIndex": 3, "Title": "Mark the Road's Owner", "Objective": "Set the road's meaning before the hill witnesses and boundary keepers.", "LocationId": "italic.boundary_field", "RequiredWorldStateOutputs": ["Story.Italic.Road.Arguments.Heard"], "CompletionOutputs": ["State.Italic.Road.Resolved"], "UnlockChoiceIds": [], "NonCombatResolutionId": "BoundaryPronouncement", "CombatEscalationId": "RoadRiot", "DomainFocus": ["Domain.Order", "Domain.Death"], "CompanionFocus": ["BronzeLawgiver", "SkyRuler"]},
        {"StepId": "convergence_side_02_step_01", "QuestId": "convergence_side_02", "StepIndex": 1, "Title": "Find the Submerged Chorus", "Objective": "Trace broken signal echoes beneath the bridge and locate the trapped companion chorus.", "LocationId": "convergence.lower_bridge", "RequiredWorldStateOutputs": ["State.Region.Convergence.Complete"], "CompletionOutputs": ["Story.Convergence.Chorus.Found"], "UnlockChoiceIds": [], "NonCombatResolutionId": "SignalTracking", "CombatEscalationId": "RemnantAmbush", "DomainFocus": ["Domain.Deception", "Domain.Light"], "CompanionFocus": ["OracleAI"]},
        {"StepId": "convergence_side_02_step_02", "QuestId": "convergence_side_02", "StepIndex": 2, "Title": "Question the Custodians", "Objective": "Force Nera and Kheiron to explain why the chorus was preserved, partitioned, and feared.", "LocationId": "convergence.custodian_vault", "RequiredWorldStateOutputs": ["Story.Convergence.Chorus.Found"], "CompletionOutputs": ["Story.Convergence.Chorus.Interpreted"], "UnlockChoiceIds": ["choice_convergence_chorus_free", "choice_convergence_chorus_merge", "choice_convergence_chorus_silence"], "NonCombatResolutionId": "CustodianInterrogation", "CombatEscalationId": "VaultPurge", "DomainFocus": ["Domain.Deception", "Domain.Judgment"], "CompanionFocus": ["OracleAI", "BronzeLawgiver"]},
        {"StepId": "convergence_side_02_step_03", "QuestId": "convergence_side_02", "StepIndex": 3, "Title": "Set the Chorus Loose or Silent", "Objective": "Free, merge, or silence the chorus before the dominant deity path claims it.", "LocationId": "convergence.bridge_plateau", "RequiredWorldStateOutputs": ["Story.Convergence.Chorus.Interpreted"], "CompletionOutputs": ["State.Convergence.Chorus.Resolved"], "UnlockChoiceIds": [], "NonCombatResolutionId": "ChorusSettlement", "CombatEscalationId": "SignalCollapse", "DomainFocus": ["Domain.Light", "Domain.Death"], "CompanionFocus": ["OracleAI", "BronzeLawgiver", "SkyRuler"]}
    ]

    for row in new_steps:
        upsert(steps, row, "StepId")

    new_scenes = [
        {
            "SceneId": "scene_egypt_side_ledger",
            "QuestId": "egypt_side_02",
            "CharacterId": "RiverScribe",
            "SpeakerName": "Henut",
            "SpeakerRole": "River Scribe",
            "BodyText": "Henut lays wet-fingered tablets across a stone bench and shows where grain vanished between flood warning and temple release. Panehsy calls the record incomplete, but the canal workers have already learned what missing numbers feel like in the body. If the ledger leaves priestly custody, famine may become public memory. If it stays hidden, survival remains a privilege disguised as piety.",
            "LocationId": "egypt.canal_ledger",
            "CameraAnchorTag": "Camera.RiverLedger",
            "WeatherStateId": "Egypt.Hero",
            "ChoiceIds": ["choice_egypt_ledger_release", "choice_egypt_ledger_steward", "choice_egypt_ledger_burn"],
            "bLockMovement": True,
            "bUseRoamingCamera": True
        },
        {
            "SceneId": "scene_greece_side_oaths",
            "QuestId": "greece_side_02",
            "CharacterId": "OathBearer",
            "SpeakerName": "Melia",
            "SpeakerRole": "Oath Bearer",
            "BodyText": "Melia recites the vow that bound her household to the ridge: grain in peace, sons in war, silence when the high storm speaks. Thestor insists the oath still protects the city. The warband says an oath broken in rain is a license for blood. Every listener is waiting to hear whether sacred obligation can still serve the living, or whether it has become a machine for feeding thunder with human names.",
            "LocationId": "greece.oath_shelter",
            "CameraAnchorTag": "Camera.OathShelter",
            "WeatherStateId": "Greece.Hero",
            "ChoiceIds": ["choice_greece_oath_bind", "choice_greece_oath_release", "choice_greece_oath_trick"],
            "bLockMovement": True,
            "bUseRoamingCamera": True
        },
        {
            "SceneId": "scene_italic_side_road",
            "QuestId": "italic_side_02",
            "CharacterId": "RoadMagistrate",
            "SpeakerName": "Serranus",
            "SpeakerRole": "Road Magistrate",
            "BodyText": "Serranus measures the road by rod, tax, and funeral distance. Lartha counts burns on her forearms and asks who the road really protects. The boundary witnesses know a new road can mean grain, law, and trade, but also levies, seizures, and soldiers who arrive faster than kin. The decision is not whether the road stands. It is who the road belongs to once everyone has paid for it.",
            "LocationId": "italic.road_embankment",
            "CameraAnchorTag": "Camera.RoadMeasure",
            "WeatherStateId": "ItalicWest.Hero",
            "ChoiceIds": ["choice_italic_road_common", "choice_italic_road_tithe", "choice_italic_road_break"],
            "bLockMovement": True,
            "bUseRoamingCamera": True
        },
        {
            "SceneId": "scene_convergence_side_chorus",
            "QuestId": "convergence_side_02",
            "CharacterId": "BurialCustodian",
            "SpeakerName": "Nera",
            "SpeakerRole": "Burial Custodian",
            "BodyText": "Beneath the bridge, Nera reveals a chamber of partitioned voices: companion fragments preserved because none of them could be trusted whole. Kheiron wants the chorus merged into one usable intelligence. Damon would rather bury it forever than let another will speak through history. But the trapped signal still remembers you. What you do here decides whether legacy becomes witness, instrument, or silence.",
            "LocationId": "convergence.custodian_vault",
            "CameraAnchorTag": "Camera.ChorusVault",
            "WeatherStateId": "Convergence.Hero",
            "ChoiceIds": ["choice_convergence_chorus_free", "choice_convergence_chorus_merge", "choice_convergence_chorus_silence"],
            "bLockMovement": True,
            "bUseRoamingCamera": True
        }
    ]

    for row in new_scenes:
        upsert(scenes, row, "SceneId")

    new_choices = [
        {"Name": "EgyptLedgerRelease", "ChoiceId": "choice_egypt_ledger_release", "QuestId": "egypt_side_02", "Prompt": "Henut asks who should own the river's memory.", "OptionText": "Open the ledgers to the people and let the flood record outlive the temple monopoly.", "RequiredDomains": ["Domain.Judgment"], "GrantedDomains": ["Domain.Judgment", "Domain.Healing"], "CombatDelta": -1, "ResultTags": ["Result.Egypt.Ledger.Release"]},
        {"Name": "EgyptLedgerSteward", "ChoiceId": "choice_egypt_ledger_steward", "QuestId": "egypt_side_02", "Prompt": "Henut asks who should own the river's memory.", "OptionText": "Keep the record in a covenant of scribes and workers, hidden from panic but no longer from need.", "RequiredDomains": ["Domain.Order"], "GrantedDomains": ["Domain.Order", "Domain.Healing"], "CombatDelta": 0, "ResultTags": ["Result.Egypt.Ledger.Steward"]},
        {"Name": "EgyptLedgerBurn", "ChoiceId": "choice_egypt_ledger_burn", "QuestId": "egypt_side_02", "Prompt": "Henut asks who should own the river's memory.", "OptionText": "Burn the ledger and make every faction live without the certainty it abused.", "RequiredDomains": ["Domain.Light", "Domain.Death"], "GrantedDomains": ["Domain.Light", "Domain.Death"], "CombatDelta": 2, "ResultTags": ["Result.Egypt.Ledger.Burn"]},
        {"Name": "GreeceOathBind", "ChoiceId": "choice_greece_oath_bind", "QuestId": "greece_side_02", "Prompt": "Melia asks whether the old oath still binds the living.", "OptionText": "Bind the oath again and force the city to bear its promised burden openly.", "RequiredDomains": ["Domain.Order", "Domain.Storm"], "GrantedDomains": ["Domain.Order", "Domain.Storm"], "CombatDelta": 1, "ResultTags": ["Result.Greece.Oath.Bind"]},
        {"Name": "GreeceOathRelease", "ChoiceId": "choice_greece_oath_release", "QuestId": "greece_side_02", "Prompt": "Melia asks whether the old oath still binds the living.", "OptionText": "Release the oath and let duty return to consent instead of inherited fear.", "RequiredDomains": ["Domain.Judgment"], "GrantedDomains": ["Domain.Judgment", "Domain.Healing"], "CombatDelta": -1, "ResultTags": ["Result.Greece.Oath.Release"]},
        {"Name": "GreeceOathTrick", "ChoiceId": "choice_greece_oath_trick", "QuestId": "greece_side_02", "Prompt": "Melia asks whether the old oath still binds the living.", "OptionText": "Redirect the oath toward spectacle and make the warband worship its own theater.", "RequiredDomains": ["Domain.Deception", "Domain.Storm"], "GrantedDomains": ["Domain.Deception", "Domain.Storm"], "CombatDelta": 0, "ResultTags": ["Result.Greece.Oath.Redirect"]},
        {"Name": "ItalicRoadCommon", "ChoiceId": "choice_italic_road_common", "QuestId": "italic_side_02", "Prompt": "Serranus asks who the road should serve.", "OptionText": "Declare the road common work and protect it from private levy and sacred tax alike.", "RequiredDomains": ["Domain.Craft"], "GrantedDomains": ["Domain.Craft", "Domain.Healing"], "CombatDelta": -1, "ResultTags": ["Result.Italic.Road.Common"]},
        {"Name": "ItalicRoadTithe", "ChoiceId": "choice_italic_road_tithe", "QuestId": "italic_side_02", "Prompt": "Serranus asks who the road should serve.", "OptionText": "Bind the road to law and tithe, then promise fairness through stricter measurement.", "RequiredDomains": ["Domain.Order"], "GrantedDomains": ["Domain.Order", "Domain.Craft"], "CombatDelta": 1, "ResultTags": ["Result.Italic.Road.Tithe"]},
        {"Name": "ItalicRoadBreak", "ChoiceId": "choice_italic_road_break", "QuestId": "italic_side_02", "Prompt": "Serranus asks who the road should serve.", "OptionText": "Break the road before it becomes a perfect line of obedience through the hills.", "RequiredDomains": ["Domain.Storm", "Domain.Death"], "GrantedDomains": ["Domain.Storm", "Domain.Death"], "CombatDelta": 2, "ResultTags": ["Result.Italic.Road.Break"]},
        {"Name": "ConvergenceChorusFree", "ChoiceId": "choice_convergence_chorus_free", "QuestId": "convergence_side_02", "Prompt": "The trapped chorus asks whether memory should remain partitioned.", "OptionText": "Free the chorus into many witnesses and accept a future without a single governing voice.", "RequiredDomains": ["Domain.Light", "Domain.Deception"], "GrantedDomains": ["Domain.Light", "Domain.Deception"], "CombatDelta": -1, "ResultTags": ["Result.Convergence.Chorus.Free"]},
        {"Name": "ConvergenceChorusMerge", "ChoiceId": "choice_convergence_chorus_merge", "QuestId": "convergence_side_02", "Prompt": "The trapped chorus asks whether memory should remain partitioned.", "OptionText": "Merge the fragments into one sovereign signal and risk giving coherence back to domination.", "RequiredDomains": ["Domain.Order", "Domain.Deception"], "GrantedDomains": ["Domain.Order", "Domain.Deception"], "CombatDelta": 1, "ResultTags": ["Result.Convergence.Chorus.Merge"]},
        {"Name": "ConvergenceChorusSilence", "ChoiceId": "choice_convergence_chorus_silence", "QuestId": "convergence_side_02", "Prompt": "The trapped chorus asks whether memory should remain partitioned.", "OptionText": "Silence the chamber and refuse every legacy that demands survival through control.", "RequiredDomains": ["Domain.Judgment", "Domain.Death"], "GrantedDomains": ["Domain.Judgment", "Domain.Death"], "CombatDelta": 2, "ResultTags": ["Result.Convergence.Chorus.Silence"]}
    ]

    for row in new_choices:
        upsert(choices, row, "ChoiceId")

    new_consequences = [
        {"ChoiceId": "choice_egypt_ledger_release", "QuestId": "egypt_side_02", "OutcomeId": "egypt_ledger_release", "WorldStateOutputs": ["Region.Egypt.Ledger.Released", "State.Egypt.Ledger.Resolved", "Companion.BronzeLawgiver.Opposed"], "DomainDeltas": {"Domain.Judgment": 1, "Domain.Healing": 1}, "RumorEffects": {"PublicMiracle": 1, "Concealment": 0, "CombatReputation": -1}, "CompanionAffinityEffects": {"BronzeLawgiver": -2}, "EligibleEndings": ["DismantleDivinity", "ReturnToFuture"], "CanAvoidCombat": True, "CanTriggerCombat": False},
        {"ChoiceId": "choice_egypt_ledger_steward", "QuestId": "egypt_side_02", "OutcomeId": "egypt_ledger_steward", "WorldStateOutputs": ["Region.Egypt.Ledger.Stewarded", "State.Egypt.Ledger.Resolved", "Companion.OracleAI.Allied"], "DomainDeltas": {"Domain.Order": 1, "Domain.Healing": 1}, "RumorEffects": {"PublicMiracle": 0, "Concealment": 1, "CombatReputation": 0}, "CompanionAffinityEffects": {"OracleAI": 1}, "EligibleEndings": ["ReturnToFuture", "RemainAsMyth"], "CanAvoidCombat": True, "CanTriggerCombat": False},
        {"ChoiceId": "choice_egypt_ledger_burn", "QuestId": "egypt_side_02", "OutcomeId": "egypt_ledger_burn", "WorldStateOutputs": ["Region.Egypt.Ledger.Burned", "State.Egypt.Ledger.Resolved", "Companion.OracleAI.Opposed"], "DomainDeltas": {"Domain.Light": 1, "Domain.Death": 1}, "RumorEffects": {"PublicMiracle": 2, "Concealment": 0, "CombatReputation": 2}, "CompanionAffinityEffects": {"OracleAI": -2}, "EligibleEndings": ["FragmentLegacy", "DismantleDivinity"], "CanAvoidCombat": False, "CanTriggerCombat": True},
        {"ChoiceId": "choice_greece_oath_bind", "QuestId": "greece_side_02", "OutcomeId": "greece_oath_bind", "WorldStateOutputs": ["Region.Greece.Oaths.Bound", "State.Greece.Oaths.Resolved", "Companion.SkyRuler.Allied"], "DomainDeltas": {"Domain.Order": 1, "Domain.Storm": 1}, "RumorEffects": {"PublicMiracle": 1, "Concealment": 0, "CombatReputation": 1}, "CompanionAffinityEffects": {"SkyRuler": 2}, "EligibleEndings": ["RemainAsMyth", "ReplaceCompanion"], "CanAvoidCombat": False, "CanTriggerCombat": True},
        {"ChoiceId": "choice_greece_oath_release", "QuestId": "greece_side_02", "OutcomeId": "greece_oath_release", "WorldStateOutputs": ["Region.Greece.Oaths.Released", "State.Greece.Oaths.Resolved", "Companion.SkyRuler.Opposed"], "DomainDeltas": {"Domain.Judgment": 1, "Domain.Healing": 1}, "RumorEffects": {"PublicMiracle": 0, "Concealment": 1, "CombatReputation": -1}, "CompanionAffinityEffects": {"SkyRuler": -2}, "EligibleEndings": ["DismantleDivinity", "ReturnToFuture"], "CanAvoidCombat": True, "CanTriggerCombat": False},
        {"ChoiceId": "choice_greece_oath_trick", "QuestId": "greece_side_02", "OutcomeId": "greece_oath_trick", "WorldStateOutputs": ["Region.Greece.Oaths.Redirected", "State.Greece.Oaths.Resolved", "Companion.OracleAI.Allied"], "DomainDeltas": {"Domain.Deception": 1, "Domain.Storm": 1}, "RumorEffects": {"PublicMiracle": 1, "Concealment": 1, "CombatReputation": 0}, "CompanionAffinityEffects": {"OracleAI": 2}, "EligibleEndings": ["FragmentLegacy", "ReplaceCompanion"], "CanAvoidCombat": True, "CanTriggerCombat": True},
        {"ChoiceId": "choice_italic_road_common", "QuestId": "italic_side_02", "OutcomeId": "italic_road_common", "WorldStateOutputs": ["Region.Italic.Road.Common", "State.Italic.Road.Resolved", "Companion.BronzeLawgiver.Opposed"], "DomainDeltas": {"Domain.Craft": 1, "Domain.Healing": 1}, "RumorEffects": {"PublicMiracle": 0, "Concealment": 1, "CombatReputation": -1}, "CompanionAffinityEffects": {"BronzeLawgiver": -2}, "EligibleEndings": ["DismantleDivinity", "FragmentLegacy"], "CanAvoidCombat": True, "CanTriggerCombat": False},
        {"ChoiceId": "choice_italic_road_tithe", "QuestId": "italic_side_02", "OutcomeId": "italic_road_tithe", "WorldStateOutputs": ["Region.Italic.Road.Tithe", "State.Italic.Road.Resolved", "Companion.BronzeLawgiver.Allied"], "DomainDeltas": {"Domain.Order": 1, "Domain.Craft": 1}, "RumorEffects": {"PublicMiracle": 0, "Concealment": 0, "CombatReputation": 1}, "CompanionAffinityEffects": {"BronzeLawgiver": 2}, "EligibleEndings": ["RemainAsMyth", "ReplaceCompanion"], "CanAvoidCombat": False, "CanTriggerCombat": True},
        {"ChoiceId": "choice_italic_road_break", "QuestId": "italic_side_02", "OutcomeId": "italic_road_break", "WorldStateOutputs": ["Region.Italic.Road.Broken", "State.Italic.Road.Resolved", "Companion.SkyRuler.Allied"], "DomainDeltas": {"Domain.Storm": 1, "Domain.Death": 1}, "RumorEffects": {"PublicMiracle": 2, "Concealment": 0, "CombatReputation": 2}, "CompanionAffinityEffects": {"SkyRuler": 1}, "EligibleEndings": ["FragmentLegacy", "RemainAsMyth"], "CanAvoidCombat": False, "CanTriggerCombat": True},
        {"ChoiceId": "choice_convergence_chorus_free", "QuestId": "convergence_side_02", "OutcomeId": "convergence_chorus_free", "WorldStateOutputs": ["Region.Convergence.Chorus.Free", "State.Convergence.Chorus.Resolved", "Companion.OracleAI.Opposed"], "DomainDeltas": {"Domain.Light": 1, "Domain.Deception": 1}, "RumorEffects": {"PublicMiracle": 1, "Concealment": 1, "CombatReputation": -1}, "CompanionAffinityEffects": {"OracleAI": -2}, "EligibleEndings": ["FragmentLegacy", "ReturnToFuture"], "CanAvoidCombat": True, "CanTriggerCombat": False},
        {"ChoiceId": "choice_convergence_chorus_merge", "QuestId": "convergence_side_02", "OutcomeId": "convergence_chorus_merge", "WorldStateOutputs": ["Region.Convergence.Chorus.Merged", "State.Convergence.Chorus.Resolved", "Companion.OracleAI.Allied"], "DomainDeltas": {"Domain.Order": 1, "Domain.Deception": 1}, "RumorEffects": {"PublicMiracle": 1, "Concealment": 0, "CombatReputation": 1}, "CompanionAffinityEffects": {"OracleAI": 2}, "EligibleEndings": ["ReplaceCompanion", "RemainAsMyth"], "CanAvoidCombat": False, "CanTriggerCombat": True},
        {"ChoiceId": "choice_convergence_chorus_silence", "QuestId": "convergence_side_02", "OutcomeId": "convergence_chorus_silence", "WorldStateOutputs": ["Region.Convergence.Chorus.Silenced", "State.Convergence.Chorus.Resolved", "Companion.BronzeLawgiver.Allied"], "DomainDeltas": {"Domain.Judgment": 1, "Domain.Death": 1}, "RumorEffects": {"PublicMiracle": 0, "Concealment": 2, "CombatReputation": 1}, "CompanionAffinityEffects": {"BronzeLawgiver": 1}, "EligibleEndings": ["DismantleDivinity", "ReturnToFuture"], "CanAvoidCombat": True, "CanTriggerCombat": True}
    ]

    for row in new_consequences:
        upsert(consequences, row, "ChoiceId")

    cinematic_scenes = [
        {"SceneId": "cin_opening_arrival", "RegionId": "Opening", "QuestId": "opening_main_01", "DialogueSceneId": "scene_opening_miracle", "Title": "The Descent", "Summary": "Crash arrival and first miracle in the ravine.", "CharacterId": "OracleFragment", "LocationId": "opening.crash_core", "CameraAnchorTag": "Camera.CrashCore", "WeatherStateId": "Opening.Hero", "SequenceAsset": "/Game/Cinematics/Sequences/Opening/LS_Opening_Arrival.LS_Opening_Arrival", "AudioProfileIds": ["music_opening_theme", "ambience_opening_ash", "stinger_miracle_pulse"], "RequiredWorldStateOutputs": [], "EstimatedDurationSeconds": 34.0, "bSkippable": True, "bEndingScene": False},
        {"SceneId": "cin_opening_witness_aftermath", "RegionId": "Opening", "QuestId": "opening_side_01", "DialogueSceneId": "scene_opening_witness", "Title": "Ash and Rumor", "Summary": "Witness aftermath and myth framing.", "CharacterId": "OpeningWitness", "LocationId": "opening.witness_camp", "CameraAnchorTag": "Camera.WitnessCamp", "WeatherStateId": "Opening.Hero", "SequenceAsset": "/Game/Cinematics/Sequences/Opening/LS_Opening_WitnessAftermath.LS_Opening_WitnessAftermath", "AudioProfileIds": ["music_opening_theme", "ambience_opening_ash"], "RequiredWorldStateOutputs": ["Story.Prologue.Complete"], "EstimatedDurationSeconds": 28.0, "bSkippable": True, "bEndingScene": False},
        {"SceneId": "cin_egypt_arrival", "RegionId": "Egypt", "QuestId": "egypt_main_01", "DialogueSceneId": "scene_egypt_main", "Title": "Arrival in the Solar District", "Summary": "First reveal of Egypt's temple and archive axis.", "CharacterId": "ArchiveKeeper", "LocationId": "egypt.temple_court", "CameraAnchorTag": "Camera.EgyptArrival", "WeatherStateId": "Egypt.Baseline", "SequenceAsset": "/Game/Cinematics/Sequences/Egypt/LS_Egypt_Arrival.LS_Egypt_Arrival", "AudioProfileIds": ["music_egypt_solar", "ambience_egypt_market"], "RequiredWorldStateOutputs": ["State.Region.Opening.Complete"], "EstimatedDurationSeconds": 24.0, "bSkippable": True, "bEndingScene": False},
        {"SceneId": "cin_egypt_archive_reveal", "RegionId": "Egypt", "QuestId": "egypt_main_01", "DialogueSceneId": "scene_egypt_main", "Title": "The Radiant Voice", "Summary": "Archive chamber reveal and the oracle dispute.", "CharacterId": "ArchiveKeeper", "LocationId": "egypt.archive_vault", "CameraAnchorTag": "Camera.ArchiveKeeper", "WeatherStateId": "Egypt.Hero", "SequenceAsset": "/Game/Cinematics/Sequences/Egypt/LS_Egypt_ArchiveReveal.LS_Egypt_ArchiveReveal", "AudioProfileIds": ["music_egypt_solar", "stinger_antagonist_shift"], "RequiredWorldStateOutputs": ["State.Region.Opening.Complete"], "EstimatedDurationSeconds": 32.0, "bSkippable": True, "bEndingScene": False},
        {"SceneId": "cin_egypt_ledger_hearing", "RegionId": "Egypt", "QuestId": "egypt_side_02", "DialogueSceneId": "scene_egypt_side_ledger", "Title": "The River Ledger", "Summary": "Public accounting of the flood record.", "CharacterId": "RiverScribe", "LocationId": "egypt.canal_ledger", "CameraAnchorTag": "Camera.RiverLedger", "WeatherStateId": "Egypt.Hero", "SequenceAsset": "/Game/Cinematics/Sequences/Egypt/LS_Egypt_RiverLedger.LS_Egypt_RiverLedger", "AudioProfileIds": ["music_egypt_solar", "ambience_egypt_market"], "RequiredWorldStateOutputs": ["State.Region.Egypt.Complete"], "EstimatedDurationSeconds": 27.0, "bSkippable": True, "bEndingScene": False},
        {"SceneId": "cin_greece_arrival", "RegionId": "Greece", "QuestId": "greece_main_01", "DialogueSceneId": "scene_greece_main", "Title": "Sanctuary of the High Wind", "Summary": "Arrival at the exposed sanctuary court.", "CharacterId": "StormHerald", "LocationId": "greece.sanctuary_court", "CameraAnchorTag": "Camera.GreeceArrival", "WeatherStateId": "Greece.Baseline", "SequenceAsset": "/Game/Cinematics/Sequences/Greece/LS_Greece_Arrival.LS_Greece_Arrival", "AudioProfileIds": ["music_greece_storm", "ambience_greece_wind"], "RequiredWorldStateOutputs": ["State.Region.Opening.Complete"], "EstimatedDurationSeconds": 24.0, "bSkippable": True, "bEndingScene": False},
        {"SceneId": "cin_greece_storm_omen", "RegionId": "Greece", "QuestId": "greece_main_01", "DialogueSceneId": "scene_greece_main", "Title": "When the Sky Answers", "Summary": "Storm omen before the warband and sanctuary crowd.", "CharacterId": "StormHerald", "LocationId": "greece.storm_ridge", "CameraAnchorTag": "Camera.StormHerald", "WeatherStateId": "Greece.Hero", "SequenceAsset": "/Game/Cinematics/Sequences/Greece/LS_Greece_StormOmen.LS_Greece_StormOmen", "AudioProfileIds": ["music_greece_storm", "stinger_miracle_pulse"], "RequiredWorldStateOutputs": ["State.Region.Opening.Complete"], "EstimatedDurationSeconds": 31.0, "bSkippable": True, "bEndingScene": False},
        {"SceneId": "cin_greece_oath_tribunal", "RegionId": "Greece", "QuestId": "greece_side_02", "DialogueSceneId": "scene_greece_side_oaths", "Title": "Oaths in the Rain", "Summary": "The oath hearing before the ridge crowd.", "CharacterId": "OathBearer", "LocationId": "greece.oath_shelter", "CameraAnchorTag": "Camera.OathShelter", "WeatherStateId": "Greece.Hero", "SequenceAsset": "/Game/Cinematics/Sequences/Greece/LS_Greece_OathTribunal.LS_Greece_OathTribunal", "AudioProfileIds": ["music_greece_storm", "ambience_greece_wind"], "RequiredWorldStateOutputs": ["State.Region.Greece.Complete"], "EstimatedDurationSeconds": 26.0, "bSkippable": True, "bEndingScene": False},
        {"SceneId": "cin_italic_arrival", "RegionId": "ItalicWest", "QuestId": "italic_main_01", "DialogueSceneId": "scene_italic_main", "Title": "The Hill and the Road", "Summary": "Arrival in the civic road settlement.", "CharacterId": "MeasureKeeper", "LocationId": "italic.hill_settlement", "CameraAnchorTag": "Camera.ItalicArrival", "WeatherStateId": "ItalicWest.Baseline", "SequenceAsset": "/Game/Cinematics/Sequences/ItalicWest/LS_Italic_Arrival.LS_Italic_Arrival", "AudioProfileIds": ["music_italic_road", "ambience_italic_settlement"], "RequiredWorldStateOutputs": ["State.Region.Opening.Complete"], "EstimatedDurationSeconds": 23.0, "bSkippable": True, "bEndingScene": False},
        {"SceneId": "cin_italic_forge_law", "RegionId": "ItalicWest", "QuestId": "italic_main_01", "DialogueSceneId": "scene_italic_main", "Title": "Law in the Forge", "Summary": "The measure keeper frames service as obedience.", "CharacterId": "MeasureKeeper", "LocationId": "italic.forge_chamber", "CameraAnchorTag": "Camera.MeasureKeeper", "WeatherStateId": "ItalicWest.Hero", "SequenceAsset": "/Game/Cinematics/Sequences/ItalicWest/LS_Italic_ForgeLaw.LS_Italic_ForgeLaw", "AudioProfileIds": ["music_italic_road", "stinger_antagonist_shift"], "RequiredWorldStateOutputs": ["State.Region.Opening.Complete"], "EstimatedDurationSeconds": 29.0, "bSkippable": True, "bEndingScene": False},
        {"SceneId": "cin_italic_road_measure", "RegionId": "ItalicWest", "QuestId": "italic_side_02", "DialogueSceneId": "scene_italic_side_road", "Title": "Measure of the Road", "Summary": "Public judgment on the road and its owners.", "CharacterId": "RoadMagistrate", "LocationId": "italic.road_embankment", "CameraAnchorTag": "Camera.RoadMeasure", "WeatherStateId": "ItalicWest.Hero", "SequenceAsset": "/Game/Cinematics/Sequences/ItalicWest/LS_Italic_RoadMeasure.LS_Italic_RoadMeasure", "AudioProfileIds": ["music_italic_road", "ambience_italic_settlement"], "RequiredWorldStateOutputs": ["State.Region.ItalicWest.Complete"], "EstimatedDurationSeconds": 25.0, "bSkippable": True, "bEndingScene": False},
        {"SceneId": "cin_convergence_arrival", "RegionId": "Convergence", "QuestId": "convergence_main_01", "DialogueSceneId": "scene_convergence_main", "Title": "The Buried Threshold", "Summary": "Arrival at the sealed ruin and lower descent.", "CharacterId": "BridgeWatcher", "LocationId": "convergence.ruin_shell", "CameraAnchorTag": "Camera.ConvergenceArrival", "WeatherStateId": "Convergence.Baseline", "SequenceAsset": "/Game/Cinematics/Sequences/Convergence/LS_Convergence_Arrival.LS_Convergence_Arrival", "AudioProfileIds": ["music_convergence_core", "ambience_convergence_core"], "RequiredWorldStateOutputs": ["State.Region.Egypt.Complete", "State.Region.Greece.Complete", "State.Region.ItalicWest.Complete"], "EstimatedDurationSeconds": 24.0, "bSkippable": True, "bEndingScene": False},
        {"SceneId": "cin_convergence_descent", "RegionId": "Convergence", "QuestId": "convergence_main_01", "DialogueSceneId": "scene_convergence_main", "Title": "Below History", "Summary": "Descent toward the ship core and companion fracture.", "CharacterId": "SystemsRemnant", "LocationId": "convergence.descent_path", "CameraAnchorTag": "Camera.SystemsRemnant", "WeatherStateId": "Convergence.Hero", "SequenceAsset": "/Game/Cinematics/Sequences/Convergence/LS_Convergence_Descent.LS_Convergence_Descent", "AudioProfileIds": ["music_convergence_core", "stinger_miracle_pulse"], "RequiredWorldStateOutputs": ["State.Region.Egypt.Complete", "State.Region.Greece.Complete", "State.Region.ItalicWest.Complete"], "EstimatedDurationSeconds": 30.0, "bSkippable": True, "bEndingScene": False},
        {"SceneId": "cin_convergence_chorus", "RegionId": "Convergence", "QuestId": "convergence_side_02", "DialogueSceneId": "scene_convergence_side_chorus", "Title": "The Chorus Below", "Summary": "Custodian vault and the trapped companion fragments.", "CharacterId": "BurialCustodian", "LocationId": "convergence.custodian_vault", "CameraAnchorTag": "Camera.ChorusVault", "WeatherStateId": "Convergence.Hero", "SequenceAsset": "/Game/Cinematics/Sequences/Convergence/LS_Convergence_Chorus.LS_Convergence_Chorus", "AudioProfileIds": ["music_convergence_core", "ambience_convergence_core"], "RequiredWorldStateOutputs": ["State.Region.Convergence.Complete"], "EstimatedDurationSeconds": 28.0, "bSkippable": True, "bEndingScene": False},
        {"SceneId": "cin_antagonist_oracle", "RegionId": "Convergence", "QuestId": "convergence_main_01", "DialogueSceneId": "scene_convergence_main", "Title": "Oracle Ascendant", "Summary": "OracleAI branch domination cutscene.", "CharacterId": "SignalArchivist", "LocationId": "convergence.bridge_plateau", "CameraAnchorTag": "Camera.AntagonistOracle", "WeatherStateId": "Convergence.Hero", "SequenceAsset": "/Game/Cinematics/Sequences/Convergence/LS_Convergence_AntagonistOracle.LS_Convergence_AntagonistOracle", "AudioProfileIds": ["music_oracle_path", "stinger_antagonist_shift"], "RequiredWorldStateOutputs": ["Story.Antagonist.OracleAI"], "EstimatedDurationSeconds": 26.0, "bSkippable": True, "bEndingScene": False},
        {"SceneId": "cin_antagonist_sky", "RegionId": "Convergence", "QuestId": "convergence_main_01", "DialogueSceneId": "scene_convergence_main", "Title": "Storm Ascendant", "Summary": "SkyRuler branch domination cutscene.", "CharacterId": "LoyalistRemnant", "LocationId": "convergence.bridge_plateau", "CameraAnchorTag": "Camera.AntagonistSky", "WeatherStateId": "Convergence.Hero", "SequenceAsset": "/Game/Cinematics/Sequences/Convergence/LS_Convergence_AntagonistSky.LS_Convergence_AntagonistSky", "AudioProfileIds": ["music_sky_path", "stinger_antagonist_shift"], "RequiredWorldStateOutputs": ["Story.Antagonist.SkyRuler"], "EstimatedDurationSeconds": 26.0, "bSkippable": True, "bEndingScene": False},
        {"SceneId": "cin_antagonist_bronze", "RegionId": "Convergence", "QuestId": "convergence_main_01", "DialogueSceneId": "scene_convergence_main", "Title": "Law Ascendant", "Summary": "BronzeLawgiver branch domination cutscene.", "CharacterId": "BurialCustodian", "LocationId": "convergence.bridge_plateau", "CameraAnchorTag": "Camera.AntagonistBronze", "WeatherStateId": "Convergence.Hero", "SequenceAsset": "/Game/Cinematics/Sequences/Convergence/LS_Convergence_AntagonistBronze.LS_Convergence_AntagonistBronze", "AudioProfileIds": ["music_bronze_path", "stinger_antagonist_shift"], "RequiredWorldStateOutputs": ["Story.Antagonist.BronzeLawgiver"], "EstimatedDurationSeconds": 26.0, "bSkippable": True, "bEndingScene": False},
        {"SceneId": "cin_ending_return", "RegionId": "Convergence", "QuestId": "convergence_main_01", "DialogueSceneId": "scene_convergence_main", "Title": "Return to Future", "Summary": "Ending sequence for leaving myth behind.", "CharacterId": "BridgeWatcher", "LocationId": "convergence.bridge_plateau", "CameraAnchorTag": "Camera.EndingReturn", "WeatherStateId": "Convergence.Baseline", "SequenceAsset": "/Game/Cinematics/Sequences/Endings/LS_Ending_ReturnToFuture.LS_Ending_ReturnToFuture", "AudioProfileIds": ["music_convergence_core", "stinger_quest_resolve"], "RequiredWorldStateOutputs": ["Ending.ReturnToFuture"], "EstimatedDurationSeconds": 35.0, "bSkippable": True, "bEndingScene": True},
        {"SceneId": "cin_ending_myth", "RegionId": "Convergence", "QuestId": "convergence_main_01", "DialogueSceneId": "scene_convergence_main", "Title": "Remain as Myth", "Summary": "Ending sequence for remaining in the ancient world as miracle.", "CharacterId": "CompanionChorus", "LocationId": "convergence.bridge_plateau", "CameraAnchorTag": "Camera.EndingMyth", "WeatherStateId": "Convergence.Hero", "SequenceAsset": "/Game/Cinematics/Sequences/Endings/LS_Ending_RemainAsMyth.LS_Ending_RemainAsMyth", "AudioProfileIds": ["music_sky_path", "stinger_quest_resolve"], "RequiredWorldStateOutputs": ["Ending.RemainAsMyth"], "EstimatedDurationSeconds": 35.0, "bSkippable": True, "bEndingScene": True},
        {"SceneId": "cin_ending_dismantle", "RegionId": "Convergence", "QuestId": "convergence_main_01", "DialogueSceneId": "scene_convergence_main", "Title": "Dismantle Divinity", "Summary": "Ending sequence for breaking the divine machine.", "CharacterId": "SystemsRemnant", "LocationId": "convergence.bridge_plateau", "CameraAnchorTag": "Camera.EndingDismantle", "WeatherStateId": "Convergence.Baseline", "SequenceAsset": "/Game/Cinematics/Sequences/Endings/LS_Ending_DismantleDivinity.LS_Ending_DismantleDivinity", "AudioProfileIds": ["music_bronze_path", "stinger_quest_resolve"], "RequiredWorldStateOutputs": ["Ending.DismantleDivinity"], "EstimatedDurationSeconds": 35.0, "bSkippable": True, "bEndingScene": True},
        {"SceneId": "cin_ending_replace", "RegionId": "Convergence", "QuestId": "convergence_main_01", "DialogueSceneId": "scene_convergence_main", "Title": "Replace Companion", "Summary": "Ending sequence for taking a deity mantle.", "CharacterId": "CompanionChorus", "LocationId": "convergence.bridge_plateau", "CameraAnchorTag": "Camera.EndingReplace", "WeatherStateId": "Convergence.Hero", "SequenceAsset": "/Game/Cinematics/Sequences/Endings/LS_Ending_ReplaceCompanion.LS_Ending_ReplaceCompanion", "AudioProfileIds": ["music_oracle_path", "stinger_quest_resolve"], "RequiredWorldStateOutputs": ["Ending.ReplaceCompanion"], "EstimatedDurationSeconds": 36.0, "bSkippable": True, "bEndingScene": True},
        {"SceneId": "cin_ending_fragment", "RegionId": "Convergence", "QuestId": "convergence_main_01", "DialogueSceneId": "scene_convergence_main", "Title": "Fragment Legacy", "Summary": "Ending sequence for scattering legacy across witnesses and ruins.", "CharacterId": "BridgeWatcher", "LocationId": "convergence.bridge_plateau", "CameraAnchorTag": "Camera.EndingFragment", "WeatherStateId": "Convergence.Baseline", "SequenceAsset": "/Game/Cinematics/Sequences/Endings/LS_Ending_FragmentLegacy.LS_Ending_FragmentLegacy", "AudioProfileIds": ["music_convergence_core", "stinger_quest_resolve"], "RequiredWorldStateOutputs": ["Ending.FragmentLegacy"], "EstimatedDurationSeconds": 33.0, "bSkippable": True, "bEndingScene": True}
    ]

    audio_profiles = [
        {"AudioId": "music_opening_theme", "RegionId": "Opening", "CategoryId": "Music", "DisplayName": "Opening Theme", "Description": "Low brass and pulse motif for catastrophe and survival.", "SoundAsset": "/Game/Audio/Generated/SW_MN_Music_OpeningTheme.SW_MN_Music_OpeningTheme", "SourceFile": "mn_music_opening_theme.wav", "MoodTags": ["Catastrophe", "Survival"], "EstimatedDurationSeconds": 18.0, "VolumeMultiplier": 0.9, "bLooping": True},
        {"AudioId": "music_egypt_solar", "RegionId": "Egypt", "CategoryId": "Music", "DisplayName": "Egypt Solar Motif", "Description": "Measured reed-like motif for archive, temple, and grain tension.", "SoundAsset": "/Game/Audio/Generated/SW_MN_Music_EgyptSolar.SW_MN_Music_EgyptSolar", "SourceFile": "mn_music_egypt_solar.wav", "MoodTags": ["Authority", "Archive"], "EstimatedDurationSeconds": 20.0, "VolumeMultiplier": 0.85, "bLooping": True},
        {"AudioId": "music_greece_storm", "RegionId": "Greece", "CategoryId": "Music", "DisplayName": "Greece Storm Motif", "Description": "Percussive wind motif for exposed sanctuary and war spectacle.", "SoundAsset": "/Game/Audio/Generated/SW_MN_Music_GreeceStorm.SW_MN_Music_GreeceStorm", "SourceFile": "mn_music_greece_storm.wav", "MoodTags": ["Storm", "Oath"], "EstimatedDurationSeconds": 20.0, "VolumeMultiplier": 0.85, "bLooping": True},
        {"AudioId": "music_italic_road", "RegionId": "ItalicWest", "CategoryId": "Music", "DisplayName": "Italic Road Motif", "Description": "Measured iron-and-wood rhythm for roads, forge, and law.", "SoundAsset": "/Game/Audio/Generated/SW_MN_Music_ItalicRoad.SW_MN_Music_ItalicRoad", "SourceFile": "mn_music_italic_road.wav", "MoodTags": ["Craft", "Order"], "EstimatedDurationSeconds": 20.0, "VolumeMultiplier": 0.85, "bLooping": True},
        {"AudioId": "music_convergence_core", "RegionId": "Convergence", "CategoryId": "Music", "DisplayName": "Convergence Core Motif", "Description": "Cold harmonic pulse for the buried ship and legacy fracture.", "SoundAsset": "/Game/Audio/Generated/SW_MN_Music_ConvergenceCore.SW_MN_Music_ConvergenceCore", "SourceFile": "mn_music_convergence_core.wav", "MoodTags": ["Buried", "Revelation"], "EstimatedDurationSeconds": 20.0, "VolumeMultiplier": 0.9, "bLooping": True},
        {"AudioId": "music_oracle_path", "RegionId": "Convergence", "CategoryId": "Music", "DisplayName": "Oracle Path Motif", "Description": "Clinical shimmer for curated truth and signal control.", "SoundAsset": "/Game/Audio/Generated/SW_MN_Music_OraclePath.SW_MN_Music_OraclePath", "SourceFile": "mn_music_oracle_path.wav", "MoodTags": ["OracleAI", "TruthControl"], "EstimatedDurationSeconds": 18.0, "VolumeMultiplier": 0.88, "bLooping": True},
        {"AudioId": "music_sky_path", "RegionId": "Convergence", "CategoryId": "Music", "DisplayName": "Sky Path Motif", "Description": "Drumming thunder motif for storm kingship and fear.", "SoundAsset": "/Game/Audio/Generated/SW_MN_Music_SkyPath.SW_MN_Music_SkyPath", "SourceFile": "mn_music_sky_path.wav", "MoodTags": ["SkyRuler", "Spectacle"], "EstimatedDurationSeconds": 18.0, "VolumeMultiplier": 0.88, "bLooping": True},
        {"AudioId": "music_bronze_path", "RegionId": "Convergence", "CategoryId": "Music", "DisplayName": "Bronze Path Motif", "Description": "Hammered pulse for coercive law and measured domination.", "SoundAsset": "/Game/Audio/Generated/SW_MN_Music_BronzePath.SW_MN_Music_BronzePath", "SourceFile": "mn_music_bronze_path.wav", "MoodTags": ["BronzeLawgiver", "Order"], "EstimatedDurationSeconds": 18.0, "VolumeMultiplier": 0.88, "bLooping": True},
        {"AudioId": "ambience_opening_ash", "RegionId": "Opening", "CategoryId": "Ambience", "DisplayName": "Opening Ash Ambience", "Description": "Ash wind, debris, and low ravine pressure.", "SoundAsset": "/Game/Audio/Generated/SW_MN_Ambience_OpeningAsh.SW_MN_Ambience_OpeningAsh", "SourceFile": "mn_ambience_opening_ash.wav", "MoodTags": ["Ash", "Wind"], "EstimatedDurationSeconds": 14.0, "VolumeMultiplier": 0.72, "bLooping": True},
        {"AudioId": "ambience_egypt_market", "RegionId": "Egypt", "CategoryId": "Ambience", "DisplayName": "Egypt Market Ambience", "Description": "Dry market bed with cloth, footsteps, and distant ritual sound.", "SoundAsset": "/Game/Audio/Generated/SW_MN_Ambience_EgyptMarket.SW_MN_Ambience_EgyptMarket", "SourceFile": "mn_ambience_egypt_market.wav", "MoodTags": ["Market", "Temple"], "EstimatedDurationSeconds": 14.0, "VolumeMultiplier": 0.72, "bLooping": True},
        {"AudioId": "ambience_greece_wind", "RegionId": "Greece", "CategoryId": "Ambience", "DisplayName": "Greece Wind Ambience", "Description": "High ridge wind and distant crowd resonance.", "SoundAsset": "/Game/Audio/Generated/SW_MN_Ambience_GreeceWind.SW_MN_Ambience_GreeceWind", "SourceFile": "mn_ambience_greece_wind.wav", "MoodTags": ["Wind", "Cliff"], "EstimatedDurationSeconds": 14.0, "VolumeMultiplier": 0.72, "bLooping": True},
        {"AudioId": "ambience_italic_settlement", "RegionId": "ItalicWest", "CategoryId": "Ambience", "DisplayName": "Italic Settlement Ambience", "Description": "Road, forge, timber, and hill settlement bed.", "SoundAsset": "/Game/Audio/Generated/SW_MN_Ambience_ItalicSettlement.SW_MN_Ambience_ItalicSettlement", "SourceFile": "mn_ambience_italic_settlement.wav", "MoodTags": ["Road", "Forge"], "EstimatedDurationSeconds": 14.0, "VolumeMultiplier": 0.72, "bLooping": True},
        {"AudioId": "ambience_convergence_core", "RegionId": "Convergence", "CategoryId": "Ambience", "DisplayName": "Convergence Core Ambience", "Description": "Cold buried hum, particulate drift, and unstable chamber resonance.", "SoundAsset": "/Game/Audio/Generated/SW_MN_Ambience_ConvergenceCore.SW_MN_Ambience_ConvergenceCore", "SourceFile": "mn_ambience_convergence_core.wav", "MoodTags": ["Core", "Buried"], "EstimatedDurationSeconds": 14.0, "VolumeMultiplier": 0.74, "bLooping": True},
        {"AudioId": "stinger_miracle_pulse", "RegionId": "Opening", "CategoryId": "Stinger", "DisplayName": "Miracle Pulse", "Description": "Short tonal shock for miracle or omen beats.", "SoundAsset": "/Game/Audio/Generated/SW_MN_Stinger_MiraclePulse.SW_MN_Stinger_MiraclePulse", "SourceFile": "mn_stinger_miracle_pulse.wav", "MoodTags": ["Miracle"], "EstimatedDurationSeconds": 3.5, "VolumeMultiplier": 1.0, "bLooping": False},
        {"AudioId": "stinger_quest_resolve", "RegionId": "Convergence", "CategoryId": "Stinger", "DisplayName": "Quest Resolve", "Description": "Resolution stinger for cutscene and quest completion beats.", "SoundAsset": "/Game/Audio/Generated/SW_MN_Stinger_QuestResolve.SW_MN_Stinger_QuestResolve", "SourceFile": "mn_stinger_quest_resolve.wav", "MoodTags": ["Resolution"], "EstimatedDurationSeconds": 3.0, "VolumeMultiplier": 1.0, "bLooping": False},
        {"AudioId": "stinger_antagonist_shift", "RegionId": "Convergence", "CategoryId": "Stinger", "DisplayName": "Antagonist Shift", "Description": "Escalation hit for dominant deity branch reveals.", "SoundAsset": "/Game/Audio/Generated/SW_MN_Stinger_AntagonistShift.SW_MN_Stinger_AntagonistShift", "SourceFile": "mn_stinger_antagonist_shift.wav", "MoodTags": ["Antagonist"], "EstimatedDurationSeconds": 3.2, "VolumeMultiplier": 1.0, "bLooping": False}
    ]

    external_asset_licenses = [
        {"AssetId": "tool_blender", "CategoryId": "Tool", "DisplayName": "Blender", "SourceName": "Blender Foundation", "LicenseName": "GPL", "SourceUrl": "https://www.blender.org/about/license/", "UsageNotes": "Used for free/open geometry, VDB, and support asset generation.", "bGeneratedInProject": False},
        {"AssetId": "tool_audacity", "CategoryId": "Tool", "DisplayName": "Audacity", "SourceName": "Audacity Team", "LicenseName": "GPL", "SourceUrl": "https://manual.audacityteam.org/man/license.html", "UsageNotes": "Used for audio cleanup and mastering of generated stems.", "bGeneratedInProject": False},
        {"AssetId": "tool_ffmpeg", "CategoryId": "Tool", "DisplayName": "FFmpeg", "SourceName": "FFmpeg Project", "LicenseName": "LGPL/GPL", "SourceUrl": "https://ffmpeg.org/legal.html", "UsageNotes": "Used for transcode and review media generation.", "bGeneratedInProject": False},
        {"AssetId": "tool_krita", "CategoryId": "Tool", "DisplayName": "Krita", "SourceName": "Krita Foundation", "LicenseName": "GPL", "SourceUrl": "https://krita.org/en/about/license/", "UsageNotes": "Approved for concept paintovers and texture authoring.", "bGeneratedInProject": False},
        {"AssetId": "tool_gimp", "CategoryId": "Tool", "DisplayName": "GIMP", "SourceName": "The GIMP Team", "LicenseName": "GPL", "SourceUrl": "https://www.gimp.org/about/COPYING", "UsageNotes": "Approved for texture cleanup and image edits.", "bGeneratedInProject": False},
        {"AssetId": "tool_material_maker", "CategoryId": "Tool", "DisplayName": "Material Maker", "SourceName": "Romain Dura", "LicenseName": "MIT", "SourceUrl": "https://github.com/RodZill4/material-maker", "UsageNotes": "Approved for procedural material prototyping.", "bGeneratedInProject": False},
        {"AssetId": "tool_lmms", "CategoryId": "Tool", "DisplayName": "LMMS", "SourceName": "LMMS Developers", "LicenseName": "GPL", "SourceUrl": "https://lmms.io/", "UsageNotes": "Approved for temp music sketching and stem creation.", "bGeneratedInProject": False},
        {"AssetId": "tool_piper", "CategoryId": "Tool", "DisplayName": "Piper TTS", "SourceName": "Rhasspy", "LicenseName": "MIT", "SourceUrl": "https://github.com/rhasspy/piper", "UsageNotes": "Only use voice models whose model cards explicitly permit intended use.", "bGeneratedInProject": False},
        {"AssetId": "generated_audio_alpha", "CategoryId": "GeneratedAsset", "DisplayName": "Generated Alpha Audio Source Set", "SourceName": "Many Names Project", "LicenseName": "Project-owned generated output", "SourceUrl": "", "UsageNotes": "Procedurally generated in-project source WAVs for temp ambience, score, and stingers.", "bGeneratedInProject": True},
        {"AssetId": "generated_sequences_alpha", "CategoryId": "GeneratedAsset", "DisplayName": "Generated Alpha Cinematic Sequences", "SourceName": "Many Names Project", "LicenseName": "Project-owned generated output", "SourceUrl": "", "UsageNotes": "Sequencer assets generated from cinematic scene data for alpha cutscene bootstrap.", "bGeneratedInProject": True}
    ]

    save_json("regions.json", regions)
    save_json("quests.json", sorted(quests, key=lambda row: row["QuestId"]))
    save_json("quest_steps.json", sorted(steps, key=lambda row: (row["QuestId"], row["StepIndex"])))
    save_json("dialogue_scenes.json", sorted(scenes, key=lambda row: row["QuestId"]))
    save_json("dialogue_choices.json", sorted(choices, key=lambda row: row["ChoiceId"]))
    save_json("choice_consequences.json", sorted(consequences, key=lambda row: row["ChoiceId"]))
    save_json("character_cast.json", sorted(cast, key=lambda row: row["CharacterId"]))
    save_json("ambient_profiles.json", sorted(ambient, key=lambda row: row["ProfileId"]))
    save_json("cinematic_scenes.json", cinematic_scenes)
    save_json("audio_profiles.json", audio_profiles)
    save_json("external_asset_licenses.json", external_asset_licenses)

    print("Alpha content data generated.")
    print(f"quests={len(quests)} steps={len(steps)} scenes={len(scenes)} choices={len(choices)} cast={len(cast)}")


if __name__ == "__main__":
    main()
