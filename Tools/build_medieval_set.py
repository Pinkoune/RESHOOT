# RESHOOT — plateau de l'univers médiéval (sous-niveau L_Medieval), sur le modèle de build_hangar.py.
#
# Phase carton (tournage) : décor en carton peint (Dressing_Prop) + matériel de tournage réaliste
#   (caméras, perche, projecteurs, passerelles, grille de poutres, câbles) qui disparaît au rideau.
# Phase réaliste (après le rideau) : prairie fantasy en extérieur (Dressing_Real) : château, enceinte qui
#   masque les murs du plateau, herbe 3D (Light_Foliage), buissons, rochers, chemins de terre (kit Farm Paths),
#   montagnes peintes au loin, nuages, exposition d'extérieur.
# Blockout : les volumes de jeu du médiéval (rempart, chemin de ronde, rampes, tours, couverts) gardent leur
#   collision mais sont cachés en jeu ; tout ce qu'on voit est de l'habillage, sans collision.
#
# À lancer dans l'éditeur (Python), hors partie, avec L_Stage ouvert (L_Medieval en sous-niveau). Relançable.
import math, os, random

les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
eal = unreal.EditorAssetLibrary
at = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary
if les.is_in_play_in_editor():
    raise RuntimeError('Arrête la partie (PIE) avant de lancer ce script.')

PROJECT = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
MED = '/Game/RESHOOT/Medieval'
MESHES = MED + '/Meshes'
MATS = MED + '/Materials/Dressing'
TILED = '/Game/RESHOOT/Core/Materials/M_SetDressing_Tiled'
PROP_TAG, REAL_TAG, SWITCH_TAG = 'DressingProp', 'DressingReal', 'DressingSwitch'
random.seed(1213)

# ---------------------------------------------------------------------------------------------
# Niveau : on travaille dans L_Medieval, chargé sous L_Stage
# ---------------------------------------------------------------------------------------------
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
med_level = None
for lvl in unreal.EditorLevelUtils.get_levels(world):
    if lvl.get_outermost().get_name().endswith('/L_Medieval'):
        med_level = lvl
if not med_level:
    raise RuntimeError('Ouvre L_Stage (L_Medieval doit être un de ses sous-niveaux).')
if not les.set_current_level_by_name('L_Medieval'):
    raise RuntimeError('Impossible de rendre L_Medieval courant.')

def in_medieval(a):
    return a.get_level() == med_level

# ---------------------------------------------------------------------------------------------
# Assets
# ---------------------------------------------------------------------------------------------
def load(path):
    a = unreal.load_asset(path)
    if not a:
        raise RuntimeError('Asset introuvable : ' + path)
    return a

def med_mesh(obj, variant):
    folder = {'Convoy_Wood': 'ConvoyWood'}.get(obj, obj)
    name = 'SM_Medieval_' + obj.replace('_', '') + '_' + variant
    return load(MESHES + '/' + folder + ('/Real/' if variant == 'Real' else '/') + name)

CUBE = load('/Engine/BasicShapes/Cube.Cube')
PLANE = load('/Engine/BasicShapes/Plane.Plane')
LF = '/Game/Light_Foliage'
FARM = '/Game/Fab/FREE_Farm_Paths_and_Stone_Walls_Kit'
ar = unreal.AssetRegistryHelpers.get_asset_registry()
_cache = {}
for root in (LF, FARM, '/Game/Hollywood_Vol3', '/Game/RESHOOT/Core/Meshes'):
    for d in ar.get_assets_by_path(root, recursive=True):
        if str(d.asset_class_path.asset_name) == 'StaticMesh':
            _cache[str(d.asset_name)] = str(d.package_name)
def SM(name):
    return load(_cache[name])

