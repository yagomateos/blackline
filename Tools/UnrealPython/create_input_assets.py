"""Crea las Input Actions propias de BLACKLINE y el contexto IMC_Blackline.

Movimiento (WASD), salto y mirada vienen del pack de Epic (IMC_Default, IMC_MouseLook).
Este script añade el resto. Es idempotente: si un asset existe, lo reutiliza y rehace los mapeos.

  UnrealEditor-Cmd.exe Blackline.uproject -run=pythonscript -script="Tools/UnrealPython/create_input_assets.py"
"""
import unreal

ACTIONS_DIR = "/Game/Input/Actions"
IMC_PATH = "/Game/Input/IMC_Blackline"

# acción -> teclas (nombres de FKey)
BINDINGS = {
    "IA_Sprint":    ["LeftShift", "Gamepad_LeftThumbstick"],
    "IA_Crouch":    ["C", "LeftControl", "Gamepad_FaceButton_Right"],
    "IA_LeanLeft":  ["Q"],
    "IA_LeanRight": ["E"],
    "IA_Aim":       ["RightMouseButton", "Gamepad_LeftTrigger"],
    "IA_Fire":      ["LeftMouseButton", "Gamepad_RightTrigger"],
    "IA_Reload":    ["R", "Gamepad_FaceButton_Left"],
    "IA_Interact":  ["F", "Gamepad_DPad_Down"],
    "IA_Grenade":   ["G", "Gamepad_RightShoulder"],
    "IA_SwapWeapon": ["MouseScrollUp", "MouseScrollDown", "Gamepad_FaceButton_Top"],   # siguiente arma
    "IA_WeaponPrimary": ["One"],
    "IA_WeaponSecondary": ["Two"],
}

tools = unreal.AssetToolsHelpers.get_asset_tools()
lib = unreal.EditorAssetLibrary


def log(m):
    unreal.log(f"[BL_Input] {m}")


def get_or_create(name, path, cls, factory):
    full = f"{path}/{name}"
    if lib.does_asset_exist(full):
        return unreal.load_asset(full)
    return tools.create_asset(name, path, cls, factory)


def make_key(name):
    k = unreal.Key()
    k.import_text(name)
    return k


actions = {}
for name in BINDINGS:
    ia = get_or_create(name, ACTIONS_DIR, unreal.InputAction, unreal.InputAction_Factory())
    ia.set_editor_property("value_type", unreal.InputActionValueType.BOOLEAN)
    lib.save_loaded_asset(ia)
    actions[name] = ia
    log(f"Acción: {ia.get_path_name()}")

imc_dir, imc_name = IMC_PATH.rsplit("/", 1)
imc = get_or_create(imc_name, imc_dir, unreal.InputMappingContext, unreal.InputMappingContext_Factory())
imc.unmap_all()
count = 0
for name, keys in BINDINGS.items():
    for key_name in keys:
        imc.map_key(actions[name], make_key(key_name))
        count += 1
lib.save_loaded_asset(imc)
log(f"IMC_Blackline: {count} mapeos")
log("OK")
