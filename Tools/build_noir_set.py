# RESHOOT — habillage du film noir (sous-niveau L_Noir), sur le modèle de build_hangar.py.
#
# Phase carton (DressingProp) : châssis de contreplaqué peint contre les murs, cartons empilés sur les couverts,
# lampadaires et voiture en carton peint, projecteurs de studio.
# Phase réaliste (DressingReal) : ruelle de nuit. Sol en asphalte, façades de briques avec rideaux de fer et appliques,
# lampadaires allumés, voitures bâchées, poubelles, tonneaux, caisses en bois sur les couverts, brume.
# Modèles et textures : Poly Haven (CC0), importés dans Content/RESHOOT/Noir.
#
# Le plateau (L_Stage) fait 60 x 60 m, murs de 6 m (face intérieure à +-3000), portes de 4 m :
# nord et sud à x = -1800 et 700, est à y = +-2500, pas de porte à l'ouest.
# Les couverts des gangsters (Noir_Caisse_*, tag NoirCover) restent le blockout : invisibles en jeu, habillés dans les deux phases.
#
# À lancer dans l'éditeur (console Python), jamais pendant une partie :
#   exec(open(r'<repo>/Tools/build_noir_set.py', encoding='utf-8').read())
import math

les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
eal = unreal.EditorAssetLibrary
at = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary
ar = unreal.AssetRegistryHelpers.get_asset_registry()
if les.is_in_play_in_editor():
    raise RuntimeError('Arrête la partie (PIE) avant de lancer ce script.')


REAL_BIAS = -0.8      # exposition figée de la ruelle de nuit
LAMP_CD = 150.0       # lampadaires (candelas)
WALL_LAMP_CD = 60.0   # appliques
FACADE_H = 1000.0     # hauteur des façades de briques
FLAT_H = 420.0        # hauteur des châssis en carton

# =====================================================================================
# Assets
# =====================================================================================
_cache = {}
for d in ar.get_assets_by_path('/Game/RESHOOT/Noir/Meshes', recursive=True):
    if str(d.asset_class_path.asset_name) == 'StaticMesh':
        _cache[str(d.asset_name)] = str(d.package_name)
def SM(name):
    return unreal.load_asset(_cache[name])
CUBE = unreal.load_asset('/Engine/BasicShapes/Cube.Cube')

TILED = '/Game/RESHOOT/Core/Materials/M_SetDressing_Tiled'
FLAT = '/Game/RESHOOT/Core/Materials/M_SetDressing'
def mi(path, parent, real, scalars=None, vectors=None, textures=None):
    folder, name = path.rsplit('/', 1)
    m = unreal.load_asset(path) if eal.does_asset_exist(path) else at.create_asset(name, folder, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    m.set_editor_property('parent', unreal.load_asset(parent))
    mel.set_material_instance_static_switch_parameter_value(m, 'bIsReal', real)
    for k, v in (scalars or {}).items(): mel.set_material_instance_scalar_parameter_value(m, k, v)
    for k, v in (vectors or {}).items(): mel.set_material_instance_vector_parameter_value(m, k, unreal.LinearColor(*v))
    for k, v in (textures or {}).items(): mel.set_material_instance_texture_parameter_value(m, k, unreal.load_asset(v))
    mel.update_material_instance(m); eal.save_loaded_asset(m)
    return m
MD = '/Game/RESHOOT/Noir/Materials'
TX = '/Game/RESHOOT/Noir/Textures'
# Asphalte mouillé (rugosité basse) : les lampadaires s'y reflètent, comme dans tout film noir.
MAT_ASPHALT = mi(MD + '/MI_Noir_Asphalt_Real', TILED, True, {'Roughness': 0.22, 'Metallic': 0.0},
                 {'TileSize': (300, 300, 300, 1), 'BaseColorTint': (0.55, 0.55, 0.55, 1)}, {'BaseColorMap': TX + '/T_Noir_Asphalt_D'})
MAT_BRICK = mi(MD + '/MI_Noir_Brick_Real', TILED, True, {'Roughness': 0.85, 'Metallic': 0.0},
               {'TileSize': (250, 250, 250, 1), 'BaseColorTint': (0.8, 0.8, 0.8, 1)}, {'BaseColorMap': TX + '/T_Noir_Brick_D'})
MAT_FLAT = mi(MD + '/MI_Noir_Flat_Prop', TILED, False, {'Roughness': 0.9, 'Metallic': 0.0}, {'BaseColorTint': (0.30, 0.24, 0.18, 1)})
MAT_NIGHT = mi(MD + '/MI_Noir_NightSky_Real', TILED, True, {'Roughness': 1.0, 'Metallic': 0.0}, {'BaseColorTint': (0.004, 0.004, 0.005, 1)})
# Fenêtres et enseignes (matériau maître non répété, avec émissif : il faut une EmissiveMap blanche, la couleur seule ne suffit pas)
WHITE = '/Game/RESHOOT/Core/Materials/T_SetDressing_WhiteMask'
MAT_WIN_LIT = mi(MD + '/MI_Noir_WindowLit_Real', FLAT, True, {'RoughnessScale': 0.3, 'MetallicScale': 0.0},
                 {'BaseColorTint': (0.05, 0.04, 0.03, 1), 'EmissiveColor': (5.0, 3.4, 1.6, 1)}, {'EmissiveMap': WHITE})
MAT_WIN_DARK = mi(MD + '/MI_Noir_WindowDark_Real', FLAT, True, {'RoughnessScale': 0.05, 'MetallicScale': 0.0},
                  {'BaseColorTint': (0.01, 0.01, 0.012, 1), 'EmissiveColor': (0, 0, 0, 1)})
MAT_SHOP = mi(MD + '/MI_Noir_ShopWindow_Real', FLAT, True, {'RoughnessScale': 0.1, 'MetallicScale': 0.0},
              {'BaseColorTint': (0.04, 0.035, 0.03, 1), 'EmissiveColor': (2.4, 1.7, 0.9, 1)}, {'EmissiveMap': WHITE})
MAT_AWNING = mi(MD + '/MI_Noir_Awning_Real', FLAT, True, {'RoughnessScale': 0.9, 'MetallicScale': 0.0}, {'BaseColorTint': (0.05, 0.05, 0.05, 1)})
MAT_NEON = mi(MD + '/MI_Noir_Neon_Real', FLAT, True, {'RoughnessScale': 0.5, 'MetallicScale': 0.0},
              {'BaseColorTint': (0.2, 0.2, 0.2, 1), 'EmissiveColor': (18.0, 15.0, 11.0, 1)}, {'EmissiveMap': WHITE})
MAT_SIGN = mi(MD + '/MI_Noir_SignPanel_Real', FLAT, True, {'RoughnessScale': 0.6, 'MetallicScale': 0.6},
              {'BaseColorTint': (0.03, 0.03, 0.035, 1), 'EmissiveColor': (0, 0, 0, 1)})
# Immeubles et ville (refonte « met le paquet ») : briques variées, pierre des corniches, trottoirs, flaques, ciel de ville
MAT_BRICK_2 = mi(MD + '/MI_Noir_Brick2_Real', TILED, True, {'Roughness': 0.85, 'Metallic': 0.0},
                 {'TileSize': (230, 230, 230, 1), 'BaseColorTint': (0.5, 0.46, 0.44, 1)}, {'BaseColorMap': TX + '/T_Noir_Brick_D'})
MAT_BRICK_3 = mi(MD + '/MI_Noir_Brick3_Real', TILED, True, {'Roughness': 0.8, 'Metallic': 0.0},
                 {'TileSize': (270, 270, 270, 1), 'BaseColorTint': (0.68, 0.62, 0.56, 1)}, {'BaseColorMap': TX + '/T_Noir_Brick_D'})
MAT_STONE = mi(MD + '/MI_Noir_Stone_Real', TILED, True, {'Roughness': 0.7, 'Metallic': 0.0},
               {'TileSize': (210, 210, 210, 1), 'BaseColorTint': (0.42, 0.42, 0.42, 1)}, {'BaseColorMap': TX + '/T_Noir_Sidewalk_D'})
MAT_SIDEWALK = mi(MD + '/MI_Noir_Sidewalk_Real', TILED, True, {'Roughness': 0.3, 'Metallic': 0.0},
                  {'TileSize': (210, 210, 210, 1), 'BaseColorTint': (0.6, 0.6, 0.6, 1)}, {'BaseColorMap': TX + '/T_Noir_Sidewalk_D'})
MAT_FAR = mi(MD + '/MI_Noir_FarBuilding_Real', TILED, True, {'Roughness': 0.9, 'Metallic': 0.0}, {'BaseColorTint': (0.012, 0.012, 0.014, 1)})
MAT_PUDDLE = mi(MD + '/MI_Noir_Puddle_Real', FLAT, True, {'RoughnessScale': 0.02, 'MetallicScale': 0.25},
                {'BaseColorTint': (0.04, 0.04, 0.045, 1), 'EmissiveColor': (0, 0, 0, 1)})
MAT_SKY = mi(MD + '/MI_Noir_CitySky_Real', FLAT, True, {'RoughnessScale': 1.0, 'MetallicScale': 0.0},
             {'BaseColorTint': (0.0, 0.0, 0.0, 1), 'EmissiveColor': (0.05, 0.05, 0.065, 1)}, {'EmissiveMap': WHITE})
MAT_FIRE = mi(MD + '/MI_Noir_Fire_Real', FLAT, True, {'RoughnessScale': 1.0, 'MetallicScale': 0.0},
              {'BaseColorTint': (0.1, 0.05, 0.0, 1), 'EmissiveColor': (40.0, 14.0, 3.0, 1)}, {'EmissiveMap': WHITE})
MAT_CLOTH = mi(MD + '/MI_Noir_Cloth_Real', FLAT, True, {'RoughnessScale': 1.0, 'MetallicScale': 0.0}, {'BaseColorTint': (0.55, 0.55, 0.52, 1)})
MAT_WIRE = mi(MD + '/MI_Noir_Wire_Real', FLAT, True, {'RoughnessScale': 0.6, 'MetallicScale': 0.3}, {'BaseColorTint': (0.02, 0.02, 0.02, 1)})
MAT_TOWER = mi(MD + '/MI_Noir_WaterTower_Real', FLAT, True, {'RoughnessScale': 0.9, 'MetallicScale': 0.0}, {'BaseColorTint': (0.03, 0.025, 0.02, 1)})
MAT_PAINTED = mi(MD + '/MI_Noir_Painted_Prop', FLAT, False, {'RoughnessScale': 1.0, 'MetallicScale': 0.0}, {'BaseColorTint': (0.55, 0.45, 0.32, 1)})

# =====================================================================================
# Pose
# =====================================================================================
def spawn(label, mesh, loc, yaw=0.0, scale=(1, 1, 1), folder='Dressing_Real', tags=('DressingReal', 'DressingSwitch'), mat=None, shadow=True,
          pitch=0.0, roll=0.0):
    a = eas.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(*loc), unreal.Rotator(pitch=pitch, yaw=yaw, roll=roll))
    c = a.static_mesh_component
    c.set_static_mesh(mesh)
    a.set_actor_scale3d(unreal.Vector(*scale))
    if mat is not None:
        for i in range(max(1, len(mesh.static_materials))): c.set_material(i, mat)
    a.set_actor_label(label); a.set_folder_path(folder); a.tags = list(tags)
    a.set_actor_enable_collision(False); c.set_collision_profile_name('NoCollision')
    c.set_editor_property('cast_shadow', shadow)
    return a

