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
def spawn(label, mesh, loc, yaw=0.0, scale=(1, 1, 1), folder='Dressing_Real', tags=('DressingReal', 'DressingSwitch'), mat=None, shadow=True):
    a = eas.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(*loc), unreal.Rotator(pitch=0, yaw=yaw, roll=0))
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
for wall, b0, b1, h, bi in BUILDINGS:
    for along in ((b0 + b1) / 2, (b0 + b1) / 2 - 300, (b0 + b1) / 2 + 300):
        if along < b0 + 160 or along > b1 - 160 or wall_busy(wall, along, 260):
            continue
        box(f'Vitrine_{bi}_{along:.0f}_Cadre', wall_point(wall, along, 31, 140), sz(wall, 262, 4, 214), MAT_STONE, shadow=False, **RT)
        box(f'Vitrine_{bi}_{along:.0f}', wall_point(wall, along, 34, 140), sz(wall, 240, 4, 190), MAT_SHOP, shadow=False, **RT)
        box(f'Vitrine_{bi}_{along:.0f}_Marquise', wall_point(wall, along, 75, 268), sz(wall, 280, 90, 8), MAT_AWNING, **RT)
        n_shop += 1
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
            t = eas.spawn_actor_from_class(unreal.TextRenderActor, unreal.Vector(*p), unreal.Rotator(0, yaw, 0))
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
for i, (x, y, yaw) in enumerate([(-2700, -1900, 0), (2680, 1700, 0)]):
    spawn(f'Voiture_{i}', car, (x, y, 0), yaw=yaw, **RD)
    spawn(f'Voiture_{i}_Carton', car, (x, y, 0), yaw=yaw, mat=MAT_PAINTED, **PD)
    blk = eas.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(x, y, 70), unreal.Rotator(0, yaw, 0))
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
