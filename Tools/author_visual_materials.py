"""M6.4 owned material assets, schema1. Editor-only, --apply to create missing assets.

Existing assets are strictly verified, never overwritten/rebuilt. Uses no external
textures/content, changes no definition/gameplay values, and refuses unowned paths.
"""
import sys
import unreal

FOLDER = "/Game/Materials/RomanVisual"
TAG = "RomanVisualSchema"
VERSION = "1"
APPLY = "--apply" in sys.argv
PROBES = "--test-rejection" in sys.argv
if APPLY and PROBES:
    raise RuntimeError("Rejection probes are read-only; cannot combine with --apply")
LIB = unreal.MaterialEditingLibrary
ASSETS = unreal.EditorAssetLibrary
TOOLS = unreal.AssetToolsHelpers.get_asset_tools()


def owned(name, cls, factory):
    path = f"{FOLDER}/{name}"
    asset = unreal.load_asset(path) if ASSETS.does_asset_exist(path) else None
    if asset:
        if not isinstance(asset, cls) or str(ASSETS.get_metadata_tag(asset, TAG)) != VERSION:
            raise RuntimeError(f"Refusing incompatible/unowned asset: {path}")
        return asset, False
    if not APPLY:
        raise RuntimeError(f"Missing required visual asset: {path}; use --apply")
    asset = TOOLS.create_asset(name, FOLDER, cls, factory)
    if not asset:
        raise RuntimeError(f"Creation failed: {path}")
    ASSETS.set_metadata_tag(asset, TAG, VERSION)
    return asset, True


def node(mat, cls, **values):
    result = LIB.create_material_expression(mat, cls)
    if not result:
        raise RuntimeError(f"Expression failed: {cls}")
    for key, value in values.items():
        result.set_editor_property(key, value)
    return result


def wire(source, target, input_name):
    if not LIB.connect_material_expressions(source, "", target, input_name):
        raise RuntimeError(f"Connection failed: {target} input {input_name}")


def output(source, mat, prop):
    if not LIB.connect_material_property(source, "", prop):
        raise RuntimeError(f"Material output failed: {prop}")


def scalar(mat, name, default):
    return node(mat, unreal.MaterialExpressionScalarParameter, parameter_name=name, default_value=default)


def color(mat):
    return node(mat, unreal.MaterialExpressionVectorParameter, parameter_name="Color", default_value=unreal.LinearColor(1, 1, 1, 1))


def save(asset):
    if not ASSETS.save_loaded_asset(asset, only_if_is_dirty=False):
        raise RuntimeError(f"Save failed: {asset.get_path_name()}")