def box(label, center, size, mat, folder, tags, shadow=True):
    return spawn(label, CUBE, center, scale=(size[0] / 100, size[1] / 100, size[2] / 100), folder=folder, tags=tags, mat=mat, shadow=shadow)

def light(label, cls, loc, rot=(0, 0, 0), intensity=1.0, radius=None, temp=None, cones=None, shadows=True, folder='Dressing_Real/Lighting', tags=('DressingReal',),
          volumetric=1.0):
    a = eas.spawn_actor_from_class(cls, unreal.Vector(*loc), unreal.Rotator(pitch=rot[0], yaw=rot[1], roll=rot[2]))
    lc = a.get_component_by_class(unreal.LightComponentBase)
    if hasattr(lc, 'set_mobility'): lc.set_mobility(unreal.ComponentMobility.MOVABLE)
    lc.set_editor_property('intensity', intensity)
    lc.set_editor_property('cast_shadows', shadows)
    lc.set_editor_property('volumetric_scattering_intensity', volumetric)
    if temp is not None:
        lc.set_editor_property('use_temperature', True); lc.set_editor_property('temperature', temp)
    if radius is not None: lc.set_editor_property('attenuation_radius', radius)
    if cones is not None:
        lc.set_editor_property('inner_cone_angle', cones[0]); lc.set_editor_property('outer_cone_angle', cones[1])
    a.set_actor_label(label); a.set_folder_path(folder); a.tags = list(tags)
    return a

def ppv(label, folder, tags, bias, **kw):
    v = eas.spawn_actor_from_class(unreal.PostProcessVolume, unreal.Vector(0, 0, 300), unreal.Rotator(0, 0, 0))
    v.set_editor_property('unbound', True)
    s = v.get_editor_property('settings')
    def put(k, val):
        s.set_editor_property('override_' + k, True); s.set_editor_property(k, val)
    put('auto_exposure_method', unreal.AutoExposureMethod.AEM_MANUAL)
    put('auto_exposure_apply_physical_camera_exposure', False)
    put('auto_exposure_bias', bias)
    for k, val in kw.items(): put(k, val)
    v.set_editor_property('settings', s)
    v.set_actor_label(label); v.set_folder_path(folder); v.tags = list(tags)
    return v

def look_rot(src, dst):
    r = unreal.MathLibrary.find_look_at_rotation(unreal.Vector(*src), unreal.Vector(*dst))
    return (r.pitch, r.yaw, 0)

# Murs : (axe le long du mur, coordonnée de la face intérieure, signe vers l'intérieur, portes (centre le long du mur))
W = 3000.0
DOOR_HALF = 200.0
DOOR_H = 400.0
WALLS = {
    'Nord': ('x', W, -1, (-1800.0, 700.0)),
    'Sud': ('x', -W, 1, (-1800.0, 700.0)),
    'Est': ('y', W, -1, (-2500.0, 2500.0)),
    'Ouest': ('y', -W, 1, ()),
}
def wall_point(wall, along, depth, z):
    """Point à `along` le long du mur, à `depth` cm de sa face intérieure (vers le plateau)."""
    axis, face, inward, _ = WALLS[wall]
    d = face + inward * depth
    return (along, d, z) if axis == 'x' else (d, along, z)
def wall_yaw(wall):
    """Lacet d'un objet dont la face avant (+Y local) regarde le plateau."""
    return {'Nord': 180.0, 'Sud': 0.0, 'Est': 90.0, 'Ouest': -90.0}[wall]
def wall_segments(wall):
    """Morceaux de mur pleins entre les portes : liste de (début, fin)."""
    doors = sorted(WALLS[wall][3])
    segs, start = [], -W
    for c in doors:
        segs.append((start, c - DOOR_HALF)); start = c + DOOR_HALF
    segs.append((start, W))
    return segs

# Zones à garder libres : portes (et 5 m devant), départ du joueur, centre
FREE = []
for wall, (axis, face, inward, doors) in WALLS.items():
    for c in doors:
        FREE.append((wall_point(wall, c, 250, 0), 350))
FREE.append(((-800.0, 0.0, 0), 300))
def is_free(p, r=0.0):
    return all(math.hypot(p[0] - c[0], p[1] - c[1]) > rad + r for c, rad in FREE)

# =====================================================================================
# Chantier
# =====================================================================================
les.load_level('/Game/RESHOOT/Noir/L_Noir', unsaved_changes_policy=unreal.LevelEditorUnsavedChangesPolicy.DISCARD)
for a in eas.get_all_level_actors():
    if str(a.get_folder_path()).startswith('Dressing') or a.get_actor_label().startswith(('Noir_Bloc_',)):
        eas.destroy_actor(a)

RD = dict(folder='Dressing_Real', tags=('DressingReal', 'DressingSwitch'))   # modèles Poly Haven : leur matériau ne lit pas le rideau
RT = dict(folder='Dressing_Real/Ruelle', tags=('DressingReal',))            # matériaux M_SetDressing : fondu tramé au passage du rideau
PD = dict(folder='Dressing_Prop', tags=('DressingProp',))
PS = dict(folder='Dressing_Prop', tags=('DressingProp', 'DressingSwitch'))

# --- Couverts : blockout invisible, habillé dans les deux phases ---
covers = []
for a in eas.get_all_level_actors():
    if 'NoirCover' in [str(t) for t in a.tags]:
        a.set_actor_hidden_in_game(True)
        covers.append(a)
crate, cbox = SM('SM_Noir_WoodCrate_Real'), SM('SM_Noir_CardboardBox_Prop')
for i, c in enumerate(covers):
    o, e = c.get_actor_bounds(False)
    sx, sy, sz = 2 * e.x, 2 * e.y, 2 * e.z
    FREE.append(((o.x, o.y, 0), max(e.x, e.y) + 40))
    # réaliste : 3 rangs de 2 caisses en bois
    for layer in range(3):
        for k in (-1, 1):
            yaw = 90 * ((layer + i) % 2)
            off = (0, k * sy / 4) if yaw == 0 else (k * sx / 4, 0)
            scl = (sx / 116, sy / 2 / 52, sz / 3 / 46) if yaw == 0 else (sy / 116, sx / 2 / 52, sz / 3 / 46)
            spawn(f'Couvert_{i}_Caisse_{layer}_{k}', crate, (o.x + off[0], o.y + off[1], layer * sz / 3), yaw=yaw, scale=scl, **RD)
    # carton : 3 rangs de 2 x 2 cartons
    for layer in range(3):
        for kx in (-1, 1):
            for ky in (-1, 1):
                spawn(f'Couvert_{i}_Carton_{layer}_{kx}_{ky}', cbox, (o.x + kx * sx / 4, o.y + ky * sy / 4, layer * sz / 3),
                      yaw=15 * kx * ky * (layer - 1), scale=(sx / 2 / 38, sy / 2 / 52, sz / 3 / 34), **PS)

# --- Sol ---
box('Ruelle_Sol', (0, 0, 0.5), (2 * W, 2 * W, 1), MAT_ASPHALT, **RT)

def sz(wall, along, perp, h):
    """Taille d'une boîte : `along` le long du mur, `perp` vers le plateau."""
    return (along, perp, h) if WALLS[wall][0] == 'x' else (perp, along, h)

# --- Immeubles (réaliste) : largeurs et hauteurs variées, corniche, bandeaux d'étage, pilastre ; châssis peints (carton) ---
BUILD_W = [650, 900, 760, 1100, 820]
BUILD_H = [1150, 1450, 950, 1700, 1250, 1050, 1550]
BUILDINGS = []          # (mur, début, fin, hauteur, numéro)
bi = 0
for wall in WALLS:
    for j, (a0, a1) in enumerate(wall_segments(wall)):
        a = a0
        while a < a1 - 1:
            wdt = min(BUILD_W[bi % len(BUILD_W)], a1 - a)
            if a1 - (a + wdt) < 350:
                wdt = a1 - a                     # pas de bout d'immeuble trop étroit
            BUILDINGS.append((wall, a, a + wdt, BUILD_H[bi % len(BUILD_H)], bi))
            a += wdt; bi += 1
        size_p = sz(wall, a1 - a0, 10, FLAT_H)
        box(f'Chassis_{wall}_{j}', wall_point(wall, (a0 + a1) / 2, 45, FLAT_H / 2), size_p, MAT_FLAT, **PD)
for wall, b0, b1, h, bi in BUILDINGS:
    mid, ln = (b0 + b1) / 2, b1 - b0
    box(f'Immeuble_{bi}', wall_point(wall, mid, 15, h / 2), sz(wall, ln, 30, h), (MAT_BRICK, MAT_BRICK_2, MAT_BRICK_3)[bi % 3], **RT)
    box(f'Immeuble_{bi}_Corniche', wall_point(wall, mid, 27, h - 15), sz(wall, ln + 10, 54, 30), MAT_STONE, **RT)
    box(f'Immeuble_{bi}_Socle', wall_point(wall, mid, 20, 25), sz(wall, ln, 40, 50), MAT_STONE, **RT)
    z = 420.0
    while z < h - 150:
        box(f'Immeuble_{bi}_Bandeau_{z:.0f}', wall_point(wall, mid, 20, z), sz(wall, ln, 40, 12), MAT_STONE, shadow=False, **RT)
        z += 280.0
    box(f'Immeuble_{bi}_Pilastre', wall_point(wall, b0 + 20, 22, h / 2), sz(wall, 40, 44, h), MAT_STONE, **RT)
