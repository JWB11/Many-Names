#!/usr/bin/env python3

import json
import math
import os
import random
import shutil
import struct
import subprocess
import wave
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
DATA = ROOT / "Data"
GENERATED = ROOT / "Generated" / "AudioSources"
VOICE_DIR = GENERATED / "voices"
VOICE_MANIFEST = GENERATED / "voice_manifest.json"
SAMPLE_RATE = 22050


def load_json(name: str):
    return json.loads((DATA / name).read_text(encoding="utf-8"))


def ensure_dirs() -> None:
    for folder in (GENERATED, VOICE_DIR):
        folder.mkdir(parents=True, exist_ok=True)


def clamp(sample: float) -> int:
    sample = max(-1.0, min(1.0, sample))
    return int(sample * 32767)


def write_wav(path: Path, samples: list[float], sample_rate: int = SAMPLE_RATE) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with wave.open(str(path), "wb") as handle:
        handle.setnchannels(1)
        handle.setsampwidth(2)
        handle.setframerate(sample_rate)
        frames = b"".join(struct.pack("<h", clamp(sample)) for sample in samples)
        handle.writeframes(frames)


def synth_profile(audio_id: str, category: str, duration: float) -> list[float]:
    duration = max(duration, 2.0)
    total = int(SAMPLE_RATE * duration)
    rng = random.Random(audio_id)
    base = {
        "Music": 110.0 + rng.randint(0, 80),
        "Ambience": 55.0 + rng.randint(0, 40),
        "Stinger": 180.0 + rng.randint(0, 120),
    }.get(category, 90.0 + rng.randint(0, 60))
    samples = []
    for i in range(total):
        t = i / SAMPLE_RATE
        env = min(1.0, t / 0.2) * min(1.0, (duration - t) / 0.35)
        tone = math.sin(2.0 * math.pi * base * t)
        overtone = 0.55 * math.sin(2.0 * math.pi * base * 1.5 * t + 0.7)
        low = 0.35 * math.sin(2.0 * math.pi * (base * 0.5) * t + 1.2)
        noise = (rng.random() * 2.0 - 1.0) * 0.08
        if category == "Ambience":
            noise *= 1.6
            tone *= 0.35
        elif category == "Stinger":
            tone *= 1.25
            overtone *= 0.9
        sample = (tone + overtone + low) * 0.18 * env + noise * env
        samples.append(sample)
    return samples


def available_tts_backend() -> str:
    if shutil.which("piper"):
        return "piper"
    if shutil.which("espeak-ng"):
        return "espeak-ng"
    if shutil.which("say") and shutil.which("ffmpeg"):
        return "say"
    return "synthetic"


def synth_voice_placeholder(text: str) -> list[float]:
    words = max(4, len(text.split()))
    duration = min(14.0, max(2.5, words * 0.32))
    total = int(SAMPLE_RATE * duration)
    samples = []
    for i in range(total):
        t = i / SAMPLE_RATE
        syllable = math.sin(2.0 * math.pi * (150.0 + 30.0 * math.sin(t * 6.0)) * t)
        cadence = 0.5 + 0.5 * math.sin(t * 4.0)
        envelope = min(1.0, t / 0.06) * min(1.0, (duration - t) / 0.12)
        samples.append(syllable * cadence * envelope * 0.14)
    return samples


def generate_voice(scene_id: str, text: str, output_path: Path, backend: str) -> dict:
    output_path.parent.mkdir(parents=True, exist_ok=True)
    if backend == "piper":
        model = os.environ.get("PIPER_VOICE_MODEL", "")
        if model and Path(model).exists():
            subprocess.run(
                ["piper", "--model", model, "--output_file", str(output_path)],
                input=text.encode("utf-8"),
                check=True,
            )
            return {"SceneId": scene_id, "Backend": "piper", "File": output_path.name}
    elif backend == "espeak-ng":
        subprocess.run(["espeak-ng", "-w", str(output_path), text], check=True)
        return {"SceneId": scene_id, "Backend": "espeak-ng", "File": output_path.name}
    elif backend == "say":
        subprocess.run(["say", "-o", str(output_path.with_suffix(".aiff")), text], check=True)
        subprocess.run(["ffmpeg", "-y", "-i", str(output_path.with_suffix(".aiff")), str(output_path)], check=True)
        output_path.with_suffix(".aiff").unlink(missing_ok=True)
        return {"SceneId": scene_id, "Backend": "say", "File": output_path.name}

    write_wav(output_path, synth_voice_placeholder(text))
    return {"SceneId": scene_id, "Backend": "synthetic", "File": output_path.name}


def main() -> None:
    ensure_dirs()
    audio_profiles = load_json("audio_profiles.json")
    dialogue_scenes = load_json("dialogue_scenes.json")
    cinematic_scenes = load_json("cinematic_scenes.json")

    for profile in audio_profiles:
        output = GENERATED / profile["SourceFile"]
        samples = synth_profile(profile["AudioId"], profile["CategoryId"], profile["EstimatedDurationSeconds"])
        write_wav(output, samples)

    backend = available_tts_backend()
    voice_manifest = []
    seen_scene_ids = set()
    for scene in dialogue_scenes:
        scene_id = scene["SceneId"]
        seen_scene_ids.add(scene_id)
        voice_manifest.append(
            generate_voice(scene_id, f"{scene.get('SpeakerName', '')}. {scene.get('BodyText', '')}", VOICE_DIR / f"{scene_id}.wav", backend)
        )
    for cinematic in cinematic_scenes:
        scene_id = cinematic["SceneId"]
        if scene_id in seen_scene_ids:
            continue
        voice_manifest.append(
            generate_voice(scene_id, f"{cinematic.get('Title', '')}. {cinematic.get('Summary', '')}", VOICE_DIR / f"{scene_id}.wav", backend)
        )

    VOICE_MANIFEST.write_text(json.dumps(voice_manifest, indent=2) + "\n", encoding="utf-8")
    print(f"Generated {len(audio_profiles)} audio profiles and {len(voice_manifest)} temporary voice clips using backend={backend}.")


if __name__ == "__main__":
    main()
