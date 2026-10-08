import unreal
tools = unreal.AssetToolsHelpers.get_asset_tools()
lib = unreal.EditorAssetLibrary
path = "/Game/Audio/Settings/RE_UrbanOutdoor"
re = unreal.load_asset(path) if lib.does_asset_exist(path) else tools.create_asset("RE_UrbanOutdoor", "/Game/Audio/Settings", unreal.ReverbEffect, unreal.ReverbEffectFactory())
# Exterior urbano: reflexiones tempranas de fachadas, cola media, pocos agudos
for k, v in {"density": 0.6, "diffusion": 0.75, "gain": 0.32, "gain_hf": 0.5, "decay_time": 1.7, "decay_hf_ratio": 0.6,
             "reflections_gain": 0.25, "reflections_delay": 0.02, "late_gain": 1.1, "late_delay": 0.03, "air_absorption_gain_hf": 0.994}.items():
    re.set_editor_property(k, v)
lib.save_loaded_asset(re)
unreal.log("[BL_Reverb] OK")