for wall in WALLS:
    for c in WALLS[wall][3]:
        box(f'Linteau_{wall}_{c:.0f}', wall_point(wall, c, 15, (950 + DOOR_H) / 2), sz(wall, 2 * DOOR_HALF, 30, 950 - DOOR_H), MAT_BRICK_2, **RT)

# --- Trottoirs en dalles et bordure, le long de tous les murs ---
for wall in WALLS:
    box(f'Trottoir_{wall}', wall_point(wall, 0, 125, 2), sz(wall, 2 * W, 250, 4), MAT_SIDEWALK, shadow=False, **RT)
    box(f'Bordure_{wall}', wall_point(wall, 0, 257, 3), sz(wall, 2 * W, 14, 6), MAT_STONE, shadow=False, **RT)

# --- Rideaux de fer et appliques le long des façades ---
shutter, wlamp = SM('SM_Noir_Shutter_Real'), SM('SM_Noir_WallLamp_Real')
SHUTTERS = {'Nord': (-2600, -700, 1700, 2500), 'Sud': (-2600, -700, 1700, 2500), 'Ouest': (-2200, -900, 900, 2200), 'Est': (-1300, 0, 1300)}
WLAMPS = {'Nord': (-1200, 1300), 'Sud': (-1200, 1300), 'Ouest': (-1500, 1500), 'Est': (-1900, 1900)}
for wall, alongs in SHUTTERS.items():
    yaw = 0.0 if WALLS[wall][0] == 'x' else 90.0
    for a_ in alongs:
        spawn(f'Rideau_{wall}_{a_}', shutter, wall_point(wall, a_, 45, 0), yaw=yaw, **RD)
for wall, alongs in WLAMPS.items():
    for a_ in alongs:
        spawn(f'Applique_{wall}_{a_}', wlamp, wall_point(wall, a_, 30, 260), yaw=wall_yaw(wall) - 90, **RD)
        light(f'Applique_{wall}_{a_}_Lumiere', unreal.PointLight, wall_point(wall, a_, 90, 380), intensity=WALL_LAMP_CD, radius=1200, temp=2600, shadows=False)

# --- Vitrines éclairées au rez-de-chaussée, avec cadre en pierre et marquise, là où le mur est libre ---
FE_SPOTS = [('Nord', -500.0), ('Sud', 2050.0), ('Est', -1350.0), ('Ouest', -2300.0), ('Nord', 2350.0)]   # escaliers de secours (plus bas)
SIGN_SPOTS = [('Nord', -1500.0), ('Sud', 1250.0), ('Est', 650.0), ('Ouest', 400.0)]
def wall_busy(wall, along, gap):
    used = [a for a in SHUTTERS.get(wall, ())] + [a for a in WLAMPS.get(wall, ())] + list(WALLS[wall][3])
    used += [a for w_, a in FE_SPOTS if w_ == wall] + [a - 190 for w_, a in FE_SPOTS if w_ == wall] + [a for w_, a in SIGN_SPOTS if w_ == wall]
    return any(abs(along - a) < gap for a in used) or abs(along) > W - 250
n_shop = 0
SHOPS = []
for wall, b0, b1, h, bi in BUILDINGS:
    for along in ((b0 + b1) / 2, (b0 + b1) / 2 - 300, (b0 + b1) / 2 + 300):
        if along < b0 + 160 or along > b1 - 160 or wall_busy(wall, along, 260):
            continue
        box(f'Vitrine_{bi}_{along:.0f}_Cadre', wall_point(wall, along, 31, 140), sz(wall, 262, 4, 214), MAT_STONE, shadow=False, **RT)
        box(f'Vitrine_{bi}_{along:.0f}', wall_point(wall, along, 34, 140), sz(wall, 240, 4, 190), MAT_SHOP, shadow=False, **RT)
        box(f'Vitrine_{bi}_{along:.0f}_Marquise', wall_point(wall, along, 75, 268), sz(wall, 280, 90, 8), MAT_AWNING, **RT)
        n_shop += 1
        SHOPS.append((wall, along))
        break

# --- Fenêtres des étages : certaines allumées (jaune chaud), les autres sombres et brillantes (elles reflètent la rue) ---
SIGNS = [('Nord', -1500.0, 'HOTEL'), ('Sud', 1250.0, 'JAZZ'), ('Est', 650.0, 'BAR'), ('Ouest', 400.0, 'PAWN')]
WIN_W, WIN_H, WIN_STEP = 110.0, 160.0, 380.0
aircon = SM('SM_Noir_AirCon_Real')
n_win = 0
for wall, b0, b1, h, bi in BUILDINGS:
    a_ = b0 + 200
    k = 0
    while a_ < b1 - 160:
        if all(not (sw == wall and abs(a_ - sa) < 170) for sw, sa, _ in SIGNS):
            z, row = 520.0, 0
            while z + WIN_H / 2 < h - 90:                     # un étage tous les 2,8 m jusqu'à la corniche
                lit = (k * 7 + row * 3 + bi * 5) % 5 in (0, 2)
                box(f'Fenetre_{bi}_{k}_{row}', wall_point(wall, a_, 33, z), sz(wall, WIN_W, 6, WIN_H), MAT_WIN_LIT if lit else MAT_WIN_DARK, shadow=False, **RT)
                box(f'Fenetre_{bi}_{k}_{row}_Appui', wall_point(wall, a_, 21, z - WIN_H / 2 - 5), sz(wall, WIN_W + 24, 42, 10), MAT_STONE, shadow=False, **RT)
                box(f'Fenetre_{bi}_{k}_{row}_Linteau', wall_point(wall, a_, 18, z + WIN_H / 2 + 8), sz(wall, WIN_W + 16, 36, 16), MAT_STONE, shadow=False, **RT)
                if not lit and (k + row * 2 + bi) % 6 == 3:      # climatiseur sous une fenêtre éteinte
                    spawn(f'Clim_{bi}_{k}_{row}', aircon, wall_point(wall, a_, 52, z - WIN_H / 2 - 112), yaw=wall_yaw(wall), **RD)
                n_win += 1
                z += 280.0; row += 1
        a_ += WIN_STEP; k += 1

# --- Enseignes lumineuses en drapeau : panneau sombre, cadre néon, lettres éclairées ---
SIGN_OUT, SIGN_H, SIGN_T, SIGN_Z = 150.0, 270.0, 16.0, 470.0
for wall, along, word in SIGNS:
    axis = WALLS[wall][0]
    c = wall_point(wall, along, 30 + SIGN_OUT / 2, SIGN_Z)
    def ssz(perp, thick, h):                    # taille d'une pièce : `perp` vers le plateau, `thick` le long du mur
        return (thick, perp, h) if axis == 'x' else (perp, thick, h)
    box(f'Enseigne_{word}', c, ssz(SIGN_OUT, SIGN_T, SIGN_H), MAT_SIGN, **RT)
    box(f'Enseigne_{word}_NeonHaut', wall_point(wall, along, 30 + SIGN_OUT / 2, SIGN_Z + SIGN_H / 2 - 4), ssz(SIGN_OUT, SIGN_T + 6, 8), MAT_NEON, shadow=False, **RT)
    box(f'Enseigne_{word}_NeonBas', wall_point(wall, along, 30 + SIGN_OUT / 2, SIGN_Z - SIGN_H / 2 + 4), ssz(SIGN_OUT, SIGN_T + 6, 8), MAT_NEON, shadow=False, **RT)
    box(f'Enseigne_{word}_NeonBord', wall_point(wall, along, 30 + SIGN_OUT - 4, SIGN_Z), ssz(8, SIGN_T + 6, SIGN_H), MAT_NEON, shadow=False, **RT)
    box(f'Enseigne_{word}_Potence', wall_point(wall, along, 30 + SIGN_OUT / 2, SIGN_Z + SIGN_H / 2 + 15), ssz(SIGN_OUT + 20, 6, 6), MAT_SIGN, **RT)
    light(f'Enseigne_{word}_Lueur', unreal.PointLight, wall_point(wall, along, 30 + SIGN_OUT / 2, SIGN_Z), intensity=25.0, radius=700, temp=4500,
          shadows=False, volumetric=3.0)
    # Lettres empilées, une face de chaque côté du panneau
    for side in (-1, 1):
        try:
            off = side * (SIGN_T / 2 + 1.5)
            p = (c[0] + off, c[1], c[2]) if axis == 'x' else (c[0], c[1] + off, c[2])
            yaw = (0.0 if side > 0 else 180.0) if axis == 'x' else (90.0 if side > 0 else -90.0)
            t = eas.spawn_actor_from_class(unreal.TextRenderActor, unreal.Vector(*p), unreal.Rotator(pitch=0, yaw=yaw, roll=0))
            tr = t.text_render
            tr.set_editor_property('text', unreal.Text('\n'.join(word)))
            tr.set_editor_property('world_size', 42.0)
            tr.set_editor_property('horizontal_alignment', unreal.HorizTextAligment.EHTA_CENTER)
            tr.set_editor_property('vertical_alignment', unreal.VerticalTextAligment.EVRTA_TEXT_CENTER)
            tr.set_editor_property('text_render_color', unreal.Color(r=255, g=245, b=225, a=255))
            t.set_actor_label(f'Enseigne_{word}_Lettres_{side}'); t.set_folder_path('Dressing_Real/Ruelle'); t.tags = ['DressingReal', 'DressingSwitch']
        except Exception as e:
            unreal.log_warning(f'[NOIR SET] lettres de {word} non posées : {e}')

