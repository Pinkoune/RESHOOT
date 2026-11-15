# RESHOOT — costume des gangsters du film noir : matériaux carton et réalistes sur BP_NoirGangster
# (SuitProp, FaceProp, SuitReal, HatReal, FaceReal, appliqués par ANoirGangster::OnDressingChanged).
# À lancer dans l'éditeur (console Python), hors partie.
import unreal

eal = unreal.EditorAssetLibrary
at = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary
FLAT = '/Game/RESHOOT/Core/Materials/M_SetDressing'
MD = '/Game/RESHOOT/Noir/Materials'

def mi(name, real, tint, rough):
    path = f'{MD}/{name}'
    m = unreal.load_asset(path) if eal.does_asset_exist(path) else at.create_asset(name, MD, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    m.set_editor_property('parent', unreal.load_asset(FLAT))
    mel.set_material_instance_static_switch_parameter_value(m, 'bIsReal', real)
    mel.set_material_instance_scalar_parameter_value(m, 'RoughnessScale', rough)
    mel.set_material_instance_scalar_parameter_value(m, 'MetallicScale', 0.0)
    mel.set_material_instance_vector_parameter_value(m, 'BaseColorTint', unreal.LinearColor(*tint))
    mel.update_material_instance(m); eal.save_loaded_asset(m)
    return m

values = {
    'suit_prop': unreal.load_asset(f'{MD}/MI_Noir_Painted_Prop'),                     # silhouette en carton peint
    'face_prop': mi('MI_Noir_GangsterFace_Prop', False, (0.85, 0.72, 0.55, 1), 1.0),  # visage dessiné sur carton clair
    'suit_real': mi('MI_Noir_GangsterSuit_Real', True, (0.025, 0.025, 0.03, 1), 0.65),  # costume anthracite
    'hat_real': mi('MI_Noir_GangsterHat_Real', True, (0.06, 0.055, 0.05, 1), 0.8),      # feutre gris sombre
    'face_real': mi('MI_Noir_GangsterFace_Real', True, (0.55, 0.38, 0.30, 1), 0.55),    # peau
}
bp = unreal.load_asset('/Game/RESHOOT/Noir/Blueprints/BP_NoirGangster')
cdo = unreal.get_default_object(bp.generated_class())
for k, v in values.items():
    cdo.set_editor_property(k, v)
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
eal.save_loaded_asset(bp)
unreal.log_warning('[GANGSTER LOOK] ' + str({k: cdo.get_editor_property(k).get_name() for k in values}))
