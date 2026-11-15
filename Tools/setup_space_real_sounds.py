# RESHOOT — remplace les sons réalistes provisoires du Spatial par ceux de Tools/synth_space_real_sfx.py.
# Même nom d'asset (/Game/RESHOOT/Space/Audio/Real/SW_Space_*) : BP_SpaceLaser, BP_SpaceDrone et BP_ReshootCharacter
# gardent leurs références, rien à rebrancher. À lancer dans l'éditeur (console Python), hors partie.
import unreal

at = unreal.AssetToolsHelpers.get_asset_tools()
SRC = unreal.Paths.project_dir() + 'SourceArt/Space/Audio/Real/'
DEST = '/Game/RESHOOT/Space/Audio/Real'
NAMES = ['SW_Space_LaserFire_Real', 'SW_Space_DroneFire_Real_1', 'SW_Space_Thruster_Real_1', 'SW_Space_Thruster_Real_2']
tasks = []
for n in NAMES:
    t = unreal.AssetImportTask(); t.filename = SRC + n + '.wav'; t.destination_path = DEST; t.destination_name = n
    t.automated = True; t.replace_existing = True; t.save = True
    tasks.append(t)
at.import_asset_tasks(tasks)

out = {}
for n in NAMES:
    s = unreal.load_asset(f'{DEST}/{n}')
    out[n] = round(s.get_editor_property('duration'), 2)
laser = unreal.get_default_object(unreal.load_asset('/Game/RESHOOT/Space/Weapons/BP_SpaceLaser').generated_class())
drone = unreal.get_default_object(unreal.load_asset('/Game/RESHOOT/Space/Enemies/BP_SpaceDrone').generated_class())
char = unreal.get_default_object(unreal.load_asset('/Game/RESHOOT/Core/Blueprints/BP_ReshootCharacter').generated_class())
out['BP_SpaceLaser.fire_sound'] = laser.get_editor_property('fire_sound').get_path_name()
out['BP_SpaceDrone.fire_sound'] = drone.get_editor_property('fire_sound').get_path_name()
out['BP_ReshootCharacter.real_thruster_sounds'] = [s.get_name() for s in char.get_editor_property('real_thruster_sounds')]
unreal.log_warning('[SPACE SOUNDS] ' + str(out))