# --- Lampadaires (réalistes allumés, en carton peint) ---
lamp = SM('SM_Noir_StreetLamp_Real')
LAMPS = [(-2000, 1700), (-2000, -1700), (300, 2100), (300, -2100), (2300, 1100), (2300, -1100)]
for i, (x, y) in enumerate(LAMPS):
    if not is_free((x, y, 0), 60):
        continue
    spawn(f'Lampadaire_{i}', lamp, (x, y, 0), yaw=30 * i, **RD)
    # Halo visible dans la brume ; ombres sur deux lampadaires seulement (trop de lumières à ombres au même endroit sinon).
    light(f'Lampadaire_{i}_Lumiere', unreal.PointLight, (x, y, 340), intensity=LAMP_CD, radius=2200, temp=2700,
          shadows=False, volumetric=4.0)
    spawn(f'Lampadaire_{i}_Carton', lamp, (x, y, 0), yaw=30 * i, mat=MAT_PAINTED, **PD)

# --- Voitures bâchées (avec une boîte de collision invisible) ---
car = SM('SM_Noir_CoveredCar_Real')
CARS = [(-2700, -1900, 0), (2680, 1700, 0), (1500, 2670, 90), (-900, -2670, 90)]   # les deux dernières garées le long du trottoir
for i, (x, y, yaw) in enumerate(CARS):
    spawn(f'Voiture_{i}', car, (x, y, 0), yaw=yaw, **RD)
    spawn(f'Voiture_{i}_Carton', car, (x, y, 0), yaw=yaw, mat=MAT_PAINTED, **PD)
    blk = eas.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(x, y, 70), unreal.Rotator(pitch=0, yaw=yaw, roll=0))
    blk.static_mesh_component.set_static_mesh(CUBE)
    blk.set_actor_scale3d(unreal.Vector(1.8, 4.4, 1.4))
    blk.set_actor_hidden_in_game(True); blk.set_actor_label(f'Noir_Bloc_Voiture_{i}'); blk.set_folder_path('Blockout')

# --- Petits objets de rue (réaliste seulement), contre les murs, hors portes ---
can, bag, barrel, hydrant = SM('SM_Noir_TrashCan_Real'), SM('SM_Noir_TrashBag_Real'), SM('SM_Noir_Barrel_Real'), SM('SM_Noir_Hydrant_Real')
STREET = [
    (can, 'Nord', -2300, 50, 10), (can, 'Nord', -2220, 45, 60), (bag, 'Nord', -2150, 40, 20), (bag, 'Nord', -2380, 90, 80),
    (can, 'Sud', 2000, 50, 0), (bag, 'Sud', 2080, 45, 40), (bag, 'Sud', 1930, 80, 110),
    (barrel, 'Est', -800, 50, 0), (barrel, 'Est', -720, 55, 30), (barrel, 'Est', -760, 120, 70),
    (can, 'Ouest', 0, 50, 0), (bag, 'Ouest', 80, 50, 20), (bag, 'Ouest', -70, 60, 60),
    (hydrant, 'Nord', 1200, 60, 0), (hydrant, 'Sud', -1250, 60, 180),
    # Ruelle plus habitée : tas de sacs, poubelles et tonneaux entre les rideaux de fer
    (bag, 'Nord', 300, 45, 30), (bag, 'Nord', 370, 60, 140), (can, 'Nord', 230, 50, 90),
    (barrel, 'Nord', 2050, 50, 0), (barrel, 'Nord', 2120, 60, 50),
    (can, 'Sud', -300, 50, 20), (can, 'Sud', -230, 55, 100), (bag, 'Sud', -370, 45, 0), (bag, 'Sud', -160, 70, 60),
    (barrel, 'Sud', -2300, 50, 0), (bag, 'Sud', -2220, 60, 70),
    (bag, 'Est', 650, 45, 0), (bag, 'Est', 720, 60, 90), (can, 'Est', 580, 50, 30),
    (barrel, 'Ouest', -1500, 50, 0), (barrel, 'Ouest', -1430, 55, 40), (can, 'Ouest', 1500, 50, 0), (bag, 'Ouest', 1570, 50, 50),
    (hydrant, 'Ouest', -600, 60, 0), (hydrant, 'Est', 2100, 60, 0),
]
for k, (mesh, wall, along, depth, yaw) in enumerate(STREET):
    p = wall_point(wall, along, depth, 0)
    if is_free(p, 40):
        spawn(f'Rue_{k}', mesh, p, yaw=wall_yaw(wall) + yaw, **RD)

# --- Armoires électriques contre les murs ---
ubox = SM('SM_Noir_UtilityBox_Real')
for k, (wall, along) in enumerate([('Nord', 1700.0), ('Sud', -1000.0), ('Est', -2100.0), ('Ouest', 2600.0)]):
    p = wall_point(wall, along, 52, 0)
    if is_free(p, 60):
        spawn(f'Armoire_{k}', ubox, p, yaw=wall_yaw(wall), **RD)

# --- Braseros : tonneau rouillé, braises et lueur orange qui éclaire la brume ---
stove = SM('SM_Noir_BarrelStove_Real')
CYL = unreal.load_asset('/Engine/BasicShapes/Cylinder.Cylinder')
CONE = unreal.load_asset('/Engine/BasicShapes/Cone.Cone')
for k, (wall, along, depth) in enumerate([('Ouest', -650.0, 170.0), ('Nord', 1950.0, 170.0), ('Sud', 300.0, 170.0)]):
    x, y, _ = wall_point(wall, along, depth, 0)
    if not is_free((x, y, 0), 60):
        continue
    spawn(f'Brasero_{k}', stove, (x, y, 0), yaw=40.0 * k, **RD)
    spawn(f'Brasero_{k}_Braises', CYL, (x, y, 78), scale=(0.48, 0.48, 0.05), mat=MAT_FIRE, shadow=False, **RT)
    light(f'Brasero_{k}_Feu', unreal.PointLight, (x, y, 125), intensity=90.0, radius=1100, temp=1900, shadows=False, volumetric=5.0)

# --- Flaques : miroirs noirs posés sur l'asphalte ---
PUDDLES = [(-1500, 300, 160, 90), (-400, -900, 220, 120), (500, 700, 140, 80), (900, -1500, 260, 110), (-2100, -500, 180, 140),
           (1800, 2300, 200, 90), (-1100, 1800, 150, 100), (2300, -2000, 170, 120), (100, -200, 120, 70), (-2600, 1200, 140, 90),
           (1500, -400, 110, 70), (-300, 2500, 190, 80), (2600, 500, 130, 100), (-1800, -2500, 210, 90)]
for k, (x, y, rx, ry) in enumerate(PUDDLES):
    spawn(f'Flaque_{k}', CYL, (x, y, 1.3), yaw=37.0 * k, scale=(rx / 50, ry / 50, 0.004), mat=MAT_PUDDLE, shadow=False, **RT)

# --- Escaliers de secours (Poly Haven) : plateforme et garde-corps à chaque étage, escalier entre deux, échelle en bas ---
fe_plat, fe_rail = SM('SM_Noir_FE_Platform_Real'), SM('SM_Noir_FE_Railing_Real')
fe_stairs, fe_ladder = SM('SM_Noir_FE_Stairs_Real'), SM('SM_Noir_FE_Ladder_Real')
FIRE_ESCAPES = [('Nord', -500.0), ('Sud', 2050.0), ('Est', -1350.0), ('Ouest', -2300.0), ('Nord', 2350.0)]
for k, (wall, along) in enumerate(FIRE_ESCAPES):
    h = next((bh for w_, b0, b1, bh, _ in BUILDINGS if w_ == wall and b0 <= along <= b1), 950.0)
    yaw = 0.0 if WALLS[wall][0] == 'x' else 90.0
    floors = [z for z in (430.0, 710.0, 990.0, 1270.0) if z < h - 150]
    for f, ztop in enumerate(floors):
        spawn(f'Secours_{k}_Plateforme_{f}', fe_plat, wall_point(wall, along, 98, ztop - 77), yaw=yaw, **RD)
        spawn(f'Secours_{k}_GardeCorps_{f}', fe_rail, wall_point(wall, along, 98, ztop - 4), yaw=yaw, **RD)
        if f + 1 < len(floors):
            rise = floors[f + 1] - ztop
            spawn(f'Secours_{k}_Escalier_{f}', fe_stairs, wall_point(wall, along + 60, 98, ztop - 4), yaw=yaw, scale=(1, 1, rise / 401), **RD)
    if floors:
        spawn(f'Secours_{k}_Echelle', fe_ladder, wall_point(wall, along - 190, 150, 150), yaw=yaw, scale=(1, 1, (floors[0] - 150) / 647), **RD)

# --- Fils électriques et cordes à linge en travers des angles de la ruelle ---
def wire(label, p0, p1, sag, segments=8, laundry=0):
    pts = []
    for s in range(segments + 1):
        t = s / segments
        pts.append(unreal.Vector(p0[0] + (p1[0] - p0[0]) * t, p0[1] + (p1[1] - p0[1]) * t, p0[2] + (p1[2] - p0[2]) * t - sag * 4 * t * (1 - t)))
    for s in range(segments):
        a, b = pts[s], pts[s + 1]
        d = b - a
        ln = d.length()
        rot = unreal.MathLibrary.make_rot_from_z(d)
        w_ = eas.spawn_actor_from_class(unreal.StaticMeshActor, (a + b) * 0.5, rot)
        w_.static_mesh_component.set_static_mesh(CYL); w_.static_mesh_component.set_material(0, MAT_WIRE)
        w_.set_actor_scale3d(unreal.Vector(0.025, 0.025, ln / 100))
        w_.set_actor_label(f'{label}_{s}'); w_.set_folder_path('Dressing_Real/Ruelle'); w_.tags = ['DressingReal']
        w_.set_actor_enable_collision(False); w_.static_mesh_component.set_collision_profile_name('NoCollision')
        w_.static_mesh_component.set_editor_property('cast_shadow', False)
    for j in range(laundry):                                 # linge qui pend : draps et chemises
        t = (j + 1) / (laundry + 1)
        p = pts[0] + (pts[-1] - pts[0]) * t
        p.z -= sag * 4 * t * (1 - t)
        h_ = (80, 55, 95, 60, 70)[j % 5]
        spawn(f'{label}_Linge_{j}', CUBE, (p.x, p.y, p.z - h_ / 2 - 3), yaw=math.degrees(math.atan2(pts[-1].y - pts[0].y, pts[-1].x - pts[0].x)),
              scale=((70, 45, 90, 50, 60)[j % 5] / 100, 0.02, h_ / 100), mat=MAT_CLOTH, **RT)
