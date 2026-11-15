# RESHOOT — sons du film noir, synthétisés par Tools/synth_noir_sfx.py (numpy, aucune source extérieure).
# Sources : SourceArt/Noir/Audio/*.wav, importées dans /Game/RESHOOT/Noir/Audio.
# Crée les Blueprints enfants (pour assigner les sons, comme BP_SpaceLaser / BP_SpaceDrone) :
#   - BP_NoirRevolver : FireSound, EmptySound, ReloadSound
#   - BP_NoirGangster : FireSound
#   - BP_NoirBlackoutEvent : PowerDownSound, PowerUpSound
# puis fait pointer DA_Noir dessus et remplace les gangsters posés dans L_Noir par BP_NoirGangster.
# À lancer dans l'éditeur (console Python), hors partie.
import unreal

eal = unreal.EditorAssetLibrary
at = unreal.AssetToolsHelpers.get_asset_tools()
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
SRC = unreal.Paths.project_dir() + 'SourceArt/Noir/Audio/'
DEST = '/Game/RESHOOT/Noir/Audio'
BPD = '/Game/RESHOOT/Noir/Blueprints'
out = {}

def sound(name):
    t = unreal.AssetImportTask(); t.filename = SRC + name + '.wav'; t.destination_path = DEST; t.destination_name = name
    t.automated = True; t.replace_existing = True; t.save = True
    at.import_asset_tasks([t])
    return unreal.load_asset(f'{DEST}/{name}')

def blueprint(name, parent):
    path = f'{BPD}/{name}'
    if eal.does_asset_exist(path):
        return unreal.load_asset(path)
    f = unreal.BlueprintFactory(); f.set_editor_property('parent_class', unreal.load_class(None, parent))
    return at.create_asset(name, BPD, unreal.Blueprint, f)

def configure(bp, values):
    cdo = unreal.get_default_object(bp.generated_class())
    for k, v in values.items():
        cdo.set_editor_property(k, v)
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    eal.save_loaded_asset(bp)
    out[bp.get_name()] = {k: (cdo.get_editor_property(k).get_name() if cdo.get_editor_property(k) else None) for k in values}

revolver = blueprint('BP_NoirRevolver', '/Script/RESHOOT.NoirRevolver')
configure(revolver, {'fire_sound': sound('SW_Noir_Revolver_Fire'), 'empty_sound': sound('SW_Noir_Revolver_Empty'),
                     'reload_sound': sound('SW_Noir_Revolver_Reload')})
gangster = blueprint('BP_NoirGangster', '/Script/RESHOOT.NoirGangster')
configure(gangster, {'fire_sound': sound('SW_Noir_Gangster_Fire')})
blackout = blueprint('BP_NoirBlackoutEvent', '/Script/RESHOOT.NoirBlackoutEvent')
configure(blackout, {'power_down_sound': sound('SW_Noir_Blackout_Down'), 'power_up_sound': sound('SW_Noir_Blackout_Up')})

da = unreal.load_asset('/Game/RESHOOT/Noir/DA_Noir')
da.set_editor_property('weapon_class', revolver.generated_class())
da.set_editor_property('enemy_classes', [gangster.generated_class()])
da.set_editor_property('event_class', blackout.generated_class())
eal.save_loaded_asset(da)

# Gangsters posés dans L_Noir : même place, même nom, version Blueprint
les.load_level('/Game/RESHOOT/Noir/L_Noir')
cpp = unreal.load_class(None, '/Script/RESHOOT.NoirGangster')
swapped = 0
for a in eas.get_all_level_actors():
    if a.get_class() == cpp:
        lbl, tr, folder = a.get_actor_label(), a.get_actor_transform(), a.get_folder_path()
        eas.destroy_actor(a)
        n = eas.spawn_actor_from_class(gangster.generated_class(), tr.translation, tr.rotation.rotator())
        n.set_actor_label(lbl); n.set_folder_path(folder)
        swapped += 1
les.save_current_level()
out['gangsters_remplaces'] = swapped
unreal.log_warning('[NOIR SOUNDS] ' + str(out))