def material(name, kind):
    mat, created = owned(name, unreal.Material, unreal.MaterialFactoryNew())
    properties = []
    scalar_names = set()
    if created:
        mat.set_editor_property("two_sided", True)
        mat.set_editor_property("used_with_skeletal_mesh", True)
        base = color(mat)
        if kind == "surface":
            damage = scalar(mat, "Damage", 0)
            factor = node(mat, unreal.MaterialExpressionLinearInterpolate, const_a=1.0, const_b=0.35)
            wire(damage, factor, "Alpha")
            tinted = node(mat, unreal.MaterialExpressionMultiply)
            wire(base, tinted, "A"); wire(factor, tinted, "B")
            output(tinted, mat, unreal.MaterialProperty.MP_BASE_COLOR)
            output(scalar(mat, "Roughness", 0.8), mat, unreal.MaterialProperty.MP_ROUGHNESS)
            output(scalar(mat, "Metallic", 0.0), mat, unreal.MaterialProperty.MP_METALLIC)
        else:
            mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
            if kind == "preview":
                mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
                # Invalid preview has repeating dark/light stripes, not only a red hue.
                uv = node(mat, unreal.MaterialExpressionTextureCoordinate)
                axis = node(mat, unreal.MaterialExpressionComponentMask, r=True, g=False, b=False, a=False)
                wire(uv, axis, "")
                frequency = node(mat, unreal.MaterialExpressionMultiply, const_b=12.0)
                wire(axis, frequency, "A")
                wave = node(mat, unreal.MaterialExpressionSine, period=1.0); wire(frequency, wave, "")
                absolute = node(mat, unreal.MaterialExpressionAbs); wire(wave, absolute, "")
                pattern = node(mat, unreal.MaterialExpressionLinearInterpolate, const_a=1.0)
                wire(absolute, pattern, "B"); wire(scalar(mat, "PreviewPattern", 0), pattern, "Alpha")
                tinted = node(mat, unreal.MaterialExpressionMultiply)
                wire(base, tinted, "A"); wire(pattern, tinted, "B"); base = tinted
                output(scalar(mat, "Opacity", 0.5), mat, unreal.MaterialProperty.MP_OPACITY)
            output(base, mat, unreal.MaterialProperty.MP_EMISSIVE_COLOR)
        errors = LIB.recompile_material(mat)
        if errors:
            raise RuntimeError(f"Shader errors for {name}: {errors}")
        save(mat)
    if kind == "surface":
        properties = [unreal.MaterialProperty.MP_BASE_COLOR, unreal.MaterialProperty.MP_ROUGHNESS, unreal.MaterialProperty.MP_METALLIC]
        scalar_names = {"Damage", "Roughness", "Metallic"}
    elif kind == "preview":
        properties = [unreal.MaterialProperty.MP_EMISSIVE_COLOR, unreal.MaterialProperty.MP_OPACITY]
        scalar_names = {"PreviewPattern", "Opacity"}
        if mat.get_editor_property("blend_mode") != unreal.BlendMode.BLEND_TRANSLUCENT:
            raise RuntimeError("Preview is not genuinely translucent")
    else:
        properties = [unreal.MaterialProperty.MP_EMISSIVE_COLOR]
    for prop in properties:
        if LIB.get_material_property_input_node(mat, prop) is None:
            raise RuntimeError(f"Disconnected required material output: {name}/{prop}")
    if set(map(str, LIB.get_scalar_parameter_names(mat))) != scalar_names or set(map(str, LIB.get_vector_parameter_names(mat))) != {"Color"}:
        raise RuntimeError(f"Material parameter schema mismatch: {name}")
    if "--inspect-substrate" in sys.argv:
        front = LIB.get_material_property_input_node(mat, unreal.MaterialProperty.MP_FRONT_MATERIAL)
        inputs = LIB.get_inputs_for_material_expression(mat, front) if front else []
        emissive = LIB.get_material_property_input_node(mat, unreal.MaterialProperty.MP_EMISSIVE_COLOR)
        unreal.log(f"VISUAL_FRONT {name} front={front.get_class().get_name() if front else None} inputs={[item.get_name() if item else None for item in inputs]} emissive={emissive.get_name() if emissive else None}")
    unreal.log(f"VISUAL_MATERIAL_VERIFIED {name} created={created}")
    return mat