WIRES = [
    ('Fil_NO', wall_point('Nord', -2350, 30, 980), wall_point('Ouest', 2350, 30, 960), 70, 4),
    ('Fil_NE', wall_point('Nord', 2600, 30, 1000), wall_point('Est', 2000, 30, 1010), 80, 5),
    ('Fil_SO', wall_point('Sud', -2450, 30, 900), wall_point('Ouest', -1900, 30, 920), 60, 0),
    ('Fil_SE', wall_point('Sud', 2450, 30, 1000), wall_point('Est', -2300, 30, 980), 90, 4),
    ('Fil_N', wall_point('Nord', -2900, 30, 1050), wall_point('Nord', 2900, 30, 1050), 120, 0),
    ('Fil_O', wall_point('Ouest', -2900, 30, 1080), wall_point('Ouest', 2900, 30, 1080), 140, 0),
]
for label, p0, p1, sag, laundry in WIRES:
    wire(label, p0, p1, sag, laundry=laundry)

# --- La ville au-dessus des toits : immeubles lointains, fenêtres allumées en hauteur, réservoirs d'eau ---
def water_tower(label, x, y, z):
    for lx, ly in ((-90, -90), (90, -90), (-90, 90), (90, 90)):
        box(f'{label}_Pied_{lx}_{ly}', (x + lx, y + ly, z + 125), (18, 18, 250), MAT_TOWER, **RT)
    spawn(f'{label}_Cuve', CYL, (x, y, z + 250 + 160), scale=(3.0, 3.0, 3.2), mat=MAT_TOWER, **RT)
    spawn(f'{label}_Toit', CONE, (x, y, z + 570 + 60), scale=(3.2, 3.2, 1.2), mat=MAT_TOWER, **RT)
FAR_W = [1300, 900, 1500, 1100, 1700, 1000]
FAR_H = [2200, 1750, 2450, 1900, 2050, 2350]
fk = 0
for wall in WALLS:
    a = -W - 600
    while a < W + 600:
        wdt = FAR_W[fk % len(FAR_W)]; fh = FAR_H[fk % len(FAR_H)]
        depth = -(350 + 300 * (fk % 3))                   # en retrait, à plusieurs distances
        box(f'Ville_{fk}', wall_point(wall, a + wdt / 2, depth - 300, fh / 2), sz(wall, wdt - 60, 600, fh), MAT_FAR, **RT)
        zz = 1200.0
        while zz < fh - 150:                               # quelques fenêtres allumées, visibles au-dessus des toits proches
            aa = a + 150
            while aa < a + wdt - 150:
                if (int(aa / 10) + int(zz) // 7 + fk * 13) % 7 == 0:
                    box(f'Ville_{fk}_F_{aa:.0f}_{zz:.0f}', wall_point(wall, aa, depth + 2, zz), sz(wall, 70, 4, 100), MAT_WIN_LIT, shadow=False, **RT)
                aa += 230
            zz += 260
        if fk % 3 == 1:
            xw, yw, _ = wall_point(wall, a + wdt / 2, depth - 300, 0)
            water_tower(f'Ville_{fk}_Reservoir', xw, yw, fh)
        a += wdt; fk += 1
for wall, b0, b1, h, bi in BUILDINGS:                       # quelques réservoirs sur les toits proches aussi
    if bi % 5 == 2:
        xw, yw, _ = wall_point(wall, (b0 + b1) / 2, -170, 0)
        water_tower(f'Toit_{bi}_Reservoir', xw, yw, h)

# =====================================================================================
# Deuxième passe d'accessoires : une rue habitée
# (terrasse du club de jazz, tonneaux du bar, livraison, bric-à-brac dans les angles, bennes, bancs, voitures garées,
#  parcmètres, boîtes aux lettres, plaques d'égout, affiches sur les murs, journaux par terre)
# =====================================================================================
import random
rng = random.Random(1947)
SPHERE = unreal.load_asset('/Engine/BasicShapes/Sphere.Sphere')
MAT_METAL = mi(MD + '/MI_Noir_StreetMetal_Real', FLAT, True, {'RoughnessScale': 0.45, 'MetallicScale': 0.8}, {'BaseColorTint': (0.06, 0.06, 0.065, 1)})
MAT_DUMP = mi(MD + '/MI_Noir_Dumpster_Real', FLAT, True, {'RoughnessScale': 0.6, 'MetallicScale': 0.5}, {'BaseColorTint': (0.03, 0.045, 0.035, 1)})
MAT_MAILBOX = mi(MD + '/MI_Noir_Mailbox_Real', FLAT, True, {'RoughnessScale': 0.5, 'MetallicScale': 0.4}, {'BaseColorTint': (0.04, 0.06, 0.11, 1)})
MAT_POSTER = mi(MD + '/MI_Noir_Poster_Real', FLAT, True, {'RoughnessScale': 0.95, 'MetallicScale': 0.0}, {'BaseColorTint': (0.36, 0.345, 0.3, 1)})
MAT_POSTER_2 = mi(MD + '/MI_Noir_Poster2_Real', FLAT, True, {'RoughnessScale': 0.95, 'MetallicScale': 0.0}, {'BaseColorTint': (0.2, 0.19, 0.17, 1)})
MAT_PAPER = mi(MD + '/MI_Noir_Paper_Real', FLAT, True, {'RoughnessScale': 0.9, 'MetallicScale': 0.0}, {'BaseColorTint': (0.48, 0.48, 0.46, 1)})

AC = dict(folder='Dressing_Real/Accessoires', tags=('DressingReal', 'DressingSwitch'))   # modèles Poly Haven
AT = dict(folder='Dressing_Real/Accessoires', tags=('DressingReal',))                   # pièces en M_SetDressing
cafe, chalk, plant, bench = SM('SM_Noir_CafeSet_Real'), SM('SM_Noir_Chalkboard_Real'), SM('SM_Noir_Plant_Real'), SM('SM_Noir_Bench_Real')
tyre, ladder, truck, suitcase = SM('SM_Noir_Tyre_Real'), SM('SM_Noir_Ladder_Real'), SM('SM_Noir_HandTruck_Real'), SM('SM_Noir_Suitcase_Real')
jerry, bucket, sack, planter = SM('SM_Noir_Jerrycan_Real'), SM('SM_Noir_Bucket_Real'), SM('SM_Noir_Sack_Real'), SM('SM_Noir_Planter_Real')
scrate, wbarrel, bottle = SM('SM_Noir_SmallCrate_Real'), SM('SM_Noir_WineBarrel_Real'), SM('SM_Noir_Bottle_Real')

# Ce qui occupe déjà le sol (x, y, rayon) : on ne pose rien dessus
OCC = []
def occ_add(x, y, r):
    OCC.append((x, y, r))
def occ_free(x, y, r):
    return is_free((x, y, 0), r) and all(math.hypot(x - ox, y - oy) > r + orr for ox, oy, orr in OCC)
bb = shutter.get_bounding_box()
sh_w, sh_d = bb.max.x - bb.min.x, bb.max.y - bb.min.y
for wall, alongs in SHUTTERS.items():
    for a_ in alongs:
        s = -sh_w / 2
        while s <= sh_w / 2:
            occ_add(*wall_point(wall, a_ + s, 45 + sh_d / 2, 0)[:2], sh_d / 2 + 5); s += 40
for mesh, wall, along, depth, yaw in STREET:
    occ_add(*wall_point(wall, along, depth, 0)[:2], 35)
for wall, along in [('Nord', 1700.0), ('Sud', -1000.0), ('Est', -2100.0), ('Ouest', 2600.0)]:
    occ_add(*wall_point(wall, along, 52, 0)[:2], 60)
for wall, along, depth in [('Ouest', -650.0, 170.0), ('Nord', 1950.0, 170.0), ('Sud', 300.0, 170.0)]:
    occ_add(*wall_point(wall, along, depth, 0)[:2], 55)
for wall, along in FIRE_ESCAPES:
    occ_add(*wall_point(wall, along - 190, 150, 0)[:2], 45)
for x, y in LAMPS:
    occ_add(x, y, 40)
for x, y, yaw in CARS:
    for t in (-150, 0, 150):
        occ_add(x + (t if yaw == 90 else 0), y + (0 if yaw == 90 else t), 110)

skipped = []
def settle(a, x, y, z):
    """Recale un objet tourné : centre de sa boîte en (x, y), dessous à z."""
    o, e = a.get_actor_bounds(False)
    a.set_actor_location(a.get_actor_location() + unreal.Vector(x - o.x, y - o.y, z - (o.z - e.z)), False, False)
def put(label, mesh, x, y, z=0.0, yaw=0.0, r=40.0, pitch=0.0, roll=0.0, scale=(1, 1, 1), carton=False, check=True):
    """Pose un modèle au sol si la place est libre ; carton=True ajoute sa version peinte pour la répétition."""
    if check and not occ_free(x, y, r):
        skipped.append(label); return None
    a = spawn(label, mesh, (x, y, z), yaw=yaw, pitch=pitch, roll=roll, scale=scale, **AC)
    if pitch or roll: settle(a, x, y, z)
    if carton:
        p = spawn(label + '_Carton', mesh, (x, y, z), yaw=yaw, pitch=pitch, roll=roll, scale=scale, mat=MAT_PAINTED, **PD)
        if pitch or roll: settle(p, x, y, z)
    if check: occ_add(x, y, r * 0.5)            # nos propres groupes peuvent se serrer
    return a
def at_wall(wall, along, depth):
    return wall_point(wall, along, depth, 0)[:2]
def unit_y(yaw):
    return (-math.sin(math.radians(yaw)), math.cos(math.radians(yaw)))

def tyre_stack(label, x, y, n):
    if not occ_free(x, y, 38): skipped.append(label); return
    for i in range(n):
        put(f'{label}_{i}', tyre, x + rng.uniform(-4, 4), y + rng.uniform(-4, 4), z=i * 16.0, yaw=rng.uniform(0, 360), roll=90.0, check=False)
    occ_add(x, y, 19)
def crate_stack(label, x, y, yaw, rows=(2, 2, 1)):
    if not occ_free(x, y, 55): skipped.append(label); return
    uy = unit_y(yaw)
    for layer, n in enumerate(rows):
        for k in range(n):
            off = (k - (n - 1) / 2) * 42.0
            put(f'{label}_{layer}_{k}', scrate, x + uy[0] * off, y + uy[1] * off, z=layer * 35.0, yaw=yaw + rng.uniform(-6, 6), check=False)
    occ_add(x, y, 28)
def sack_pile(label, x, y, yaw, layers=3):
    if not occ_free(x, y, 50): skipped.append(label); return
    for layer in range(layers):
        for k in (-1, 1) if layer < layers - 1 else (0,):
            ux = (math.cos(math.radians(yaw)), math.sin(math.radians(yaw)))
            put(f'{label}_{layer}_{k}', sack, x + ux[0] * k * 25, y + ux[1] * k * 25, z=layer * 16.0, yaw=yaw + rng.uniform(-10, 10), check=False)
    occ_add(x, y, 25)

# --- Terrasse du club de jazz (JAZZ, mur sud) : tables pliantes, ardoise, plantes, lumière chaude ---
for k, along in enumerate((1180.0, 1440.0)):
    put(f'Terrasse_Table_{k}', cafe, *at_wall('Sud', along, 150), yaw=wall_yaw('Sud') + 90 + rng.uniform(-8, 8), r=95, carton=True)
put('Terrasse_Ardoise', chalk, *at_wall('Sud', 1300, 245), yaw=wall_yaw('Sud') + 25, r=50, carton=True)
for k, along in enumerate((1110.0, 1530.0)):
    put(f'Terrasse_Plante_{k}', plant, *at_wall('Sud', along, 45), yaw=rng.uniform(0, 360), r=35)
light('Terrasse_Lumiere', unreal.PointLight, wall_point('Sud', 1360, 160, 250), intensity=35.0, radius=750, temp=2400, shadows=False, volumetric=2.0)

# --- Devant le bar (BAR, mur est) : tonneaux de vin en mange-debout, bouteilles dessus ---
for k, along in enumerate((430.0, 880.0)):
    x, y = at_wall('Est', along, 200)
    if put(f'Bar_Tonneau_{k}', wbarrel, x, y, yaw=rng.uniform(0, 360), r=45, carton=True):
        for j in range(2):
            put(f'Bar_Tonneau_{k}_Bouteille_{j}', bottle, x + rng.uniform(-15, 15), y + rng.uniform(-15, 15), z=87.0, check=False)
x, y = at_wall('Est', 1000, 300)
put('Bar_Bouteille_Couchee', bottle, x, y, yaw=rng.uniform(0, 360), roll=90.0, r=10)

# --- Livraison devant l'hôtel (mur nord) : caisses empilées, diable, sacs ---
crate_stack('Livraison_Caisses', *at_wall('Nord', -1020, 60), yaw=wall_yaw('Nord') + 90)
put('Livraison_Diable', truck, *at_wall('Nord', -880, 95), yaw=wall_yaw('Nord') + 160, r=40)
sack_pile('Livraison_Sacs', *at_wall('Nord', -1260, 60), yaw=wall_yaw('Nord'))

# --- Bric-à-brac dans les angles : pneus, bidons, seau, échelle, caisses, valises, sacs ---
tyre_stack('Angle_NO_Pneus_A', -2900, 2900, 5)
tyre_stack('Angle_NO_Pneus_B', -2825, 2905, 3)
put('Angle_NO_Bidon_A', jerry, -2760, 2935, yaw=rng.uniform(0, 360), r=22)
put('Angle_NO_Bidon_B', jerry, -2935, 2700, yaw=rng.uniform(0, 360), r=22)
put('Angle_NO_Seau', bucket, -2930, 2360, yaw=rng.uniform(0, 360), r=22)
put('Angle_NO_Echelle', ladder, -2890, 2770, yaw=90.0 + rng.uniform(-8, 8), r=55)
crate_stack('Angle_NO_Caisses', -2905, 2430, 90.0, rows=(2, 1))
tyre_stack('Angle_SO_Pneus_A', -2900, -2900, 4)
tyre_stack('Angle_SO_Pneus_B', -2830, -2905, 2)
put('Angle_SO_Valise_A', suitcase, -2930, -2620, yaw=rng.uniform(80, 100), r=40)
put('Angle_SO_Valise_B', suitcase, -2700, -2880, yaw=rng.uniform(80, 100), roll=90.0, r=35)
sack_pile('Angle_SO_Sacs', -2905, -2740, 90.0, layers=2)
put('Angle_SO_Caisse', scrate, -2765, -2935, yaw=rng.uniform(-8, 8), r=45)
tyre_stack('Angle_NE_Pneus', 2905, 2905, 3)
put('Angle_NE_Bidon', jerry, 2840, 2935, yaw=rng.uniform(0, 360), r=22)
put('Angle_NE_Seau', bucket, 2925, 2830, yaw=rng.uniform(0, 360), r=22)
crate_stack('Angle_SE_Caisses', 2900, -2915, 0.0, rows=(2, 1))
put('Angle_SE_Pneu', tyre, 2785, -2925, yaw=rng.uniform(0, 360), roll=90.0, r=32)
put('Angle_SE_Bidon', jerry, 2720, -2940, yaw=rng.uniform(0, 360), r=22)

# --- Bennes à ordures (deux phases : peinte en carton), boîte de collision invisible ---
def dumpster(k, wall, along, depth):
    x, y = at_wall(wall, along, depth)
    if not occ_free(x, y, 110): skipped.append(f'Benne_{k}'); return
    occ_add(x, y, 110)
    for kw, mat in ((AT, MAT_DUMP), (PD, MAT_PAINTED)):
        sfx = '' if kw is AT else '_Carton'
        box(f'Benne_{k}{sfx}', (x, y, 70), sz(wall, 190, 110, 105), mat, kw['folder'], kw['tags'])
        box(f'Benne_{k}_Couvercle{sfx}', (x, y, 126), sz(wall, 198, 118, 6), mat, kw['folder'], kw['tags'])
        for s in (-1, 1):
            box(f'Benne_{k}_Renfort_{s}{sfx}', wall_point(wall, along + s * 55, depth + 57, 70), sz(wall, 8, 6, 100), mat, kw['folder'], kw['tags'])
    for sa in (-80, 80):
        for sp in (-40, 40):
            p = wall_point(wall, along + sa, depth + sp, 9)
            box(f'Benne_{k}_Roue_{sa}_{sp}', p, (16, 16, 16), MAT_METAL, **AT)
            box(f'Benne_{k}_Roue_{sa}_{sp}_Carton', p, (16, 16, 16), MAT_PAINTED, **PD)
    blk = eas.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(x, y, 65), unreal.Rotator(0, 0, 0))
    blk.static_mesh_component.set_static_mesh(CUBE)
    s3 = sz(wall, 198, 118, 130)
    blk.set_actor_scale3d(unreal.Vector(s3[0] / 100, s3[1] / 100, s3[2] / 100))
    blk.set_actor_hidden_in_game(True); blk.set_actor_label(f'Noir_Bloc_Benne_{k}'); blk.set_folder_path('Blockout')
    for j, (da, dd) in enumerate(((120, -20), (-125, 10), (40, 95))):
        put(f'Benne_{k}_Sac_{j}', bag, *at_wall(wall, along + da, depth + dd), yaw=rng.uniform(0, 360), r=25, check=False)
