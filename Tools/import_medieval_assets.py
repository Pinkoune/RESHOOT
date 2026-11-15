# RESHOOT — import des assets médiévaux générés (Tripo, Rodin) depuis SourceArt/Medieval.
#
# Pour chaque objet (Guide de génération des assets, section Import) :
#   - import du .obj par Interchange, rotation de 90° sur X (Y en haut -> Z en haut), pas de collision ;
#   - échelle calculée pour atteindre la taille du guide, pivot ramené en bas au centre ;
#   - textures importées à part (couleur sRGB, normale, rugosité et métal en masques, 2048 max) ;
#   - matériau : instance de M_SetDressing, bIsReal décoché (carton) ou coché (réaliste).
# Rangement : Content/RESHOOT/Medieval/Meshes/<Objet>/ (carton) et <Objet>/Real/ (réaliste).
#
# À lancer dans l'éditeur (Python), hors partie. Relançable : un objet déjà importé est remplacé.
import os

eal = unreal.EditorAssetLibrary
at = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary
if unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).is_in_play_in_editor():
    raise RuntimeError('Arrête la partie (PIE) avant de lancer ce script.')

PROJECT = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
SRC = os.path.join(PROJECT, 'SourceArt', 'Medieval')
DST = '/Game/RESHOOT/Medieval/Meshes'
PIPE_SRC = '/Game/RESHOOT/Core/Import/IP_ReshootImport'
PIPE = '/Game/RESHOOT/Medieval/Import/IP_MedievalImport'
MASTER = '/Game/RESHOOT/Core/Materials/M_SetDressing'

# objet -> (taille en cm, mesure) ; mesure : 'h' hauteur, 'l' plus grand côté au sol, 'm' plus grande dimension
SIZES = {
    'CastleWall': (400, 'l'), 'Battlements': (400, 'l'), 'Barrel': (100, 'h'), 'Cart': (250, 'l'),
    'Convoy': (250, 'l'), 'Convoy_Wood': (250, 'l'), 'Banner': (250, 'h'), 'Torch': (70, 'h'),
    'Sword': (100, 'm'), 'Shield': (80, 'h'), 'Bow': (140, 'm'), 'Helmet': (30, 'h'),
}
# objet -> sous-dossier Meshes/<Dossier>
FOLDER = {'Convoy_Wood': 'ConvoyWood'}

def _pipeline():
    if not eal.does_asset_exist(PIPE):
        eal.duplicate_asset(PIPE_SRC, PIPE)
    p = unreal.load_asset(PIPE)
    p.get_editor_property('mesh_pipeline').set_editor_property('collision', False)
    mat = p.get_editor_property('material_pipeline')
    mat.set_editor_property('import_materials', False)
    return p

def _import_obj(obj, folder, name, scale, offset):
    p = _pipeline()
    p.set_editor_property('import_offset_rotation', unreal.Rotator(roll=90.0, pitch=0.0, yaw=0.0))
    p.set_editor_property('import_offset_uniform_scale', scale)
    p.set_editor_property('import_offset_translation', unreal.Vector(*offset))
    params = unreal.ImportAssetParameters()
    params.set_editor_property('is_automated', True)
    params.set_editor_property('replace_existing', True)
    params.set_editor_property('destination_name', name)
    params.set_editor_property('override_pipelines', [unreal.SoftObjectPath(PIPE)])
    src = unreal.InterchangeManager.create_source_data(obj)
    unreal.InterchangeManager.get_interchange_manager_scripted().import_asset(folder, src, params)
    path = folder + '/' + name
    if not eal.does_asset_exist(path):
        raise RuntimeError('Import raté : ' + obj)
    return unreal.load_asset(path)

def _bounds(sm):
    b = sm.get_bounding_box()
    return b.min, b.max

