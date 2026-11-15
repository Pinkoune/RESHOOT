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
les.load_level('/Game/RESHOOT/Noir/L_Noir')
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

# --- Façades de briques (réaliste) et châssis peints (carton) ---
for wall in WALLS:
    axis = WALLS[wall][0]
    for j, (a0, a1) in enumerate(wall_segments(wall)):
        mid, ln = (a0 + a1) / 2, a1 - a0
        size_r = (ln, 30, FACADE_H) if axis == 'x' else (30, ln, FACADE_H)
        box(f'Facade_{wall}_{j}', wall_point(wall, mid, 15, FACADE_H / 2), size_r, MAT_BRICK, **RT)
        size_p = (ln, 10, FLAT_H) if axis == 'x' else (10, ln, FLAT_H)
        box(f'Chassis_{wall}_{j}', wall_point(wall, mid, 45, FLAT_H / 2), size_p, MAT_FLAT, **PD)
    for c in WALLS[wall][3]:
        lin_r = (2 * DOOR_HALF, 30, FACADE_H - DOOR_H) if axis == 'x' else (30, 2 * DOOR_HALF, FACADE_H - DOOR_H)
        box(f'Facade_{wall}_Linteau_{c:.0f}', wall_point(wall, c, 15, (FACADE_H + DOOR_H) / 2), lin_r, MAT_BRICK, **RT)

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

# --- Lampadaires (réalistes allumés, en carton peint) ---
lamp = SM('SM_Noir_StreetLamp_Real')
LAMPS = [(-2000, 1700), (-2000, -1700), (300, 2100), (300, -2100), (2300, 1100), (2300, -1100)]
for i, (x, y) in enumerate(LAMPS):
    if not is_free((x, y, 0), 60):
        continue
    spawn(f'Lampadaire_{i}', lamp, (x, y, 0), yaw=30 * i, **RD)
    # Halo visible dans la brume ; ombres sur deux lampadaires seulement (trop de lumières à ombres au même endroit sinon).
    light(f'Lampadaire_{i}_Lumiere', unreal.PointLight, (x, y, 340), intensity=LAMP_CD, radius=2200, temp=2700,
          shadows=(i in (2, 3)), volumetric=4.0)
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
# Un plafond noir au-dessus de la ruelle (réaliste seulement) cache le ciel et fait de l'ombre au soleil.
box('Ruelle_CielNuit', (0, 0, FACADE_H + 25), (2 * W + 2000, 2 * W + 2000, 50), MAT_NIGHT, **RT)

# Carton : rien ici, le studio de L_Stage (projecteurs, exposition) éclaire déjà cette phase.

les.save_current_level()
n = len([a for a in eas.get_all_level_actors() if str(a.get_folder_path()).startswith('Dressing')])
unreal.log_warning(f'[NOIR SET] {n} acteurs d\'habillage, {len(covers)} couverts habillés')