dumpster(0, 'Ouest', 1200, 95)
dumpster(1, 'Est', -300, 95)

# --- Bancs contre les façades ---
for k, (wall, along) in enumerate([('Ouest', -1150.0), ('Sud', -470.0), ('Nord', 1500.0), ('Est', 1000.0)]):
    put(f'Banc_{k}', bench, *at_wall(wall, along, 55), yaw=wall_yaw(wall) + 180, r=60, carton=True)

# --- Plantes et jardinières devant une vitrine sur deux ---
for k, (wall, along) in enumerate(SHOPS):
    if k % 2 == 0:
        for s in (-1, 1):
            put(f'Vitrine_Plante_{k}_{s}', plant, *at_wall(wall, along + s * 100, 95), yaw=rng.uniform(0, 360), r=35)
    else:
        put(f'Vitrine_Jardiniere_{k}', planter, *at_wall(wall, along, 52), yaw=wall_yaw(wall), r=48)

# --- Parcmètres le long des trottoirs nord et sud ---
def meter(label, x, y):
    spawn(f'{label}_Pied', CYL, (x, y, 55), scale=(0.06, 0.06, 1.1), mat=MAT_METAL, **AT)
    box(f'{label}_Tete', (x, y, 124), (16, 13, 28), MAT_METAL, **AT)
    spawn(f'{label}_Dome', SPHERE, (x, y, 139), scale=(0.15, 0.13, 0.12), mat=MAT_METAL, **AT)
for wall in ('Nord', 'Sud'):
    along = -2650.0
    while along < 2800:
        x, y = at_wall(wall, along, 238)
        if occ_free(x, y, 25):
            meter(f'Parcmetre_{wall}_{along:.0f}', x, y); occ_add(x, y, 25)
        along += 640.0

# --- Boîtes aux lettres ---
def mailbox(label, wall, along, depth):
    x, y = at_wall(wall, along, depth)
    if not occ_free(x, y, 40): skipped.append(label); return
    occ_add(x, y, 40)
    for sa in (-18, 18):
        for sp in (-16, 16):
            box(f'{label}_Pied_{sa}_{sp}', wall_point(wall, along + sa, depth + sp, 15), (6, 6, 30), MAT_MAILBOX, **AT)
    box(label, (x, y, 65), sz(wall, 50, 46, 70), MAT_MAILBOX, **AT)
    rot = dict(roll=90.0) if WALLS[wall][0] == 'x' else dict(pitch=90.0)
    spawn(f'{label}_Dessus', CYL, (x, y, 100), scale=(0.5, 0.5, 0.46), mat=MAT_MAILBOX, **AT, **rot)
mailbox('BoiteAuxLettres_0', 'Ouest', 250, 55)
mailbox('BoiteAuxLettres_1', 'Nord', 1080, 60)

# --- Plaques d'égout ---
for k, (x, y) in enumerate([(-300, 1200), (1200, -800), (-1500, -1300), (2000, 600), (300, -2300)]):
    spawn(f'Egout_{k}', CYL, (x, y, 1.1), scale=(0.72, 0.72, 0.012), mat=MAT_METAL, shadow=False, **AT)