def _import_texture(file, folder, name, kind):
    t = unreal.AssetImportTask()
    t.set_editor_property('filename', file)
    t.set_editor_property('destination_path', folder)
    t.set_editor_property('destination_name', name)
    t.set_editor_property('automated', True)
    t.set_editor_property('replace_existing', True)
    t.set_editor_property('save', False)
    at.import_asset_tasks([t])
    tex = unreal.load_asset(folder + '/' + name)
    tex.set_editor_property('max_texture_size', 2048)
    if kind == 'normal':
        tex.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_NORMALMAP)
        tex.set_editor_property('srgb', False)
    elif kind in ('roughness', 'metallic'):
        tex.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_MASKS)
        tex.set_editor_property('srgb', False)
    eal.save_loaded_asset(tex)
    return tex

def _find_textures(src_dir):
    found = {}
    for f in os.listdir(src_dir):
        low = f.lower()
        if not low.endswith(('.png', '.jpg', '.jpeg')) or 'shaded' in low or 'texture_pbr' in low:
            continue
        for kind, keys in (('basecolor', ('basecolor', 'diffuse')), ('normal', ('normal',)), ('roughness', ('roughness',)), ('metallic', ('metallic',))):
            if any(k in low for k in keys):
                found[kind] = os.path.join(src_dir, f)
    return found

def import_object(obj_key, variant):
    """variant : 'Prop' ou 'Real'."""
    src_dir = os.path.join(SRC, variant, 'SM_Medieval_' + obj_key + ('_Real' if variant == 'Real' else ''))
    objs = [f for f in os.listdir(src_dir) if f.lower().endswith('.obj')]
    if not objs:
        return 'pas de .obj'
    obj = os.path.join(src_dir, objs[0])
    name = 'SM_Medieval_' + obj_key.replace('_', '') + '_' + variant
    folder = DST + '/' + FOLDER.get(obj_key, obj_key) + ('/Real' if variant == 'Real' else '')
    size, mode = SIZES[obj_key]

    # 1. mesure à l'échelle 1, 2. import à la bonne taille avec le pivot en bas au centre
    sm = _import_obj(obj, folder, name, 1.0, (0, 0, 0))
    mn, mx = _bounds(sm)
    ext = mx - mn
    ref = {'h': ext.z, 'l': max(ext.x, ext.y), 'm': max(ext.x, ext.y, ext.z)}[mode]
    s = size / max(ref, 1e-4)
    cx, cy = (mn.x + mx.x) * 0.5, (mn.y + mx.y) * 0.5
    sm = _import_obj(obj, folder, name, s, (-cx * s, -cy * s, -mn.z * s))

    # textures + matériau de transition
    tex = {k: _import_texture(f, folder, 'T_' + name[3:] + '_' + k, k) for k, f in _find_textures(src_dir).items()}
    mi_path = folder + '/MI_' + name[3:]
    m = unreal.load_asset(mi_path) if eal.does_asset_exist(mi_path) else at.create_asset('MI_' + name[3:], folder, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    m.set_editor_property('parent', unreal.load_asset(MASTER))
    mel.set_material_instance_static_switch_parameter_value(m, 'bIsReal', variant == 'Real')
    for k, param in (('basecolor', 'BaseColorMap'), ('normal', 'NormalMap'), ('roughness', 'RoughnessMap'), ('metallic', 'MetallicMap')):
        if k in tex:
            mel.set_material_instance_texture_parameter_value(m, param, tex[k])
    mel.update_material_instance(m)
    eal.save_loaded_asset(m)
    for i in range(len(sm.static_materials)):
        sm.set_material(i, m)
    eal.save_loaded_asset(sm)

    mn, mx = _bounds(sm)
    return {'mesh': folder + '/' + name, 'taille_cm': [round(mx.x - mn.x), round(mx.y - mn.y), round(mx.z - mn.z)],
            'min_z': round(mn.z, 1), 'textures': sorted(tex)}

def import_all():
    report = {}
    for variant in ('Prop', 'Real'):
        base = os.path.join(SRC, variant)
        for d in sorted(os.listdir(base)):
            if not d.startswith('SM_Medieval_'):
                continue
            key = d[len('SM_Medieval_'):]
            if key.endswith('_Real'):
                key = key[:-len('_Real')]
            if key not in SIZES:
                continue
            try:
                report[key + '_' + variant] = import_object(key, variant)
            except Exception as e:
                report[key + '_' + variant] = 'ERREUR : ' + str(e)
    return report