def tiled(name, real, tint, rough=0.8, metal=0.0, tex=None, tile=200.0):
    path = MATS + '/' + name
    m = load(path) if eal.does_asset_exist(path) else at.create_asset(name, MATS, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    m.set_editor_property('parent', load(TILED))
    mel.set_material_instance_static_switch_parameter_value(m, 'bIsReal', real)
    mel.set_material_instance_vector_parameter_value(m, 'BaseColorTint', unreal.LinearColor(*tint, 1.0))
    mel.set_material_instance_vector_parameter_value(m, 'TileSize', unreal.LinearColor(tile, tile, tile, 0.0))
    mel.set_material_instance_scalar_parameter_value(m, 'Roughness', rough)
    mel.set_material_instance_scalar_parameter_value(m, 'Metallic', metal)
    if tex:
        mel.set_material_instance_texture_parameter_value(m, 'BaseColorMap', load(tex))
    mel.update_material_instance(m)
    eal.save_loaded_asset(m)
    return m

MAT_PLYWOOD = tiled('MI_Medieval_Plywood_Prop', False, (0.50, 0.36, 0.22))
MAT_PAINTSTONE = tiled('MI_Medieval_PaintedStone_Prop', False, (0.62, 0.58, 0.50))
MAT_HAY = tiled('MI_Medieval_Hay_Prop', False, (0.80, 0.64, 0.30))
MAT_STONE = tiled('MI_Medieval_Stone_Real', True, (0.85, 0.85, 0.85), 0.9, 0.0, MESHES + '/CastleWall/Real/T_Medieval_CastleWall_Real_basecolor', 250.0)
MAT_WOOD = tiled('MI_Medieval_Wood_Real', True, (0.9, 0.85, 0.8), 0.75, 0.0, MESHES + '/Barrel/Real/T_Medieval_Barrel_Real_basecolor', 150.0)
def find_basecolor(keywords):
    """Texture de couleur (2048 de préférence) d'un pack Fab dont le chemin contient un des mots-clés."""
    best = None
    for d in ar.get_assets_by_path('/Game', recursive=True):
        if str(d.asset_class_path.asset_name) != 'Texture2D':
            continue
        path, name = str(d.package_name), str(d.asset_name).lower()
        if not any(k.lower() in path.lower() for k in keywords) or not any(k in name for k in ('basecolor', 'albedo', 'diffuse', 'color')):
            continue
        score = 2 if '/2048/' in path else (1 if '_2k' in name or '2048' in name else 0)
        if best is None or score > best[0]:
            best = (score, path)
    return best[1] if best else None
# Sols photographiés (Fab) : prairie (GrassyMeadowGround4) et terre boueuse des chemins
TEX_MEADOW = find_basecolor(['GrassyMeadowGround'])
TEX_MUD = find_basecolor(['Muddy', 'Mud_', 'MuddyGround', 'Pebbles'])
MAT_GRASS = tiled('MI_Medieval_Meadow_Real', True, (1.0, 1.0, 1.0) if TEX_MEADOW else (0.11, 0.17, 0.05), 0.95, 0.0, TEX_MEADOW, 300.0)
MAT_DIRT = tiled('MI_Medieval_Dirt_Real', True, (1.0, 1.0, 1.0) if TEX_MUD else (0.20, 0.14, 0.08), 1.0, 0.0, TEX_MUD, 250.0)
MAT_STUDIO_FLOOR = load('/Game/RESHOOT/Core/Materials/MI_Stage_Floor_Prop')

# Images de fond (montagnes sur fond noir, ciel) -> textures + matériaux (parents de Jérémy)
def import_texture(file, name):
    path = MED + '/Backdrops/' + name
    if not eal.does_asset_exist(path):
        t = unreal.AssetImportTask()
        t.set_editor_property('filename', file)
        t.set_editor_property('destination_path', MED + '/Backdrops')
        t.set_editor_property('destination_name', name)
        t.set_editor_property('automated', True)
        t.set_editor_property('replace_existing', True)
        at.import_asset_tasks([t])
    tex = load(path)
    tex.set_editor_property('max_texture_size', 4096)
    eal.save_loaded_asset(tex)
    return tex
BD_SRC = os.path.join(PROJECT, 'SourceArt', 'Medieval', 'Real', 'Backdrops')
T_MOUNT = import_texture(os.path.join(BD_SRC, 'T_Medieval_Mountains.png'), 'T_Medieval_Mountains')
import_texture(os.path.join(BD_SRC, 'T_Medieval_SkyPanorama.png'), 'T_Medieval_SkyPanorama')
def mi_simple(name, parent, params):
    path = MED + '/Backdrops/' + name
    m = load(path) if eal.does_asset_exist(path) else at.create_asset(name, MED + '/Backdrops', unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    m.set_editor_property('parent', load(parent))
    for k, v in params.items():
        if isinstance(v, float):
            mel.set_material_instance_scalar_parameter_value(m, k, v)
        else:
            mel.set_material_instance_texture_parameter_value(m, k, v)
    mel.update_material_instance(m)
    eal.save_loaded_asset(m)
    return m
# Matériau de fond opaque : le noir de l'image devient transparent (masque), le reste est affiché sans éclairage.
# (M_Space_Backdrop est additif : parfait sur le noir de l'espace, invisible sur un ciel de jour.)
BD_MASTER = MED + '/Backdrops/M_Medieval_Backdrop'
BD_VERSION = 'v2'   # v2 : seul le ciel noir (moitié haute de l'image) devient transparent, plus de trous dans la plaine
_bd = unreal.load_asset(BD_MASTER) if eal.does_asset_exist(BD_MASTER) else None
if _bd and eal.get_metadata_tag(_bd, 'ReshootVersion') != BD_VERSION:
    mel.delete_all_material_expressions(_bd)
    m = _bd
elif _bd:
    m = None
else:
    m = at.create_asset('M_Medieval_Backdrop', MED + '/Backdrops', unreal.Material, unreal.MaterialFactoryNew())
if m:
    m.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_UNLIT)
    m.set_editor_property('blend_mode', unreal.BlendMode.BLEND_MASKED)
    m.set_editor_property('two_sided', True)
    m.set_editor_property('opacity_mask_clip_value', 0.12)
    tex = mel.create_material_expression(m, unreal.MaterialExpressionTextureSampleParameter2D, -800, 0)
    tex.set_editor_property('parameter_name', 'BackdropTexture')
    tex.set_editor_property('texture', T_MOUNT)
    bright = mel.create_material_expression(m, unreal.MaterialExpressionScalarParameter, -800, 300)
    bright.set_editor_property('parameter_name', 'Brightness')
    bright.set_editor_property('default_value', 1.0)
    mul = mel.create_material_expression(m, unreal.MaterialExpressionMultiply, -400, 0)
    mel.connect_material_expressions(tex, 'RGB', mul, 'A')
    mel.connect_material_expressions(bright, '', mul, 'B')
    mel.connect_material_property(mul, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    # masque = luminance x 8 (le noir pur au-dessus des sommets disparaît) + toute la moitié basse de l'image
    # opaque (plaine, lac : leurs zones sombres ne doivent pas faire de trous)
    lum = mel.create_material_expression(m, unreal.MaterialExpressionDotProduct, -400, 250)
    col = mel.create_material_expression(m, unreal.MaterialExpressionConstant3Vector, -800, 450)
    col.set_editor_property('constant', unreal.LinearColor(2.4, 4.7, 0.9, 1.0))
    mel.connect_material_expressions(tex, 'RGB', lum, 'A')
    mel.connect_material_expressions(col, '', lum, 'B')
    uv = mel.create_material_expression(m, unreal.MaterialExpressionTextureCoordinate, -1000, 650)
    v = mel.create_material_expression(m, unreal.MaterialExpressionComponentMask, -800, 650)
    v.set_editor_property('r', False); v.set_editor_property('g', True)
    mel.connect_material_expressions(uv, '', v, '')
    half = mel.create_material_expression(m, unreal.MaterialExpressionConstant, -800, 750)
    half.set_editor_property('r', 0.45)
    sub = mel.create_material_expression(m, unreal.MaterialExpressionSubtract, -600, 650)
    mel.connect_material_expressions(v, '', sub, 'A')
    mel.connect_material_expressions(half, '', sub, 'B')
    steep = mel.create_material_expression(m, unreal.MaterialExpressionMultiply, -450, 650)
    mel.connect_material_expressions(sub, '', steep, 'A')
    k = mel.create_material_expression(m, unreal.MaterialExpressionConstant, -600, 750)
    k.set_editor_property('r', 50.0)
    mel.connect_material_expressions(k, '', steep, 'B')
    add = mel.create_material_expression(m, unreal.MaterialExpressionAdd, -250, 400)
    mel.connect_material_expressions(lum, '', add, 'A')
    mel.connect_material_expressions(steep, '', add, 'B')
    mel.connect_material_property(add, '', unreal.MaterialProperty.MP_OPACITY_MASK)
    mel.recompile_material(m)
    eal.set_metadata_tag(m, 'ReshootVersion', BD_VERSION)
    eal.save_loaded_asset(m)
MAT_MOUNT = mi_simple('MI_Medieval_Mountains', BD_MASTER, {'BackdropTexture': T_MOUNT, 'Brightness': 1.0})

# ---------------------------------------------------------------------------------------------
# Nettoyage (relançable) et blockout
# ---------------------------------------------------------------------------------------------
BLOCKOUT_PREFIX = ('Rempart', 'CheminDeRonde', 'Parapet_', 'Poteau_', 'TourG', 'TourD', 'RampeG', 'RampeD', 'Tonneau_', 'Charrette_', 'BottePaille_')
DELETE_PREFIX = ('Merlon_', 'Etai_', 'Porte', 'Banniere_')
blockout = {}
for a in list(eas.get_all_level_actors()):
    if not in_medieval(a):
        continue
    lab = a.get_actor_label()
    fp = str(a.get_folder_path() or '')
    if fp.startswith('Dressing_Prop') or fp.startswith('Dressing_Real'):
        eas.destroy_actor(a)
        continue
    if not isinstance(a, unreal.StaticMeshActor):
        continue
    if lab.startswith(DELETE_PREFIX) or '_Merlon_' in lab or '_Roue_' in lab:
        eas.destroy_actor(a)
        continue
    if lab.startswith(BLOCKOUT_PREFIX):
        a.set_folder_path('Blockout')
        a.set_actor_hidden_in_game(True)
        a.static_mesh_component.set_collision_profile_name('BlockAll')
        blockout[lab] = a

# ---------------------------------------------------------------------------------------------
# Pose
# ---------------------------------------------------------------------------------------------
count = {'prop': 0, 'real': 0}
def put(label, folder, mesh, loc, yaw=0.0, scale=(1, 1, 1), mat=None, phase='real', roll=0.0, pitch=0.0, switch=False, shadow=True):
    a = eas.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(*loc), unreal.Rotator(roll=roll, pitch=pitch, yaw=yaw))
    c = a.static_mesh_component
    c.set_static_mesh(mesh)
    if mat:
        for i in range(max(1, len(mesh.static_materials))):
            c.set_material(i, mat)
    c.set_collision_profile_name('NoCollision')
    c.set_cast_shadow(shadow)
    c.set_editor_property('mobility', unreal.ComponentMobility.MOVABLE)
    a.set_actor_scale3d(unreal.Vector(*scale))
    a.set_actor_label(label)
    tag = PROP_TAG if phase == 'prop' else REAL_TAG
    a.set_folder_path(('Dressing_Prop/' if phase == 'prop' else 'Dressing_Real/') + folder)
    tags = [unreal.Name(tag)]
    if switch:
        tags.append(unreal.Name(SWITCH_TAG))
    a.tags = tags
    count[phase] += 1
    return a

def copy_box(src, label, folder, mat, phase, grow=1.01):
    """Copie habillée d'un volume de blockout (même mesh, même transformation, légèrement plus grande)."""
    t = src.get_actor_transform()
    s = src.get_actor_scale3d()
    loc, rot = src.get_actor_location(), src.get_actor_rotation()
    return put(label, folder, src.static_mesh_component.static_mesh, (loc.x, loc.y, loc.z), rot.yaw, (s.x * grow, s.y * grow, s.z * grow), mat, phase, rot.roll, rot.pitch)

def size_of(mesh):
    b = mesh.get_bounding_box()
    return (b.max.x - b.min.x, b.max.y - b.min.y, b.max.z - b.min.z)

# ---------------------------------------------------------------------------------------------
# Château (rempart, chemin de ronde, tours, rampes) : carton et réaliste
# ---------------------------------------------------------------------------------------------
WALL_P, WALL_R = med_mesh('CastleWall', 'Prop'), med_mesh('CastleWall', 'Real')
BATT_P, BATT_R = med_mesh('Battlements', 'Prop'), med_mesh('Battlements', 'Real')
wp, wr, bp, br = size_of(WALL_P), size_of(WALL_R), size_of(BATT_P), size_of(BATT_R)
RAMPART_H = 750.0
for i, y in enumerate(range(-1830, 1831, 366)):    # 11 pans de 4 m qui se chevauchent
    # le mesh fait 400 de large sur X : tourné de 90° pour longer le rempart (axe Y)
    put('Rempart_Prop_%02d' % i, 'Chateau', WALL_P, (2390, y, 0), 90.0, (1.0, 110.0 / wp[1], RAMPART_H / wp[2]), phase='prop')
    put('Rempart_Real_%02d' % i, 'Chateau', WALL_R, (2390, y, 0), 90.0, (1.0, 110.0 / wr[1], RAMPART_H / wr[2]), phase='real')
    put('Creneaux_Real_%02d' % i, 'Chateau', BATT_R, (2390, y, RAMPART_H), 90.0, (1.0, 120.0 / br[1], 1.0), phase='real')
for side, y in (('G', -1700), ('D', 1700)):
    # tours : bloc à créneaux en carton ; pierre + couronne de créneaux en réaliste
    put('Tour%s_Prop' % side, 'Chateau', BATT_P, (2100, y, 0), 90.0, (400 / bp[0], 400 / bp[1], 650 / bp[2]), phase='prop')
    copy_box(blockout['Tour' + side], 'Tour%s_Real' % side, 'Chateau', MAT_STONE, 'real')
    for k, (dx, dy, yw) in enumerate(((-200, 0, 90.0), (200, 0, 90.0), (0, -200, 0.0), (0, 200, 0.0))):
        put('Tour%s_Creneaux_Real_%d' % (side, k), 'Chateau', BATT_R, (2100 + dx, y + dy, 650), yw, (1.0, 100.0 / br[1], 1.0), phase='real')
for lab, a in blockout.items():
    if lab == 'CheminDeRonde' or lab.startswith(('RampeG', 'RampeD', 'Poteau_')):
        copy_box(a, lab + '_Prop', 'Chateau', MAT_PLYWOOD, 'prop')
        copy_box(a, lab + '_Real', 'Chateau', MAT_WOOD, 'real')
    elif lab.startswith('Parapet_'):
        copy_box(a, lab + '_Prop', 'Chateau', MAT_PAINTSTONE, 'prop')
        copy_box(a, lab + '_Real', 'Chateau', MAT_STONE, 'real')

# bannières et torches sur la face du rempart
BAN_P, BAN_R = med_mesh('Banner', 'Prop'), med_mesh('Banner', 'Real')
TOR_P, TOR_R = med_mesh('Torch', 'Prop'), med_mesh('Torch', 'Real')
for i, y in enumerate((-1000, 1000)):
    put('Banniere_Prop_%d' % i, 'Chateau', BAN_P, (2320, y, 420), 90.0, phase='prop')
    put('Banniere_Real_%d' % i, 'Chateau', BAN_R, (2320, y, 420), 90.0, phase='real')
for i, y in enumerate((-1400, -600, 600, 1400)):
    put('Torche_Prop_%d' % i, 'Chateau', TOR_P, (2315, y, 480), -90.0, phase='prop')
    put('Torche_Real_%d' % i, 'Chateau', TOR_R, (2315, y, 480), -90.0, phase='real')

# ---------------------------------------------------------------------------------------------
# Couverts : tonneaux, charrettes, bottes de paille (carton) / rochers (réaliste)
# ---------------------------------------------------------------------------------------------
BAR_P, BAR_R = med_mesh('Barrel', 'Prop'), med_mesh('Barrel', 'Real')
CART_P, CART_R = med_mesh('Cart', 'Prop'), med_mesh('Cart', 'Real')
ROCK = SM('SM_Rock_04')
rk = size_of(ROCK)
for lab, a in blockout.items():
    loc, yaw = a.get_actor_location(), a.get_actor_rotation().yaw
    if lab.startswith('Tonneau_'):
        put(lab + '_Prop', 'Couverts', BAR_P, (loc.x, loc.y, 0), yaw, phase='prop')
        put(lab + '_Real', 'Couverts', BAR_R, (loc.x, loc.y, 0), yaw, phase='real')
    elif lab.startswith('Charrette_'):
        put(lab + '_Prop', 'Couverts', CART_P, (loc.x, loc.y, 0), yaw, phase='prop')
        put(lab + '_Real', 'Couverts', CART_R, (loc.x, loc.y, 0), yaw + 90.0, phase='real')   # le réaliste est modélisé en long sur Y
    elif lab.startswith('BottePaille_'):
        copy_box(a, lab + '_Prop', 'Couverts', MAT_HAY, 'prop')
        put(lab + '_Real', 'Couverts', ROCK, (loc.x, loc.y, 0), yaw + random.uniform(-20, 20), (170 / rk[0], 120 / rk[1], 110 / rk[2]), phase='real')

# ---------------------------------------------------------------------------------------------
# Sol : plancher de studio (carton) / prairie (réaliste)
# ---------------------------------------------------------------------------------------------
put('Sol_Studio_Prop', 'Sol', CUBE, (0, 0, 0.6), 0.0, (60.0, 60.0, 0.01), MAT_STUDIO_FLOOR, 'prop', shadow=False)
put('Sol_Prairie_Real', 'Prairie', CUBE, (0, 0, 1.2), 0.0, (66000.0, 66000.0, 0.02), MAT_GRASS, 'real', shadow=False)   # 66 km : la prairie va jusqu'aux montagnes

# ---------------------------------------------------------------------------------------------
# Matériel de tournage (carton seulement : il disparaît au rideau)
# ---------------------------------------------------------------------------------------------
def look_yaw(frm, to):
    return math.degrees(math.atan2(to[1] - frm[1], to[0] - frm[0]))
TARGET = (1200, 0)
for i, (x, y) in enumerate(((-2500, -1300), (-2500, 1300), (-1200, -2500))):
    put('Camera_%d' % i, 'Studio', SM('SM_Stage_Camera'), (x, y, 0), look_yaw((x, y), TARGET), phase='prop')
    put('Camera_%d_Sacs' % i, 'Studio', SM('SM_Stage_Sandbags'), (x - 90, y + 60, 0), 30.0 * i, phase='prop')
put('Perche', 'Studio', SM('SM_Stage_BoomMic'), (-1700, 1000, 0), look_yaw((-1700, 1000), (0, 0)), phase='prop')
for i, (x, y) in enumerate(((-2300, -2300), (-2300, 2300), (700, -2650), (700, 2650), (-600, -2700), (-600, 2700))):
    mesh = SM('SM_Production_Light_01a') if i % 2 == 0 else SM('SM_Lightbox_01a')
    put('Projecteur_%d' % i, 'Studio', mesh, (x, y, 0), look_yaw((x, y), (600, 0)), phase='prop')
    put('Projecteur_%d_Sacs' % i, 'Studio', SM('SM_Stage_Sandbags'), (x + 70, y, 0), 0.0, phase='prop')
for i, (x, y, yw) in enumerate(((-2800, -2600, 10.0), (-2650, -2650, -8.0), (-2800, 2600, 0.0), (1500, 2750, 15.0), (1500, -2750, 0.0))):
    put('FlightCase_%d' % i, 'Studio', SM('SM_Stage_FlightCase'), (x, y, 0), yw, phase='prop')
for i, (x, y) in enumerate(((-2450, -1100), (-2450, 1100), (-2100, -2350), (-500, 2600), (650, -2500))):
    put('Cables_%d' % i, 'Studio', SM('SM_Stage_Cables'), (x, y, 0), random.uniform(0, 360), phase='prop')
# passerelles le long des murs nord et sud, à 5 m
cw = SM('SM_Stage_Catwalk')
for side, y in (('N', 2850), ('S', -2850)):
    for i, x in enumerate(range(-2600, 2601, 400)):
        put('Passerelle_%s_%02d' % (side, i), 'Studio', cw, (x, y, 500), 90.0, phase='prop')
# grille de poutres au-dessus du plateau, avec des projecteurs suspendus
beam = SM('SM_Structural_Beam_01d')
lamp = SM('SM_Stage_Lights_01a')
for j, y in enumerate((-1600, 0, 1600)):
    for i, x in enumerate(range(-2400, 1900, 434)):
        put('Grille_%d_%02d' % (j, i), 'Studio', beam, (x, y, 950), 0.0, phase='prop')
        if i % 2 == 0:
            put('Grille_%d_%02d_Lampe' % (j, i), 'Studio', lamp, (x, y, 900), look_yaw((x, y), (800, 0)), phase='prop', pitch=-20.0)

# ---------------------------------------------------------------------------------------------
# Nature autour du plateau (réaliste) : les murs gris du plateau sont cachés en médiéval réaliste (tag
# MedievalHideReal dans L_Stage, voir MedievalHorseConvoyEvent.cpp) ; à leur place, la prairie continue et une forêt
# de bouleaux s'étend jusqu'aux montagnes. Pas d'enceinte : on doit se sentir en pleine nature.
# ---------------------------------------------------------------------------------------------
DOORS_X = ((-2000, -1600), (500, 900))    # portes nord et sud du plateau (entrées des figurants)
_birch = []
for d in ar.get_assets_by_path('/Game/FC_BirchTreeSample', recursive=True):
    if str(d.asset_class_path.asset_name) == 'StaticMesh':
        sm_b = unreal.load_asset(str(d.package_name))
        bb = sm_b.get_bounding_box()
        if bb.max.z - bb.min.z > 600:          # seulement les arbres (pas les plantes basses du pack)
            _birch.append(sm_b)
TREES = _birch
def outside_plateau(x, y, margin=0.0):
    return abs(x) > 3050 + margin or abs(y) > 3050 + margin
n_trees = 0
if TREES:
    # lisière : rangée irrégulière juste derrière les murs (cache le vide derrière les portes), puis forêt clairsemée au loin
    for i in range(520):
        ang = random.uniform(0, 2 * math.pi)
        rad = random.choice((random.uniform(3300, 5200), random.uniform(5200, 16000)))
        x, y = rad * math.cos(ang), rad * math.sin(ang)
        deg = math.degrees(ang)
        if 125.0 < deg < 235.0 and rad < 14000:    # grande clairière à l'ouest : vue dégagée sur les montagnes
            continue
        if not outside_plateau(x, y, 150):
            continue
        s = random.uniform(0.85, 1.35)
        put('Bouleau_%03d' % n_trees, 'Foret', random.choice(TREES), (x, y, 0), random.uniform(0, 360), (s, s, s), phase='real', switch=True, shadow=rad < 8000)
        n_trees += 1
# herbe et buissons hors du plateau, jusqu'à 70 m
OUT_GRASS = [SM('SM_Grass_0%d' % k) for k in (2, 3, 6, 8)]
n_out = 0
yy = -7000.0
while yy <= 7000.0:
    xx = -7000.0
    while xx <= 7000.0:
        gx, gy = xx + random.uniform(-120, 120), yy + random.uniform(-120, 120)
        if outside_plateau(gx, gy, 60):
            mesh = random.choice(OUT_GRASS)
            s = random.uniform(1.0, 1.7)
            put('HerbeLoin_%04d' % n_out, 'Prairie/HerbeLoin', mesh, (gx, gy, 0), random.uniform(0, 360), (s, s, s), phase='real', switch=True, shadow=False)
            n_out += 1
        xx += 300.0
    yy += 300.0

# ---------------------------------------------------------------------------------------------
# Prairie : chemins de terre, herbe 3D, fleurs, buissons, rochers
# ---------------------------------------------------------------------------------------------
PATH = SM('SM_DirtPathStraight_01')
CROSS = SM('SM_DirtPathCross_01')
for i, y in enumerate(range(-2600, 2601, 400)):          # chemin du convoi (remplace les rails)
    put('Chemin_Convoi_%02d' % i, 'Chemins', CROSS if y == 0 else PATH, (250, y, 2), 90.0, mat=MAT_DIRT, phase='real', shadow=False)
for i, x in enumerate(range(-2600, 1801, 400)):           # chemin du départ au château
    if abs(x - 250) < 300:
        continue
    put('Chemin_Chateau_%02d' % i, 'Chemins', PATH, (x, 0, 2), 0.0, mat=MAT_DIRT, phase='real', shadow=False)

covers = [(a.get_actor_location().x, a.get_actor_location().y) for lab, a in blockout.items() if lab.startswith(('Tonneau_', 'Charrette_', 'BottePaille_'))]
def free_spot(x, y):
    if abs(x - 250) < 260 or (abs(y) < 220 and x < 1900):       # chemins
        return False
    if any((x - cx) ** 2 + (y - cy) ** 2 < 170 ** 2 for cx, cy in covers):
        return False
    return True
GRASS = [SM('SM_Grass_0%d' % k) for k in range(1, 9)]
FLOWERS = SM('SM_Flowers_01')
n = 0
step = 135.0       # herbe dense : une touffe tous les 1,35 m
y = -2800.0
while y <= 2800.0:
    x = -2850.0
    while x <= 1850.0:
        gx, gy = x + random.uniform(-80, 80), y + random.uniform(-80, 80)
        if free_spot(gx, gy):
            mesh = FLOWERS if random.random() < 0.12 else random.choice(GRASS)
            s = random.uniform(0.9, 1.45)
            put('Herbe_%04d' % n, 'Prairie/Herbe', mesh, (gx, gy, 0), random.uniform(0, 360), (s, s, s), phase='real', switch=True, shadow=False)
            n += 1
        x += step
    y += step
BUSHES = [SM('SM_Bush_0%d' % k) for k in range(1, 6)] + [SM('SM_Shrub_01')]
for i in range(40):
    side = i % 3
    if side == 0:
        x, y = random.uniform(-2700, 1700), 2700 + random.uniform(-60, 80)
    elif side == 1:
        x, y = random.uniform(-2700, 1700), -2700 + random.uniform(-80, 60)
    else:
        x, y = -2700 + random.uniform(-60, 80), random.uniform(-2600, 2600)
    if not free_spot(x, y) or any(lo - 250 < x < hi + 250 for lo, hi in DOORS_X) and abs(y) > 2500:
        continue
    s = random.uniform(0.7, 1.1)
    put('Buisson_%02d' % i, 'Prairie/Buissons', random.choice(BUSHES), (x, y, 0), random.uniform(0, 360), (s, s, s), phase='real', switch=True)
ROCKS = [SM('SM_Rock_0%d' % k) for k in (1, 2, 3, 5, 8)]
for i in range(18):
    x, y = random.uniform(-2700, 1700), random.uniform(-2700, 2700)
    if not free_spot(x, y):
        continue
    s = random.uniform(0.4, 0.9)
    put('Rocher_%02d' % i, 'Prairie/Rochers', random.choice(ROCKS), (x, y, 0), random.uniform(0, 360), (s, s, s), phase='real', switch=True)

# ---------------------------------------------------------------------------------------------
# Lointain : montagnes peintes sur 4 grands panneaux, nuages, exposition d'extérieur
# ---------------------------------------------------------------------------------------------
R, W, H = 32000.0, 70000.0, 26000.0     # proportions proches de l'image (21:9), bas de l'image sous la prairie
for i, yaw in enumerate((0.0, 90.0, 180.0, 270.0)):
    c, s = math.cos(math.radians(yaw)), math.sin(math.radians(yaw))
    # le plan (100 x 100, normale +Z) est redressé par un roulis de 90° : X local reste horizontal (largeur de
    # l'image), Y local devient vertical (haut de l'image en haut), puis tourné face au centre
    put('Montagnes_%d' % i, 'Ciel', PLANE, (R * c, R * s, H * 0.5 - 9500.0), yaw - 90.0, (W / 100.0, H / 100.0, 1.0), MAT_MOUNT, 'real', roll=90.0, pitch=0.0, switch=True, shadow=False)

clouds = eas.spawn_actor_from_class(unreal.VolumetricCloud, unreal.Vector(0, 0, 0), unreal.Rotator(0, 0, 0))
clouds.set_actor_label('Nuages')
clouds.set_folder_path('Dressing_Real/Ciel')
clouds.tags = [unreal.Name(REAL_TAG), unreal.Name(SWITCH_TAG)]
count['real'] += 1

def exposure_volume(label, folder, tag, bias, bloom):
    """Volume d'exposition d'une phase : le rideau fond les volumes tagués pendant le balayage."""
    v = eas.spawn_actor_from_class(unreal.PostProcessVolume, unreal.Vector(0, 0, 300), unreal.Rotator(0, 0, 0))
    v.set_actor_label(label)
    v.set_folder_path(folder)
    v.tags = [unreal.Name(tag)]
    v.set_editor_property('unbound', True)
    v.set_editor_property('priority', 1.0)
    st = v.get_editor_property('settings')
    st.set_editor_property('override_auto_exposure_bias', True)
    st.set_editor_property('auto_exposure_bias', bias)
    st.set_editor_property('override_bloom_intensity', True)
    st.set_editor_property('bloom_intensity', bloom)
    v.set_editor_property('settings', st)
    return v
exposure_volume('PPV_Prairie', 'Dressing_Real/Lumiere', REAL_TAG, -0.3, 0.4)
exposure_volume('PPV_Studio_Medieval', 'Dressing_Prop/Lumiere', PROP_TAG, -1.2, 0.3)
count['real'] += 1
count['prop'] += 1

# le réalisateur déclenche lui-même le convoi : pas de relance automatique de test
for a in eas.get_all_level_actors():
    if in_medieval(a) and isinstance(a, unreal.MedievalHorseConvoyEvent):
        a.set_editor_property('auto_start_interval', 0.0)

les.save_all_dirty_levels()
result = {'blockout': len(blockout), 'carton': count['prop'], 'reel': count['real'], 'herbe': n, 'arbres': n_trees, 'herbe_loin': n_out}
