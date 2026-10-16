"""M6.5 owned audio schema1: readonly by default; --apply creates missing only.

Run through the existing Unreal Python commandlet, never concurrently with PIE,
asset edits or builds. Existing assets/source files are never overwritten.
"""
from pathlib import Path
import sys
import unreal

PROJECT = Path(unreal.Paths.project_dir()).resolve()
sys.path.insert(0, str(PROJECT / "Tools"))
import roman_audio_recipe as recipe

APPLY = "--apply" in sys.argv
PROBES = "--test-rejection" in sys.argv
if APPLY and PROBES:
    raise RuntimeError("Readonly rejection probes cannot combine with --apply")
FOLDER = "/Game/Audio/RomanFeedback"
TAG = "RomanAudioSchema"
PCM_TAG = "RomanAudioPCM_SHA256"
ASSETS = unreal.EditorAssetLibrary
TOOLS = unreal.AssetToolsHelpers.get_asset_tools()


def load_owned(name, cls):
    path = f"{FOLDER}/{name}"
    if not ASSETS.does_asset_exist(path):
        return None
    asset = unreal.load_asset(path)
    if not isinstance(asset, cls) or str(ASSETS.get_metadata_tag(asset, TAG)) != recipe.SCHEMA:
        raise RuntimeError(f"Refusing incompatible/unowned audio: {path}")
    return asset


def save_new(asset):
    ASSETS.set_metadata_tag(asset, TAG, recipe.SCHEMA)
    if not ASSETS.save_loaded_asset(asset, only_if_is_dirty=False):
        raise RuntimeError(f"Audio asset save failed: {asset.get_path_name()}")


sound_class = load_owned("SC_RomanSFX", unreal.SoundClass)
created_class = sound_class is None
if created_class:
    if not APPLY:
        raise RuntimeError("Missing required SC_RomanSFX; use --apply")
    sound_class = TOOLS.create_asset("SC_RomanSFX", FOLDER, unreal.SoundClass, unreal.SoundClassFactory())
    if not sound_class:
        raise RuntimeError("Sound class creation failed")
properties = sound_class.get_editor_property("properties")
if abs(properties.get_editor_property("volume") - 1) > .0001 or abs(properties.get_editor_property("pitch") - 1) > .0001:
    raise RuntimeError("Unexpected authored SFX class volume/pitch")
if created_class:
    save_new(sound_class)
unreal.log(f"AUDIO_CLASS_VERIFIED SC_RomanSFX created={created_class}")


def verify(name, asset):
    if str(ASSETS.get_metadata_tag(asset, PCM_TAG)) != recipe.digest(name):
        raise RuntimeError(f"PCM recipe fingerprint mismatch: {name}")
    if asset.get_editor_property("num_channels") != 1 or asset.get_editor_property("imported_sample_rate") != recipe.RATE:
        raise RuntimeError(f"Audio format mismatch: {name}")
    duration = len(recipe.pcm(name)) / (2 * recipe.RATE)
    if abs(asset.get_editor_property("duration") - duration) > .002:
        raise RuntimeError(f"Audio duration mismatch: {name}")
    if asset.get_editor_property("looping") or asset.get_editor_property("sound_class_object") != sound_class:
        raise RuntimeError(f"Audio playback/class mismatch: {name}")
    if abs(asset.get_editor_property("volume") - 1) > .0001 or abs(asset.get_editor_property("pitch") - 1) > .0001:
        raise RuntimeError(f"Authored audio volume/pitch mismatch: {name}")


for name in recipe.RECIPES:
    asset = load_owned(name, unreal.SoundWave)
    created = asset is None
    if created:
        if not APPLY:
            raise RuntimeError(f"Missing required audio: {name}; use --apply")
        source_root = (PROJECT / "Artifacts" / "RomanAudioSource" / "schema1").resolve()
        source = (source_root / f"{name}.wav").resolve()
        if not source_root.is_relative_to(PROJECT / "Artifacts") or source.parent != source_root:
            raise RuntimeError("Audio output escaped allowed artifact root")
        data = recipe.wav(name)
        if source.exists() and source.read_bytes() != data:
            raise RuntimeError(f"Refusing overwrite of changed source: {source}")
        source_root.mkdir(parents=True, exist_ok=True)
        if not source.exists():
            source.write_bytes(data)
        task = unreal.AssetImportTask()
        task.set_editor_property("filename", str(source))
        task.set_editor_property("destination_path", FOLDER)
        task.set_editor_property("destination_name", name)
        task.set_editor_property("automated", True)
        task.set_editor_property("replace_existing", False)
        task.set_editor_property("save", False)
        TOOLS.import_asset_tasks([task])
        asset = unreal.load_asset(f"{FOLDER}/{name}")
        if not isinstance(asset, unreal.SoundWave):
            raise RuntimeError(f"Sound wave import failed: {name}")
        asset.set_editor_property("sound_class_object", sound_class)
        asset.set_editor_property("looping", False)
        ASSETS.set_metadata_tag(asset, PCM_TAG, recipe.digest(name))
        # Validate all readback before the first explicit save.
        verify(name, asset)
        save_new(asset)
    verify(name, asset)
    unreal.log(f"AUDIO_WAVE_VERIFIED {name} created={created} pcm_sha256={recipe.digest(name)}")

if PROBES:
    asset = load_owned("S_RomanSelect", unreal.SoundWave)
    previous = str(ASSETS.get_metadata_tag(asset, PCM_TAG))
    try:
        ASSETS.set_metadata_tag(asset, PCM_TAG, "invalid")
        try:
            verify("S_RomanSelect", asset)
        except RuntimeError as error:
            unreal.log(f"AUDIO_REJECTION_OK fingerprint: {error}")
        else:
            raise AssertionError("Changed PCM fingerprint accepted")
    finally:
        ASSETS.set_metadata_tag(asset, PCM_TAG, previous)
    previous_schema = str(ASSETS.get_metadata_tag(asset, TAG))
    try:
        ASSETS.set_metadata_tag(asset, TAG, "unowned")
        try:
            load_owned("S_RomanSelect", unreal.SoundWave)
        except RuntimeError as error:
            unreal.log(f"AUDIO_REJECTION_OK ownership: {error}")
        else:
            raise AssertionError("Unowned audio accepted")
    finally:
        ASSETS.set_metadata_tag(asset, TAG, previous_schema)
    previous_looping = asset.get_editor_property("looping")
    try:
        asset.set_editor_property("looping", True)
        try:
            verify("S_RomanSelect", asset)
        except RuntimeError as error:
            unreal.log(f"AUDIO_REJECTION_OK playback: {error}")
        else:
            raise AssertionError("Unexpected looping accepted")
    finally:
        asset.set_editor_property("looping", previous_looping)
    unreal.log("AUDIO_REJECTION_PROBES_OK count=3 no_saves")
unreal.log("AUDIO_ASSETS_OK schema=1 classes=1 waves=6 no_gameplay_changes")