# --- Affiches collées sur les façades, avec un titre pour certaines ---
POSTER_WORDS = ['WANTED', 'REWARD\n$500', 'VOTE\nMALONE', 'BOXING\nFRIDAY', 'MISSING', 'JAZZ\nTONIGHT', 'FOR\nRENT', 'NO\nLOITERING']
TEXT_YAW = {'Nord': -90.0, 'Sud': 90.0, 'Est': 180.0, 'Ouest': 0.0}   # le texte regarde vers la rue
n_post = 0
for wall, b0, b1, h, bi in BUILDINGS:
    along = b0 + 110
    while along < b1 - 60:
        busy = wall_busy(wall, along, 215) or any(sw == wall and abs(along - sa) < 175 for sw, sa in SHOPS)
        if not busy and rng.random() < 0.85:
            for j in range(rng.choice((1, 2, 2, 3))):        # affiches qui se chevauchent
                a2 = along + j * rng.uniform(40, 60) - 30
                pw, ph = rng.choice(((70, 100), (60, 85), (80, 110)))
                zc = 175 + rng.uniform(-15, 20)
                box(f'Affiche_{n_post}', wall_point(wall, a2, 31 + 0.4 * j, zc), sz(wall, pw, 1, ph),
                    MAT_POSTER if rng.random() < 0.65 else MAT_POSTER_2, shadow=False, **AT)
                if rng.random() < 0.6:
                    try:
                        p = wall_point(wall, a2, 33 + 0.4 * j, zc + ph * 0.18)
                        t = eas.spawn_actor_from_class(unreal.TextRenderActor, unreal.Vector(*p), unreal.Rotator(pitch=0, yaw=TEXT_YAW[wall], roll=0))
                        tr = t.text_render
                        tr.set_editor_property('text', unreal.Text(rng.choice(POSTER_WORDS)))
                        tr.set_editor_property('world_size', pw * 0.2)
                        tr.set_editor_property('horizontal_alignment', unreal.HorizTextAligment.EHTA_CENTER)
                        tr.set_editor_property('vertical_alignment', unreal.VerticalTextAligment.EVRTA_TEXT_CENTER)
                        tr.set_editor_property('text_render_color', unreal.Color(r=18, g=18, b=18, a=255))
                        t.set_actor_label(f'Affiche_{n_post}_Titre'); t.set_folder_path('Dressing_Real/Accessoires'); t.tags = ['DressingReal', 'DressingSwitch']
                    except Exception as e:
                        unreal.log_warning(f'[NOIR SET] titre d\'affiche non posé : {e}')
                n_post += 1
        along += rng.uniform(300, 420)

# --- Journaux et papiers éparpillés (surtout le long des trottoirs) ---
for k in range(90):
    if k % 3:
        wall = rng.choice(list(WALLS))
        x, y = at_wall(wall, rng.uniform(-2900, 2900), rng.uniform(40, 450))
    else:
        x, y = rng.uniform(-2850, 2850), rng.uniform(-2850, 2850)
    zb = 4.4 if max(abs(x), abs(y)) > W - 252 else 1.6           # sur le trottoir (4 cm) ou sur l'asphalte
    spawn(f'Journal_{k}', CUBE, (x, y, zb + (k % 4) * 0.15), yaw=rng.uniform(0, 360),
          scale=(rng.choice((0.42, 0.3, 0.36, 0.22)), rng.choice((0.3, 0.24, 0.28, 0.18)), 0.004),
          mat=MAT_PAPER if k % 4 else MAT_POSTER, shadow=False, **AT)

# =====================================================================================
# Troisième passe : scène de crime, cabines téléphoniques, kiosque à journaux, lampadaires et panneaux en plus
# =====================================================================================
MAT_CHALK = mi(MD + '/MI_Noir_Chalk_Real', FLAT, True, {'RoughnessScale': 1.0, 'MetallicScale': 0.0},
               {'BaseColorTint': (0.85, 0.85, 0.82, 1), 'EmissiveColor': (0.15, 0.15, 0.15, 1)}, {'EmissiveMap': WHITE})
MAT_TAPE = mi(MD + '/MI_Noir_Tape_Real', FLAT, True, {'RoughnessScale': 0.5, 'MetallicScale': 0.0}, {'BaseColorTint': (0.75, 0.6, 0.05, 1)})
MAT_STRIPE_D = mi(MD + '/MI_Noir_StripeDark_Real', FLAT, True, {'RoughnessScale': 0.7, 'MetallicScale': 0.0}, {'BaseColorTint': (0.02, 0.02, 0.02, 1)})
MAT_STRIPE_L = mi(MD + '/MI_Noir_StripeLight_Real', FLAT, True, {'RoughnessScale': 0.7, 'MetallicScale': 0.0}, {'BaseColorTint': (0.7, 0.7, 0.68, 1)})
MAT_BOOTH = mi(MD + '/MI_Noir_Booth_Real', FLAT, True, {'RoughnessScale': 0.45, 'MetallicScale': 0.3}, {'BaseColorTint': (0.08, 0.06, 0.04, 1)})
MAT_GLASS = mi(MD + '/MI_Noir_BoothGlass_Real', FLAT, True, {'RoughnessScale': 0.05, 'MetallicScale': 0.2},
               {'BaseColorTint': (0.02, 0.02, 0.025, 1), 'EmissiveColor': (0.6, 0.5, 0.3, 1)}, {'EmissiveMap': WHITE})
AT3 = dict(folder='Dressing_Real/Accessoires3', tags=('DressingReal',))

def text(label, loc, yaw, words, size, color=(240, 235, 220)):
    try:
        t = eas.spawn_actor_from_class(unreal.TextRenderActor, unreal.Vector(*loc), unreal.Rotator(pitch=0, yaw=yaw, roll=0))
        tr = t.text_render
        tr.set_editor_property('text', unreal.Text(words)); tr.set_editor_property('world_size', size)
        tr.set_editor_property('horizontal_alignment', unreal.HorizTextAligment.EHTA_CENTER)
        tr.set_editor_property('vertical_alignment', unreal.VerticalTextAligment.EVRTA_TEXT_CENTER)
        tr.set_editor_property('text_render_color', unreal.Color(r=color[0], g=color[1], b=color[2], a=255))
        t.set_actor_label(label); t.set_folder_path('Dressing_Real/Accessoires3'); t.tags = ['DressingReal', 'DressingSwitch']
        return t
    except Exception as e:
        unreal.log_warning(f'[NOIR SET] texte {label} non posé : {e}')

def seg(label, a, b, z, width, height, mat):
    """Barre horizontale de a à b (x, y)."""
    dx, dy = b[0] - a[0], b[1] - a[1]
    ln = math.hypot(dx, dy)
    spawn(label, CUBE, ((a[0] + b[0]) / 2, (a[1] + b[1]) / 2, z), yaw=math.degrees(math.atan2(dy, dx)),
          scale=(ln / 100, width / 100, height / 100), mat=mat, shadow=False, **AT3)

# --- Scène de crime : silhouette à la craie, chevalets de police, ruban, chapeau et arme du mort, marqueurs ---
CS = (-1650.0, 950.0)
if occ_free(CS[0], CS[1], 220):
    occ_add(CS[0], CS[1], 200)
    cyaw = 25.0
    ca, sa = math.cos(math.radians(cyaw)), math.sin(math.radians(cyaw))
    def cp(x, y):
        return (CS[0] + x * ca - y * sa, CS[1] + x * sa + y * ca)
    BODY = [(-95, -12), (-95, -28), (-20, -22), (-8, -18), (28, -26), (52, -62), (64, -55), (44, -18), (58, -14), (60, 14),
            (44, 18), (68, 52), (58, 60), (28, 24), (-8, 18), (-20, 24), (-95, 32), (-95, 14), (-28, 5), (-28, -5), (-95, -12)]
    for k in range(len(BODY) - 1):
        seg(f'Crime_Craie_{k}', cp(*BODY[k]), cp(*BODY[k + 1]), 1.7, 3.0, 0.4, MAT_CHALK)
    for k in range(14):                                       # la tête
        a0, a1 = 2 * math.pi * k / 14, 2 * math.pi * (k + 1) / 14
        seg(f'Crime_Craie_Tete_{k}', cp(80 + 17 * math.cos(a0), 17 * math.sin(a0)), cp(80 + 17 * math.cos(a1), 17 * math.sin(a1)), 1.7, 3.0, 0.4, MAT_CHALK)
    # chevalets aux quatre coins, reliés par le ruban
    HORSES = [cp(-170, -150), cp(170, -150), cp(170, 150), cp(-170, 150)]
    for k, (hx, hy) in enumerate(HORSES):
        hyaw = cyaw + (0 if k % 2 == 0 else 90)
        ux, uy = math.cos(math.radians(hyaw)), math.sin(math.radians(hyaw))
        for s in (-1, 1):
            for d in (-1, 1):
                box(f'Crime_Chevalet_{k}_Pied_{s}_{d}', (hx + ux * s * 50 - uy * d * 12, hy + uy * s * 50 + ux * d * 12, 45), (5, 5, 90), MAT_STRIPE_L, **AT3)
        for j in range(6):                                    # barre rayée noir et blanc
            off = -50 + 100 * (j + 0.5) / 6
            spawn(f'Crime_Chevalet_{k}_Barre_{j}', CUBE, (hx + ux * off, hy + uy * off, 92), yaw=hyaw,
                  scale=(100 / 6 / 100, 0.04, 0.16), mat=MAT_STRIPE_D if j % 2 else MAT_STRIPE_L, **AT3)
    for k in range(4):
        a, b = HORSES[k], HORSES[(k + 1) % 4]
        seg(f'Crime_Ruban_{k}', a, b, 95, 0.6, 7, MAT_TAPE)
    mid = ((HORSES[0][0] + HORSES[1][0]) / 2, (HORSES[0][1] + HORSES[1][1]) / 2)
    text('Crime_Ruban_Texte', (mid[0], mid[1], 95), cyaw - 90, 'POLICE LINE - DO NOT CROSS', 6.0, (20, 20, 10))
    # chapeau tombé et arme du mort
    hx, hy = cp(105, 40)
    spawn('Crime_Chapeau_Bord', CYL, (hx, hy, 2.2), scale=(0.36, 0.36, 0.018), mat=MAT_STRIPE_D, **AT3)
    spawn('Crime_Chapeau_Calotte', CYL, (hx, hy, 8.0), scale=(0.2, 0.2, 0.12), mat=MAT_STRIPE_D, **AT3)
    try:
        gx, gy = cp(30, -85)
        pistol = unreal.load_asset('/Game/RESHOOT/Noir/Meshes/SM_Noir_Pistol_Real/SM_Noir_Pistol_Real/StaticMeshes/SM_Noir_Pistol_Real')
        if pistol:
            spawn('Crime_Arme', pistol, (gx, gy, 3.0), yaw=cyaw + 70, roll=90.0, **AT)
    except Exception as e:
        unreal.log_warning(f'[NOIR SET] arme de la scène de crime : {e}')
    for k, (mx, my) in enumerate([cp(30, -110), cp(125, 55), cp(-120, 60)]):
        box(f'Crime_Marqueur_{k}', (mx, my, 5), (12, 8, 10), MAT_TAPE, **AT3)
        text(f'Crime_Marqueur_{k}_Num', (mx, my, 6), cyaw + 180, str(k + 1), 7.0, (10, 10, 10))
    light('Crime_Lumiere', unreal.PointLight, (CS[0], CS[1], 260), intensity=30.0, radius=600, temp=4000, shadows=False, volumetric=1.0)