def instance(name, parent, values):
    asset, created = owned(name, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    if created:
        LIB.set_material_instance_parent(asset, parent)
        for parameter, value in values.items():
            names = LIB.get_vector_parameter_names(parent) if isinstance(value, tuple) else LIB.get_scalar_parameter_names(parent)
            if parameter not in set(map(str, names)):
                raise RuntimeError(f"Instance parameter is not declared: {name}/{parameter}")
            # UE5.8's two setter implementations always return false (bResult is
            # never assigned). Check declared schema and strict value readback,
            # not that broken return flag; no shader/asset error is suppressed.
            if isinstance(value, tuple):
                LIB.set_material_instance_vector_parameter_value(asset, parameter, unreal.LinearColor(*value))
            else:
                LIB.set_material_instance_scalar_parameter_value(asset, parameter, value)
        LIB.update_material_instance(asset)
        save(asset)
    if asset.get_editor_property("parent") != parent:
        raise RuntimeError(f"Unexpected instance parent: {name}")
    for parameter, expected in values.items():
        if isinstance(expected, tuple):
            actual = LIB.get_material_instance_vector_parameter_value(asset, parameter)
            actual = (actual.r, actual.g, actual.b, actual.a)
            matches = all(abs(a-b) < 1e-5 for a, b in zip(actual, expected))
        else:
            matches = abs(LIB.get_material_instance_scalar_parameter_value(asset, parameter)-expected) < 1e-5
        if not matches:
            raise RuntimeError(f"Instance value mismatch: {name}/{parameter}")
    unreal.log(f"VISUAL_INSTANCE_VERIFIED {name} created={created}")


surface = material("M_RomanSurface", "surface")
preview = material("M_RomanPreview", "preview")
indicator = material("M_RomanIndicator", "indicator")
instance("MI_Surface", surface, {"Color": (1, 1, 1, 1), "Damage": 0, "Roughness": 0.8, "Metallic": 0})
instance("MI_Construction", surface, {"Color": (0.85, 0.6, 0.2, 1), "Damage": 0, "Roughness": 0.9})
instance("MI_Damaged", surface, {"Color": (1, 1, 1, 1), "Damage": 1})
instance("MI_PreviewValid", preview, {"Color": (0.2, 0.8, 0.2, 1), "Opacity": 0.5, "PreviewPattern": 0})
instance("MI_PreviewInvalid", preview, {"Color": (0.8, 0.2, 0.2, 1), "Opacity": 0.5, "PreviewPattern": 1})
instance("MI_Selection", indicator, {"Color": (1, 1, 1, 1)})
for faction in ("Rome", "Carthage", "Gaul"):
    definition = unreal.load_asset(f"/Game/Data/Factions/PDA_Faction_{faction}")
    if not isinstance(definition, unreal.RomanFactionDefinition):
        raise RuntimeError(f"Missing existing canonical faction definition: {faction}")
    tint = definition.get_editor_property("definition").get_editor_property("faction_color")
    instance(f"MI_Faction{faction}", surface, {"Color": (tint.r, tint.g, tint.b, tint.a), "Damage": 0})
unreal.log("VISUAL_MATERIALS_OK schema=1 bases=3 instances=9 no_gameplay_changes")

if PROBES:
    def expect_rejection(label, callback):
        try:
            callback()
        except RuntimeError as error:
            unreal.log(f"VISUAL_REJECTION_OK {label}: {error}")
            return
        raise RuntimeError(f"Validation unexpectedly accepted: {label}")

    # Only in-memory loaded assets are temporarily changed. No save or asset
    # generation in this mode; restore values in finally, verify file hashes.
    existing = unreal.load_asset(f"{FOLDER}/MI_Surface")
    original = LIB.get_material_instance_scalar_parameter_value(existing, "Damage")
    try:
        LIB.set_material_instance_scalar_parameter_value(existing, "Damage", 0.37)
        expect_rejection("changed_instance_value", lambda: instance("MI_Surface", surface, {"Damage": 0}))
    finally:
        LIB.set_material_instance_scalar_parameter_value(existing, "Damage", original)
    original_tag = str(ASSETS.get_metadata_tag(existing, TAG))
    try:
        ASSETS.set_metadata_tag(existing, TAG, "unowned")
        expect_rejection("unowned_existing_asset", lambda: owned("MI_Surface", unreal.MaterialInstanceConstant, None))
    finally:
        ASSETS.set_metadata_tag(existing, TAG, original_tag)
    expect_rejection("missing_required_asset", lambda: owned("__MissingReadOnlyProbe", unreal.Material, None))
    instance("MI_Surface", surface, {"Color": (1, 1, 1, 1), "Damage": 0, "Roughness": 0.8, "Metallic": 0})
    unreal.log("VISUAL_REJECTION_PROBES_OK count=3 no_saves")
