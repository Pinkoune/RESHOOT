# RESHOOT — bruitages Epidemic Sound (licence de la jam) pour le film noir et le médiéval.
# Les WAV téléchargés (pas dans le dépôt) sont importés dans /Game/RESHOOT/Audio/SFX, puis branchés :
#   Noir : BP_NoirRevolver (tir, clic à vide, recharge), BP_NoirGangster (tir), BP_NoirBlackoutEvent (coupure, retour).
#   Médiéval : BP_MedievalSword (épée, coup, bouclier, arc) ; BP_MedievalKnight, BP_MedievalArcher, BP_MedievalArrow créés
#   dans Medieval/Blueprints (sous-classes sans changement de code) et mis dans DA_Medieval à la place des classes C++.
# Usage (console Python ou MCP) : EPIDEMIC_SRC = r'<dossier des WAV>' ; exec(open(r'<repo>/Tools/setup_epidemic_sfx.py', encoding='utf-8').read())
import os
import unreal

eal = unreal.EditorAssetLibrary
at = unreal.AssetToolsHelpers.get_asset_tools()
DEST = '/Game/RESHOOT/Audio/SFX'
SRC = globals().get('EPIDEMIC_SRC')
tasks = []
for f in sorted(os.listdir(SRC)):
    if f.endswith('.wav'):
        t = unreal.AssetImportTask()
        t.filename = os.path.join(SRC, f); t.destination_path = DEST; t.destination_name = f[:-4]
        t.automated = True; t.replace_existing = True; t.save = True
        tasks.append(t)
at.import_asset_tasks(tasks)
def snd(name):
    return unreal.load_asset(f'{DEST}/{name}_ES')

def set_bp(path, values):
    bp = unreal.load_asset(path)
    cdo = unreal.get_default_object(bp.generated_class())
    for k, v in values.items():
        cdo.set_editor_property(k, v)
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    eal.save_loaded_asset(bp)
    return bp

def child_bp(name, parent):
    path = f'/Game/RESHOOT/Medieval/Blueprints/{name}'
    if eal.does_asset_exist(path):
        return unreal.load_asset(path)
    f = unreal.BlueprintFactory(); f.set_editor_property('parent_class', parent)
    return at.create_asset(name, '/Game/RESHOOT/Medieval/Blueprints', unreal.Blueprint, f)

N = '/Game/RESHOOT/Noir/Blueprints/'
set_bp(N + 'BP_NoirRevolver', {'fire_sound': snd('SW_Noir_Revolver_Fire'), 'empty_sound': snd('SW_Noir_Revolver_Empty'), 'reload_sound': snd('SW_Noir_Revolver_Reload')})
set_bp(N + 'BP_NoirGangster', {'fire_sound': snd('SW_Noir_Gangster_Fire')})
set_bp(N + 'BP_NoirBlackoutEvent', {'power_down_sound': snd('SW_Noir_PowerDown'), 'power_up_sound': snd('SW_Noir_PowerUp')})

set_bp('/Game/RESHOOT/Medieval/BP_MedievalSword', {'swing_sound': snd('SW_Med_Sword_Swing'), 'sword_hit_sound': snd('SW_Med_Sword_Hit'),
                                                   'shield_up_sound': snd('SW_Med_Shield_Up'), 'bow_release_sound': snd('SW_Med_Bow_Release')})
arrow = child_bp('BP_MedievalArrow', unreal.MedievalArrow)
set_bp(arrow.get_path_name().split('.')[0], {'impact_sound': snd('SW_Med_Arrow_Impact')})
knight = child_bp('BP_MedievalKnight', unreal.MedievalKnight)
set_bp(knight.get_path_name().split('.')[0], {'attack_sound': snd('SW_Med_Knight_Attack'), 'death_sound': snd('SW_Med_Knight_Death')})
archer = child_bp('BP_MedievalArcher', unreal.MedievalArcher)
set_bp(archer.get_path_name().split('.')[0], {'fire_sound': snd('SW_Med_Archer_Fire'), 'arrow_class': arrow.generated_class()})
da = unreal.load_asset('/Game/RESHOOT/Medieval/DA_Medieval')
da.set_editor_property('enemy_classes', [knight.generated_class(), archer.generated_class()])
eal.save_loaded_asset(da)
unreal.log_warning('[EPIDEMIC SFX] %d sons importés, Noir et Médiéval branchés' % len(tasks))
