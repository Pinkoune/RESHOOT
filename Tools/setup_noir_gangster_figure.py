# RESHOOT — gangsters du film noir en corps animés (mannequin UE livré avec le moteur, pistolet, chapeau).
# 1. Déplace le mannequin copié depuis les modèles du moteur (/Game/Characters/Mannequins) dans /Game/RESHOOT/Noir/Mannequin.
# 2. Renseigne BP_NoirGangster : squelette, pistolet, animations (arrêt pistolet levé, course, tir, morts).
# À lancer dans l'éditeur (console Python), hors partie.
import unreal

eal = unreal.EditorAssetLibrary
at = unreal.AssetToolsHelpers.get_asset_tools()
SRC, DST = '/Game/Characters/Mannequins', '/Game/RESHOOT/Noir/Mannequin'

if eal.does_directory_exist(SRC) and not eal.does_directory_exist(DST):
    eal.rename_directory(SRC, DST)
    # Redirecteurs laissés à l'ancien endroit : on les résout puis on supprime le dossier vide.
    ar = unreal.AssetRegistryHelpers.get_asset_registry()
    redirs = [unreal.load_asset(str(d.package_name)) for d in ar.get_assets_by_path('/Game/Characters', recursive=True)
              if str(d.asset_class_path.asset_name) == 'ObjectRedirector']
    redirs = [r for r in redirs if r]
    if redirs:
        at.fixup_referencers(redirs)
    eal.delete_directory('/Game/Characters')
    eal.save_directory(DST)

A = DST + '/Anims'
def load(p):
    a = unreal.load_asset(p)
    if not a:
        unreal.log_warning('[GANGSTER FIGURE] introuvable : ' + p)
    return a

values = {
    'figure_mesh': load(DST + '/Meshes/SKM_Manny_Simple'),
    'gun_real': load('/Game/RESHOOT/Noir/Meshes/SM_Noir_Pistol_Real/SM_Noir_Pistol_Real/StaticMeshes/SM_Noir_Pistol_Real'),
    'gun_prop': load('/Game/RESHOOT/Noir/Meshes/SM_Noir_Pistol_Prop'),
    'idle_anim': load(A + '/Pistol/MF_Pistol_Idle_ADS'),
    'move_anim': load(A + '/Pistol/Jog/MF_Pistol_Jog_Fwd'),
    'fire_anim': load(A + '/Pistol/MM_Pistol_Fire'),
    # Les animations de mort du mannequin (Anims/Death) font disparaître le corps à l'écran dans ce projet (testé en jeu) :
    # on garde la chute théâtrale codée (bascule en arrière, chapeau qui s'envole), qui marche avec le corps animé.
    'death_from_front_anims': [],
    'death_from_back_anims': [],
    'corpse_life_span': 4.0,
}
bp = unreal.load_asset('/Game/RESHOOT/Noir/Blueprints/BP_NoirGangster')
cdo = unreal.get_default_object(bp.generated_class())
for k, v in values.items():
    cdo.set_editor_property(k, v)
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
eal.save_loaded_asset(bp)
fire = values['fire_anim']
unreal.log_warning('[GANGSTER FIGURE] ok, tir additif : %s, slots du mannequin : %s' % (
    fire.is_valid_additive() if fire and hasattr(fire, 'is_valid_additive') else '?',
    [str(m.material_slot_name) for m in values['figure_mesh'].materials] if values['figure_mesh'] else '?'))
