# RESHOOT — sons du décor en carton (Space), faits à la bouche. À lancer dans l'éditeur (Unreal MCP, execute_python),
# après la compilation qui ajoute PropFireSounds / PropThrusterSounds.
# Sources : SourceArt/Space/Audio/Prop/*.mp3, importées dans /Game/RESHOOT/Space/Audio/Prop.
#   - BP_SpaceLaser.PropFireSounds   : tir du fusil en carton
#   - BP_SpaceDrone.PropFireSounds   : tir des drones en carton
#   - BP_ReshootCharacter.PropThrusterSounds : bouffées de réacteur en apesanteur
#     (BP_ReshootCharacter est créé ici s'il n'existe pas, et devient le pion de BP_ReshootGameMode)
import unreal

eal = unreal.EditorAssetLibrary
at = unreal.AssetToolsHelpers.get_asset_tools()
SRC = unreal.Paths.project_dir() + 'SourceArt/Space/Audio/Prop/'
DEST = '/Game/RESHOOT/Space/Audio/Prop'
NAMES = {'FireSound_placeholder_1': 'SW_Space_LaserFire_Prop_1', 'FireSound_placeholder_2': 'SW_Space_LaserFire_Prop_2',
         'Drone_placeholder_1': 'SW_Space_DroneFire_Prop_1', 'Drone_placeholder_2': 'SW_Space_DroneFire_Prop_2',
         'Throttle_placeholder_1': 'SW_Space_Thruster_Prop_1', 'Throttle_placeholder_2': 'SW_Space_Thruster_Prop_2'}
out = {}

def sound(name):
    path = f'{DEST}/{name}'
    if not eal.does_asset_exist(path):
        src = [k for k, v in NAMES.items() if v == name][0]
        t = unreal.AssetImportTask(); t.filename = SRC + src + '.mp3'; t.destination_path = DEST; t.destination_name = name
        t.automated = True; t.replace_existing = True; t.save = True
        at.import_asset_tasks([t])
    return unreal.load_asset(path)

def pair(prefix):
    return [sound(f'{prefix}_1'), sound(f'{prefix}_2')]

def set_and_save(bp_path, prop, value):
    bp = unreal.load_asset(bp_path)
    cdo = unreal.get_default_object(bp.generated_class())
    cdo.set_editor_property(prop, value)
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    eal.save_loaded_asset(bp)
    out[bp_path.split('/')[-1]] = [s.get_name() for s in cdo.get_editor_property(prop)]

set_and_save('/Game/RESHOOT/Space/Weapons/BP_SpaceLaser', 'prop_fire_sounds', pair('SW_Space_LaserFire_Prop'))
set_and_save('/Game/RESHOOT/Space/Enemies/BP_SpaceDrone', 'prop_fire_sounds', pair('SW_Space_DroneFire_Prop'))

# Pion joueur : Blueprint enfant de AReshootCharacter, pour y assigner des assets.
CHAR = '/Game/RESHOOT/Core/Blueprints/BP_ReshootCharacter'
if not eal.does_asset_exist(CHAR):
    f = unreal.BlueprintFactory()
    f.set_editor_property('parent_class', unreal.ReshootCharacter)
    at.create_asset('BP_ReshootCharacter', '/Game/RESHOOT/Core/Blueprints', unreal.Blueprint, f)
set_and_save(CHAR, 'prop_thruster_sounds', pair('SW_Space_Thruster_Prop'))
gm = unreal.load_asset('/Game/RESHOOT/Core/Blueprints/BP_ReshootGameMode')
gm_cdo = unreal.get_default_object(gm.generated_class())
gm_cdo.set_editor_property('default_pawn_class', unreal.load_asset(CHAR).generated_class())
unreal.BlueprintEditorLibrary.compile_blueprint(gm); eal.save_loaded_asset(gm)
out['pawn'] = str(gm_cdo.get_editor_property('default_pawn_class').get_name())
out