else:
    skipped.append('Scene_de_crime')

# --- Cabines téléphoniques contre les façades : bois sombre, vitres éclairées de l'intérieur, enseigne TELEPHONE ---
def booth(k, wall, along):
    x, y = at_wall(wall, along, 95)
    if not occ_free(x, y, 75):
        skipped.append(f'Cabine_{k}'); return
    occ_add(x, y, 75)
    for sa in (-45, 45):
        for sp in (-45, 45):
            box(f'Cabine_{k}_Montant_{sa}_{sp}', wall_point(wall, along + sa, 95 + sp, 115), (10, 10, 230), MAT_BOOTH, **AT3)
    box(f'Cabine_{k}_Socle', (x, y, 5), sz(wall, 100, 100, 10), MAT_BOOTH, **AT3)
    box(f'Cabine_{k}_Toit', (x, y, 235), sz(wall, 112, 112, 12), MAT_BOOTH, **AT3)
    box(f'Cabine_{k}_Fond', wall_point(wall, along, 47, 115), sz(wall, 90, 4, 220), MAT_BOOTH, **AT3)
    for s in (-1, 1):                                          # côtés vitrés
        box(f'Cabine_{k}_Vitre_{s}', wall_point(wall, along + s * 45, 95, 130), sz(wall, 3, 80, 150), MAT_GLASS, shadow=False, **AT3)
    box(f'Cabine_{k}_Porte', wall_point(wall, along, 143, 130), sz(wall, 80, 3, 150), MAT_GLASS, shadow=False, **AT3)
    box(f'Cabine_{k}_Enseigne', wall_point(wall, along, 146, 214), sz(wall, 90, 3, 22), MAT_NEON, shadow=False, **AT3)
    text(f'Cabine_{k}_Enseigne_Texte', wall_point(wall, along, 148.5, 214), {'Nord': -90.0, 'Sud': 90.0, 'Est': 180.0, 'Ouest': 0.0}[wall], 'TELEPHONE', 12.0, (15, 15, 15))
    box(f'Cabine_{k}_Telephone', wall_point(wall, along, 55, 140), sz(wall, 22, 14, 30), MAT_METAL, **AT3)
    light(f'Cabine_{k}_Lumiere', unreal.PointLight, wall_point(wall, along, 95, 215), intensity=20.0, radius=450, temp=3000, shadows=False, volumetric=2.0)
booth(0, 'Est', 1650.0)
booth(1, 'Ouest', -300.0)

# --- Kiosque à journaux : comptoir, piles de journaux, auvent, enseigne NEWS ---
def kiosk(wall, along):
    x, y = at_wall(wall, along, 110)
    if not occ_free(x, y, 120):
        skipped.append('Kiosque'); return
    occ_add(x, y, 120)
    box('Kiosque_Caisse', wall_point(wall, along, 80, 110), sz(wall, 220, 100, 220), MAT_BOOTH, **AT3)
    box('Kiosque_Comptoir', wall_point(wall, along, 150, 50), sz(wall, 230, 50, 100), MAT_BOOTH, **AT3)
    box('Kiosque_Auvent', wall_point(wall, along, 165, 240), sz(wall, 250, 110, 8), MAT_AWNING, **AT3)
    box('Kiosque_Enseigne', wall_point(wall, along, 132, 262), sz(wall, 200, 4, 36), MAT_SIGN, **AT3)
    text('Kiosque_Enseigne_Texte', wall_point(wall, along, 135, 262), {'Nord': -90.0, 'Sud': 90.0, 'Est': 180.0, 'Ouest': 0.0}[wall], 'NEWS - DAILY STAR', 20.0)
    for j in range(7):                                         # piles de journaux sur le comptoir
        a2 = along - 95 + j * 31
        box(f'Kiosque_Pile_{j}', wall_point(wall, a2, 150, 100 + 3 * (j % 3) + 4), sz(wall, 26, 34, 8 + 3 * (j % 3)), MAT_PAPER, shadow=False, **AT3)
    for j in range(4):                                         # magazines accrochés en façade
        box(f'Kiosque_Revue_{j}', wall_point(wall, along - 75 + j * 50, 131, 170), sz(wall, 34, 2, 44), MAT_POSTER if j % 2 else MAT_POSTER_2, shadow=False, **AT3)
    light('Kiosque_Lumiere', unreal.PointLight, wall_point(wall, along, 190, 225), intensity=30.0, radius=600, temp=3200, shadows=False, volumetric=2.0)
kiosk('Ouest', 1780.0)

# --- Lampadaires en plus (réalistes et carton), au bord des trottoirs ---
for k, (x, y) in enumerate([(-2650, 0), (2650, -400), (-600, 2650), (1900, -2650), (-2650, -1100), (1150, 2650)]):
    if not occ_free(x, y, 40):
        skipped.append(f'Lampadaire2_{k}'); continue
    occ_add(x, y, 40)
    spawn(f'Lampadaire2_{k}', lamp, (x, y, 0), yaw=45 * k, **RD)
    spawn(f'Lampadaire2_{k}_Carton', lamp, (x, y, 0), yaw=45 * k, mat=MAT_PAINTED, **PD)
    light(f'Lampadaire2_{k}_Lumiere', unreal.PointLight, (x, y, 340), intensity=LAMP_CD, radius=2000, temp=2700, shadows=False, volumetric=4.0)

# --- Panneaux de rue : STOP et ONE WAY sur poteaux ---
for k, (x, y, words, yaw) in enumerate([(-2700, 2300, 'STOP', -45.0), (2400, 2700, 'ONE WAY', 180.0), (2700, -1600, 'NO PARKING', 90.0)]):
    if not occ_free(x, y, 20):
        skipped.append(f'Panneau_{k}'); continue
    occ_add(x, y, 20)
    spawn(f'Panneau_{k}_Poteau', CYL, (x, y, 125), scale=(0.06, 0.06, 2.5), mat=MAT_METAL, **AT3)
    spawn(f'Panneau_{k}_Plaque', CUBE, (x, y, 230), yaw=yaw, scale=(0.02, 0.6 if len(words) > 4 else 0.5, 0.35 if len(words) > 4 else 0.5), mat=MAT_STRIPE_L, **AT3)
    ux, uy = math.cos(math.radians(yaw)), math.sin(math.radians(yaw))
    for s in (-1, 1):
        text(f'Panneau_{k}_Texte_{s}', (x + ux * s * 1.6, y + uy * s * 1.6, 230), yaw if s > 0 else yaw + 180, words, 9.0, (15, 15, 15))

unreal.log_warning(f'[NOIR SET] accessoires : {n_post} affiches, rideau {sh_w:.0f}x{sh_d:.0f}, non posés (place prise) : {skipped}')

# =====================================================================================
# Lumière et atmosphère
# =====================================================================================
# Réaliste : nuit. Brume, lampadaires et appliques chauds (le plafond noir bloque le soleil du plateau).
fog = eas.spawn_actor_from_class(unreal.ExponentialHeightFog, unreal.Vector(0, 0, 0), unreal.Rotator(0, 0, 0))
fc = fog.get_component_by_class(unreal.ExponentialHeightFogComponent)
fc.set_editor_property('fog_density', 0.03); fc.set_editor_property('fog_height_falloff', 0.12)
fc.set_editor_property('volumetric_fog_scattering_distribution', 0.6)   # halo vers la caméra autour des lampes
fc.set_editor_property('enable_volumetric_fog', True)
fc.set_editor_property('fog_inscattering_luminance', unreal.LinearColor(0.002, 0.002, 0.003, 1))
fog.set_actor_label('Ruelle_Brume'); fog.set_folder_path('Dressing_Real/Lighting'); fog.tags = ['DressingReal']
ppv('PPV_Ruelle', 'Dressing_Real/Lighting', ['DressingReal'], REAL_BIAS, bloom_intensity=0.8, vignette_intensity=0.6, film_grain_intensity=0.35)

# Ciel de nuit : le soleil, le ciel et la lumière d'ambiance de L_Stage restent allumés dans toutes les phases.
# Une boîte fermée autour de la ville (réaliste seulement) cache le ciel du plateau et fait de l'ombre au soleil ;
# sa face intérieure est un ciel de ville gris très sombre, pour que les toits et les réservoirs s'y découpent.
SKY_H, RING = 3400.0, 4600.0
box('Ville_Ciel', (0, 0, SKY_H + 25), (2 * RING + 200, 2 * RING + 200, 50), MAT_SKY, shadow=True, **RT)
for k, (cx, cy, sx_, sy_) in enumerate([(RING, 0, 100, 2 * RING), (-RING, 0, 100, 2 * RING), (0, RING, 2 * RING, 100), (0, -RING, 2 * RING, 100)]):
    box(f'Ville_Horizon_{k}', (cx, cy, SKY_H / 2), (sx_, sy_, SKY_H), MAT_SKY, shadow=True, **RT)

# Carton : rien ici, le studio de L_Stage (projecteurs, exposition) éclaire déjà cette phase.

les.save_current_level()
n = len([a for a in eas.get_all_level_actors() if str(a.get_folder_path()).startswith('Dressing')])
unreal.log_warning(f'[NOIR SET] {n} acteurs d\'habillage, {len(covers)} couverts habillés')
