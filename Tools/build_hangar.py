# RESHOOT — plateau de l'univers spatial : un hangar de station, fermé.
#
# Génère, à partir du blockout de L_Stage :
#   - les corrections du blockout (portes est/ouest au niveau des mezzanines, coursives fermées, points d'apparition) ;
#   - le studio de tournage (phase carton, commun aux univers) dans L_Stage ;
#   - le hangar réaliste et le décor en carton dans L_Space.
# Chaque objet libre est contrôlé contre les zones de passage (rampes, portes, colonne de vol des drones,
# couverts, paliers). Un objet en conflit n'est pas posé : il est listé dans le rapport.
#
# À lancer dans l'éditeur (Python), jamais pendant une partie. Kits Fab : Hallway_sci_fi, Hollywood_Vol3.
import math

les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
eal = unreal.EditorAssetLibrary; at = unreal.AssetToolsHelpers.get_asset_tools(); mel = unreal.MaterialEditingLibrary
ar = unreal.AssetRegistryHelpers.get_asset_registry()
DISCARD = unreal.LevelEditorUnsavedChangesPolicy.DISCARD
if les.is_in_play_in_editor():
    raise RuntimeError('Arrête la partie (PIE) avant de lancer ce script.')

STUDIO_BIAS = -3.7
HANGAR_BIAS = -2.6
H = 1200.0                      # hauteur sous plafond

# =====================================================================================
# Géométrie du plateau (doit correspondre au blockout)
# =====================================================================================
#  repère d'un mur : a = le long du mur, b = vers l'intérieur (b < 0 : derrière le mur)
WALLS = {'S': ((0, -2000), 0.0, 2500), 'N': ((0, 2000), 180.0, 2500), 'E': ((2500, 0), 90.0, 2000), 'W': ((-2500, 0), -90.0, 2000)}
DOOR_HALF = 200.0
DOOR_FLOOR = {'S': 0.0, 'N': 0.0, 'E': 300.0, 'W': 300.0}   # est / ouest : au niveau des mezzanines
DOOR_H = 400.0
CORRIDOR_DEPTH = 600.0          # coursive derrière chaque porte (après l'épaisseur du mur, 100)
def wpos(wall, a, b, z=0.0):
    (ox, oy), th, _ = WALLS[wall]
    c, s = math.cos(math.radians(th)), math.sin(math.radians(th))
    return (ox + a * c - b * s, oy + a * s + b * c, z)
def wyaw(wall, local=0.0):
    return WALLS[wall][1] + local

# =====================================================================================
# Assets
# =====================================================================================
_cache = {}
for root in ['/Game/Hallway_sci_fi', '/Game/Hollywood_Vol3', '/Game/RESHOOT/Space/Meshes', '/Game/RESHOOT/Core/Meshes']:
    for d in ar.get_assets_by_path(root, recursive=True):
        if str(d.asset_class_path.asset_name) == 'StaticMesh':
            _cache[str(d.asset_name)] = str(d.package_name)
def SM(name):
    return unreal.load_asset(_cache[name])
CUBE = unreal.load_asset('/Engine/BasicShapes/Cube.Cube')
PLANE = unreal.load_asset('/Engine/BasicShapes/Plane.Plane')
SPHERE = unreal.load_asset('/Engine/BasicShapes/Sphere.Sphere')

def mi(path, parent, real=None, scalars=None, vectors=None):
    folder, name = path.rsplit('/', 1)
    m = unreal.load_asset(path) if eal.does_asset_exist(path) else at.create_asset(name, folder, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    m.set_editor_property('parent', unreal.load_asset(parent))
    if real is not None: mel.set_material_instance_static_switch_parameter_value(m, 'bIsReal', real)
    for k, v in (scalars or {}).items(): mel.set_material_instance_scalar_parameter_value(m, k, v)
    for k, v in (vectors or {}).items(): mel.set_material_instance_vector_parameter_value(m, k, unreal.LinearColor(*v))
    mel.update_material_instance(m); eal.save_loaded_asset(m)
    return m
TILED = '/Game/RESHOOT/Core/Materials/M_SetDressing_Tiled'
MAT_STUDIO_FLOOR = mi('/Game/RESHOOT/Core/Materials/MI_Stage_Floor_Prop', TILED, False, {'Roughness': 0.35, 'Metallic': 0.0}, {'BaseColorTint': (0.035, 0.035, 0.04, 1)})
MAT_CURTAIN = mi('/Game/RESHOOT/Core/Materials/MI_Stage_Curtain_Prop', TILED, False, {'Roughness': 1.0, 'Metallic': 0.0}, {'BaseColorTint': (0.006, 0.006, 0.007, 1)})
MAT_PLYWOOD = mi('/Game/RESHOOT/Core/Materials/MI_Stage_Plywood_Prop', TILED, False, {'Roughness': 0.8, 'Metallic': 0.0}, {'BaseColorTint': (0.42, 0.29, 0.17, 1)})
MAT_DECK = mi('/Game/RESHOOT/Space/Materials/MI_Space_DeckMetal_Real', TILED, True, {'Roughness': 0.45, 'Metallic': 0.8}, {'BaseColorTint': (0.05, 0.055, 0.065, 1)})

# Champ de force de la baie : voile additif bleuté, presque invisible
FF = '/Game/RESHOOT/Space/Materials/M_Space_ForceField'
if not eal.does_asset_exist(FF):
    m = at.create_asset('M_Space_ForceField', '/Game/RESHOOT/Space/Materials', unreal.Material, unreal.MaterialFactoryNew())
    m.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_UNLIT)
    m.set_editor_property('blend_mode', unreal.BlendMode.BLEND_ADDITIVE)
    m.set_editor_property('two_sided', True)
    fres = mel.create_material_expression(m, unreal.MaterialExpressionFresnel, -600, 0)
    col = mel.create_material_expression(m, unreal.MaterialExpressionVectorParameter, -600, 200)
    col.set_editor_property('parameter_name', 'Color'); col.set_editor_property('default_value', unreal.LinearColor(0.02, 0.08, 0.16, 1))
    base = mel.create_material_expression(m, unreal.MaterialExpressionConstant, -600, 350); base.set_editor_property('r', 0.25)
    add = mel.create_material_expression(m, unreal.MaterialExpressionAdd, -400, 100)
    mel.connect_material_expressions(fres, '', add, 'A'); mel.connect_material_expressions(base, '', add, 'B')
    mul = mel.create_material_expression(m, unreal.MaterialExpressionMultiply, -200, 150)
    mel.connect_material_expressions(add, '', mul, 'A'); mel.connect_material_expressions(col, '', mul, 'B')
    mel.connect_material_property(mul, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    mel.recompile_material(m); eal.save_loaded_asset(m)
MAT_FF = unreal.load_asset(FF)

# =====================================================================================
# Contrôle des placements
# =====================================================================================
class OBB:
    """Boîte orientée : centre, demi-tailles locales, lacet (deg) et tangage (deg)."""
    def __init__(self, name, center, half, yaw=0.0, pitch=0.0):
        self.name, self.c, self.h = name, center, half
        self.t = unreal.Transform(unreal.Vector(*center), unreal.Rotator(pitch=pitch, yaw=yaw, roll=0), unreal.Vector(1, 1, 1))
    def contains(self, p):
        q = self.t.inverse_transform_location(unreal.Vector(*p))
        return abs(q.x) <= self.h[0] and abs(q.y) <= self.h[1] and abs(q.z) <= self.h[2]

ZONES = []          # zones à garder libres
def keep_free(name, center, half, yaw=0.0, pitch=0.0):
    ZONES.append(OBB(name, center, half, yaw, pitch))

def sample_points(actor):
    o, e = actor.get_actor_bounds(False)
    pts = []
    for fx in (-0.95, 0, 0.95):
        for fy in (-0.95, 0, 0.95):
            for fz in (-0.9, 0, 0.9):
                pts.append((o.x + fx * e.x, o.y + fy * e.y, o.z + fz * e.z))
    return pts

REPORT = []
def validate(actor, allow=()):
    """Retire l'acteur s'il entre dans une zone libre (sauf celles listées dans allow)."""
    pts = sample_points(actor)
    for z in ZONES:
        if z.name in allow or any(z.name.startswith(a) for a in allow):
            continue
        if any(z.contains(p) for p in pts):
            REPORT.append(f'{actor.get_actor_label()} -> {z.name}')
            eas.destroy_actor(actor)
            return False
    return True

# =====================================================================================
# Pose
# =====================================================================================
def spawn(label, mesh, loc, yaw=0.0, pitch=0.0, roll=0.0, scale=(1, 1, 1), folder='Dressing_Real', tags=('DressingReal',),
          mat=None, shadow=True, check=False, allow=()):
    a = eas.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(*loc), unreal.Rotator(pitch=pitch, yaw=yaw, roll=roll))
    c = a.static_mesh_component
    c.set_static_mesh(mesh)
    a.set_actor_scale3d(unreal.Vector(*scale))
    if mat is not None:
        for i in range(max(1, len(mesh.static_materials))): c.set_material(i, mat)
    a.set_actor_label(label); a.set_folder_path(folder); a.tags = list(tags)
    a.set_actor_enable_collision(False); c.set_collision_profile_name('NoCollision')
    c.set_editor_property('cast_shadow', shadow)
    if check and not validate(a, allow):
        return None
    return a

def light(label, cls, loc, rot=(0, 0, 0), color=(255, 255, 255), intensity=1.0, folder='Dressing_Real/Lighting', tags=('DressingReal',),
          radius=None, cones=None, shadows=True, temp=None, extra=None):
    a = eas.spawn_actor_from_class(cls, unreal.Vector(*loc), unreal.Rotator(pitch=rot[0], yaw=rot[1], roll=rot[2]))
    lc = a.get_component_by_class(unreal.LightComponentBase)
    if hasattr(lc, 'set_mobility'): lc.set_mobility(unreal.ComponentMobility.MOVABLE)
    lc.set_editor_property('light_color', unreal.Color(r=color[0], g=color[1], b=color[2], a=255))
    lc.set_editor_property('intensity', intensity)
    lc.set_editor_property('cast_shadows', shadows)
    if temp is not None:
        lc.set_editor_property('use_temperature', True); lc.set_editor_property('temperature', temp)
    if radius is not None: lc.set_editor_property('attenuation_radius', radius)
    if cones is not None:
        lc.set_editor_property('inner_cone_angle', cones[0]); lc.set_editor_property('outer_cone_angle', cones[1])
    for k, v in (extra or {}).items(): lc.set_editor_property(k, v)
    a.set_actor_label(label); a.set_folder_path(folder); a.tags = list(tags)
    return a

def look_rot(src, dst):
    r = unreal.MathLibrary.find_look_at_rotation(unreal.Vector(*src), unreal.Vector(*dst))
    return (r.pitch, r.yaw, 0)

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

def haze(label, folder, tags, density, falloff):
    fog = eas.spawn_actor_from_class(unreal.ExponentialHeightFog, unreal.Vector(0, 0, 0), unreal.Rotator(0, 0, 0))
    fc = fog.get_component_by_class(unreal.ExponentialHeightFogComponent)
    fc.set_editor_property('fog_density', density); fc.set_editor_property('fog_height_falloff', falloff)
    fc.set_editor_property('enable_volumetric_fog', True)
    fc.set_editor_property('fog_inscattering_luminance', unreal.LinearColor(0.0, 0.0, 0.0, 1))
    fog.set_actor_label(label); fog.set_folder_path(folder); fog.tags = list(tags)

def block_box(label, wall, lat, b, z, size):
    """Boîte de collision du blockout, invisible en jeu, dans le repère d'un mur."""
    a = eas.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(*wpos(wall, lat, b, z)), unreal.Rotator(0, wyaw(wall), 0))
    a.static_mesh_component.set_static_mesh(CUBE)
    a.set_actor_scale3d(unreal.Vector(size[0] / 100, size[1] / 100, size[2] / 100))
    a.set_actor_label(label); a.set_folder_path('Stage/Corridors'); a.set_actor_hidden_in_game(True)
    a.static_mesh_component.set_editor_property('cast_shadow', False)

out = {}

# =====================================================================================
# 1) L_Stage : corrections du blockout
# =====================================================================================
les.load_level('/Game/RESHOOT/Core/Maps/L_Stage', unsaved_changes_policy=DISCARD)
for a in eas.get_all_level_actors():
    lbl = a.get_actor_label(); f = str(a.get_folder_path())
    if f.startswith(('Dressing', 'Lighting', 'Stage/Corridors')):
        eas.destroy_actor(a)
for a in eas.get_all_level_actors():
    lbl = a.get_actor_label()
    if lbl in ('Lintel_E', 'Lintel_W'):     # la porte va de 300 (mezzanine) à 700
        loc = a.get_actor_location(); sc = a.get_actor_scale3d()
        a.set_actor_location(unreal.Vector(loc.x, loc.y, 950), False, False)
        a.set_actor_scale3d(unreal.Vector(sc.x, sc.y, 5.0))
for wall in WALLS:
    fz = DOOR_FLOOR[wall]
    mid = -100 - CORRIDOR_DEPTH / 2
    block_box(f'Corridor_{wall}_SideL', wall, -DOOR_HALF - 50, mid, fz + 250, (100, CORRIDOR_DEPTH, 500))
    block_box(f'Corridor_{wall}_SideR', wall, DOOR_HALF + 50, mid, fz + 250, (100, CORRIDOR_DEPTH, 500))
    block_box(f'Corridor_{wall}_End', wall, 0, -100 - CORRIDOR_DEPTH - 50, fz + 250, (2 * DOOR_HALF + 200, 100, 500))
    block_box(f'Corridor_{wall}_Roof', wall, 0, mid, fz + 550, (2 * DOOR_HALF + 200, CORRIDOR_DEPTH + 200, 100))
    if fz > 0:
        block_box(f'Corridor_{wall}_Floor', wall, 0, mid - 50, fz / 2, (2 * DOOR_HALF, CORRIDOR_DEPTH + 100, fz))   # comble aussi le seuil
# Points d'apparition : dans l'embrasure, côté hangar (les figurants « sortent » de la coursive)
spawn_points = {'Spawn_Door_0': 'N', 'Spawn_Door_1': 'S', 'Spawn_Door_2': 'E', 'Spawn_Door_3': 'W'}
for a in eas.get_all_level_actors():
    if a.get_actor_label() in spawn_points:
        w = spawn_points[a.get_actor_label()]
        a.set_actor_location(unreal.Vector(*wpos(w, 0, 120, DOOR_FLOOR[w] + 150)), False, False)
        a.set_actor_rotation(unreal.Rotator(0, wyaw(w, 90), 0), False)

# Relevé du blockout final
block = {}
for a in eas.get_all_level_actors():
    if isinstance(a, unreal.StaticMeshActor) and str(a.get_folder_path()).startswith('Stage'):
        a.set_actor_hidden_in_game(True)
        block[a.get_actor_label()] = a.get_actor_transform()

# Zones à garder libres (pour le studio comme pour le hangar)
for lbl, tr in block.items():
    s = tr.scale3d
    if lbl.startswith('Ramp'):
        r = tr.rotation.rotator()
        # la rampe et 2,2 m d'air au-dessus
        up = unreal.Vector(0, 0, 1)
        keep_free(lbl, (tr.translation.x, tr.translation.y, tr.translation.z + 110), (s.x * 50 + 60, s.y * 50 + 40, 120), r.yaw, r.pitch)
    if lbl.startswith('Cover'):
        keep_free(lbl, (tr.translation.x, tr.translation.y, 50), (s.x * 50 + 40, s.y * 50 + 40, 60))
for wall in WALLS:
    fz = DOOR_FLOOR[wall]
    keep_free(f'Door_{wall}', wpos(wall, 0, -150, fz + 210), (DOOR_HALF + 30, 900, 210), wyaw(wall))   # embrasure + 6 m devant
for s in (-1, 1):
    for y in (-650, 650):
        keep_free(f'Landing_{s}_{y}', (s * 1650, y, 410), (200, 230, 110))     # palier en haut de rampe
keep_free('FlightColumn', (0, 0, 700), (1350, 650, 450))                         # le ciel des drones, au centre
keep_free('PlayerStart', (-1950, 0, 420), (180, 180, 120))

# =====================================================================================
# 2) L_Stage : studio de tournage (phase carton, commun aux univers)
# =====================================================================================
P = dict(folder='Dressing_Prop/Studio', tags=('DressingProp',))
for lbl, tr in block.items():
    s = tr.scale3d
    if lbl.startswith('Floor'): m = MAT_STUDIO_FLOOR
    elif lbl.startswith(('Wall', 'Lintel')): m = MAT_CURTAIN
    elif lbl.startswith(('Mezzanine', 'Ramp')): m = MAT_PLYWOOD
    else: continue
    a = spawn('Studio_' + lbl, CUBE, (0, 0, 0), mat=m, folder='Dressing_Prop/Shell', tags=('DressingProp',))
    a.set_actor_transform(unreal.Transform(tr.translation, tr.rotation.rotator(), unreal.Vector(s.x + 0.03, s.y + 0.03, s.z + 0.03)), False, False)
# Gril : 4 tours dans les angles libres du plateau (hors rampes, hors couverts), cadre à 8 m
GX, GY, GZ = 1100, 1550, 808
towers = []
for sx in (-1, 1):
    for sy in (-1, 1):
        for k in range(2):
            t = spawn(f'Truss_Tower_{sx}_{sy}_{k}', SM('SM_Structural_Beam_01b'), (sx * GX, sy * GY, k * 404), check=True, **P)
            towers.append(t)
for sy in (-1, 1):
    n = int(2 * GX / 404)
    for k in range(n):
        spawn(f'Truss_X_{sy}_{k}', SM('SM_Structural_Beam_01b'), (-GX + 2 * GX * (k + 1) / n, sy * GY, GZ + 17), pitch=90, scale=(1, 1, 2 * GX / n / 404), **P)
for sx in (-1, 1):
    n = int(2 * GY / 404)
    for k in range(n):
        spawn(f'Truss_Y_{sx}_{k}', SM('SM_Structural_Beam_01b'), (sx * GX, -GY + 2 * GY * (k + 1) / n, GZ + 17), pitch=90, yaw=90, scale=(1, 1, 2 * GY / n / 404), **P)
L = dict(folder='Dressing_Prop/Lighting', tags=('DressingProp',))
hang = [((-GX, -600), (-900, -300)), ((-GX, 600), (-900, 300)), ((GX, -600), (900, -300)), ((GX, 600), (900, 300)),
        ((-400, GY), (-300, 500)), ((400, GY), (300, 500)), ((-400, -GY), (-300, -500)), ((400, -GY), (300, -500))]
for i, ((x, y), (tx, ty)) in enumerate(hang):
    r = look_rot((x, y, GZ - 30), (tx, ty, 0))
    spawn(f'Stage_HangLight_{i}', SM('SM_Stage_Lights_01a'), (x, y, GZ - 40), pitch=r[0], yaw=r[1], **P)
    light(f'Stage_HangSpot_{i}', unreal.SpotLight, (x, y, GZ - 70), r, intensity=9000, radius=2600, cones=(16, 30), temp=3400, **L)
stands = [(-2250, -1650), (-2250, 1650), (2250, -1650), (2250, 1650), (-1300, -1750), (1300, 1750)]
for i, (x, y) in enumerate(stands):
    r = look_rot((x, y, 250), (x * 0.25, y * 0.25, 100))
    if spawn(f'Stage_ProdLight_{i}', SM('SM_Production_Light_01a'), (x, y, 0), yaw=r[1] + 180, check=True, **P):
        light(f'Stage_ProdSpot_{i}', unreal.SpotLight, (x * 0.97, y * 0.97, 255), r, intensity=14000, radius=4200, cones=(18, 34), temp=4300, **L)
        spawn(f'Stage_Sandbags_{i}', SM('SM_Stage_Sandbags'), (x * 0.96, y * 0.96 + 80, 0), yaw=40 * i, check=True, **P)
        spawn(f'Stage_Cables_{i}', SM('SM_Stage_Cables'), (x * 0.9, y * 0.9, 0), yaw=70 * i, check=True, **P)
for i, (x, y, yaw) in enumerate([(-1750, -1150, 20), (1750, 1150, 200), (-300, 1750, -60), (300, -1750, 120)]):
    spawn(f'Stage_LightStand_{i}', SM('SM_Light_Stand_01a'), (x, y, 0), yaw=yaw, check=True, **P)
for i, (x, y, yaw) in enumerate([(-1950, -1300, 40), (1950, 1300, -140)]):
    spawn(f'Stage_Camera_{i}', SM('SM_Stage_Camera'), (x, y, 0), yaw=yaw, check=True, **P)
spawn('Stage_BoomMic', SM('SM_Stage_BoomMic'), (-1600, 1650, 0), yaw=-60, check=True, **P)
for i, (x, y, yaw) in enumerate([(-2300, -1350, 0), (2300, 1350, 180), (-1700, 1800, 90), (1750, -1800, -90), (-2350, 1200, 15)]):
    spawn(f'Stage_FlightCase_{i}', SM('SM_Stage_FlightCase'), (x, y, 0), yaw=yaw, check=True, **P)
light('Stage_HouseLight', unreal.PointLight, (0, 0, 1100), color=(200, 215, 255), intensity=2500, radius=7000, shadows=False, **L)
haze('Stage_Haze', 'Dressing_Prop/Lighting', ['DressingProp'], 0.012, 0.05)
ppv('PPV_Studio', 'Dressing_Prop/Lighting', ['DressingProp'], STUDIO_BIAS, bloom_intensity=0.35, vignette_intensity=0.45)
les.save_current_level()
out['studio_conflicts'] = list(REPORT); REPORT.clear()

# =====================================================================================
# 3) L_Space : le hangar de station (réaliste) et le décor en carton
# =====================================================================================
les.load_level('/Game/RESHOOT/Space/Maps/L_Space', unsaved_changes_policy=DISCARD)
for a in eas.get_all_level_actors():
    if isinstance(a, (unreal.StaticMeshActor, unreal.Light, unreal.SkyLight, unreal.PostProcessVolume, unreal.ExponentialHeightFog, unreal.SkyAtmosphere)):
        eas.destroy_actor(a)
K = dict(folder='Dressing_Real/Hangar', tags=('DressingReal', 'DressingSwitch'))
PD = dict(folder='Dressing_Prop', tags=('DressingProp',))
RD = dict(folder='Dressing_Real', tags=('DressingReal',))
cnt = {}
def count(k): cnt[k] = cnt.get(k, 0) + 1

# Modules de mur du kit : face décorée côté +Y local, pivot au dos -> on recule le pivot de l'épaisseur
DEPTH = {'SM_wall_bot_a1': 53, 'SM_wall_mid_a1': 38, 'SM_wall_top_a1': 28, 'SM_w_1': 17, 'SM_empty_wall_2_2_2': 19}
def module(wall, name, a, z, b0=0.0, local_yaw=0.0, tag=''):
    spawn(f'{tag}{name[3:]}_{wall}_{a:.0f}_{z:.0f}_{b0:.0f}', SM(name), wpos(wall, a, b0 - DEPTH.get(name, 30), z), yaw=wyaw(wall, local_yaw), **K)
    count('wall')
def stack(wall, a, z0, b0=0.0, window=False, tag=''):
    module(wall, 'SM_wall_bot_a1', a, z0, b0, tag=tag)
    module(wall, 'SM_w_1' if window else 'SM_wall_mid_a1', a, z0 + 100, b0, tag=tag)
    module(wall, 'SM_wall_top_a1', a, z0 + 200, b0, tag=tag)
def column(wall, a, z0, height, b0=0.0):
    spawn(f'Col_{wall}_{a:.0f}_{z0:.0f}', SM('SM_column_a1_1'), wpos(wall, a, b0 - 60, z0), yaw=wyaw(wall), scale=(1, 1, height / 350), **K)

# --- Sol : dalles de 2 m (le niveau mezzanine est dallé à 3 m) ---
for i in range(25):
    cx = -2400 + 200 * i
    for j in range(20):
        y0 = -2000 + 200 * j
        on_mezz = abs(cx) > 1500 and -900 <= y0 and y0 + 200 <= 900
        spawn(f'Deck_{i}_{j}', SM('SM_floor_a1_2_2'), (cx, y0, 300 if on_mezz else 0), **K); count('floor')

# --- Plafond : panneaux du kit, poutres tous les 5 m, rampes de néons ---
for i in range(25):
    cx = -2400 + 200 * i
    for j in range(20):
        spawn(f'Ceil_{i}_{j}', SM('SM_celling_a1_2_2'), (cx, -2000 + 200 * j, H + 10), **K); count('ceiling')
for k in range(9):
    x = -2000 + 500 * k
    for j in range(9):
        spawn(f'Girder_{k}_{j}', SM('SM_rigel_a1_1'), (x, -2000 + 450 * j, H - 50), **K)
    for y in (-1300, 0, 1300):
        spawn(f'CeilLamp_{k}_{y}', SM('SM_lamp_1'), (x, y - 12, H - 70), yaw=90, **K)
        light(f'CeilLight_{k}_{y}', unreal.RectLight, (x, y, H - 90), (-90, 0, 0), color=(220, 235, 255), intensity=25.0, radius=1800,
              shadows=(y == 0 and k % 2 == 0), extra={'source_width': 200.0, 'source_height': 25.0, 'barn_door_angle': 70.0, 'intensity_units': unreal.LightUnits.CANDELAS})

# --- Murs pleins (sud, est, ouest) : 4 rangées de modules, hublots à mi-hauteur, colonnes tous les 4 m ---
for wall in ('S', 'E', 'W'):
    half = WALLS[wall][2]; fz = DOOR_FLOOR[wall]
    for k in range(int(half / 100)):
        a = -half + 100 + 200 * k
        if abs(a) < DOOR_HALF:
            for z0 in range(int(fz + DOOR_H), int(H), 100):
                module(wall, 'SM_wall_top_a1', a, z0, tag='Lintel')
            continue
        for row in range(4):
            stack(wall, a, 300 * row, window=(row == 2 and k % 2 == 0))
    for k in range(int(half / 200) + 1):
        a = -half + 400 * k
        if abs(a) < DOOR_HALF + 60: continue
        for row in range(4):
            column(wall, a, 300 * row, 300)
    for side in (-1, 1):
        for row in range(4):
            column(wall, side * (DOOR_HALF + 25), 300 * row, 300)

# --- La baie (nord) : soubassement, montants, champ de force, la planète derrière ---
half = WALLS['N'][2]
for k in range(int(half / 100)):
    a = -half + 100 + 200 * k
    if abs(a) >= DOOR_HALF:
        stack('N', a, 0)
        module('N', 'SM_wall_top_a1', a, 300)
    spawn(f'BaySill_{a:.0f}', SM('SM_decel_a1'), wpos('N', a, 0, 400), yaw=wyaw('N'), **K)
    spawn(f'BayHead_{a:.0f}', SM('SM_decel_a1'), wpos('N', a, 0, H - 50), yaw=wyaw('N'), **K)
for k in range(int(half / 200) + 1):
    a = -half + 400 * k
    if abs(a) >= DOOR_HALF + 60:
        column('N', a, 0, 400)
    column('N', a, 450, H - 500)
for side in (-1, 1):
    column('N', side * (DOOR_HALF + 25), 0, 400)
ff = spawn('Bay_ForceField', PLANE, wpos('N', 0, 5, 800), roll=90, yaw=wyaw('N'), scale=(2 * half / 100, (H - 500) / 100, 1), mat=MAT_FF, shadow=False, **K)

# --- Portes : cadre du kit, coursive éclairée derrière, balise rouge au-dessus ---
for wall in WALLS:
    fz = DOOR_FLOOR[wall]
    spawn(f'Gate_{wall}', SM('SM_door2frame'), wpos(wall, 0, 10, fz), yaw=wyaw(wall), scale=(0.85, 1, (DOOR_H + 40) / 350), **K)
    for lat in (-100, 100):
        for b in (-200, -400, -600):
            spawn(f'CorFloor_{wall}_{lat}_{b}', SM('SM_floor_a1_2_2'), wpos(wall, lat, b - 100, fz), yaw=wyaw(wall), **K)
            spawn(f'CorCeil_{wall}_{lat}_{b}', SM('SM_celling_a1_2_2'), wpos(wall, lat, b - 100, fz + 510), yaw=wyaw(wall), **K)
        for b in (-100, -50):
            spawn(f'CorSill_{wall}_{lat}_{b}', SM('SM_floor_a1_2_05'), wpos(wall, lat + 100, b, fz), yaw=wyaw(wall), **K)
    for side in (-1, 1):
        for b in (-200, -400, -600):
            for z0, nm in ((0, 'SM_wall_bot_a1'), (100, 'SM_wall_mid_a1'), (200, 'SM_wall_top_a1'), (300, 'SM_wall_bot_a1'), (400, 'SM_wall_top_a1')):
                spawn(f'CorWall_{wall}_{side}_{b}_{z0}', SM(nm), wpos(wall, side * (DOOR_HALF + DEPTH[nm]), b, fz + z0), yaw=wyaw(wall, 90 * side), **K)
    for lat in (-100, 100):
        for z0, nm in ((0, 'SM_wall_bot_a1'), (100, 'SM_wall_mid_a1'), (200, 'SM_wall_top_a1'), (300, 'SM_wall_bot_a1'), (400, 'SM_wall_top_a1')):
            module(wall, nm, lat, fz + z0, b0=-100 - CORRIDOR_DEPTH, tag='CorEnd')
    light(f'CorLight_{wall}', unreal.PointLight, wpos(wall, 0, -400, fz + 420), color=(200, 225, 255), intensity=3000, radius=900, shadows=False)
    light(f'GateBeacon_{wall}', unreal.PointLight, wpos(wall, 0, 60, fz + DOOR_H + 80), color=(255, 50, 30), intensity=1800, radius=700, shadows=False)

# --- Mezzanines : parement, bordure, piliers sous les quais ---
for wall in ('E', 'W'):
    for a in (-800, -600, -400, -200, 0, 200, 400, 600, 800):
        stack(wall, a, 0, b0=1000, tag='MezzFront')                         # face tournée vers le centre
        spawn(f'MezzEdge_{wall}_{a}', SM('SM_decel_a1'), wpos(wall, a, 975, 300), yaw=wyaw(wall), check=True, **K)
    for side in (-1, 1):
        for b in (100, 300, 500, 700, 900):
            for z0, nm in ((0, 'SM_wall_bot_a1'), (100, 'SM_wall_mid_a1'), (200, 'SM_wall_top_a1')):
                spawn(f'MezzSide_{wall}_{side}_{b}_{z0}', SM(nm), wpos(wall, side * (900 - DEPTH[nm]), b, z0), yaw=wyaw(wall, -90 * side), **K)

# --- Rampes : structure sombre + dalles ---
for lbl, tr in block.items():
    if lbl.startswith('Ramp'):
        s = tr.scale3d
        a = spawn('Deck_' + lbl, CUBE, (0, 0, 0), mat=MAT_DECK, **RD)
        a.set_actor_transform(unreal.Transform(tr.translation, tr.rotation.rotator(), unreal.Vector(s.x + 0.03, s.y + 0.03, s.z + 0.03)), False, False)
run, rise = 1400.0, 300.0
ang = math.degrees(math.atan2(rise, run)); ln = math.hypot(run, rise)
for s in (-1, 1):
    for y0 in (-850, -650, 450, 650):
        for k in range(7):
            t = (k + 0.5) / 7
            spawn(f'RampTile_{s}_{y0}_{k}', SM('SM_floor_a1_2_2'), (s * (1500 - run * t), y0, rise * (1 - t) + 12), pitch=s * ang, scale=(ln / 7 / 200, 1, 1), **K)

# --- Couverts : caisses arrimées (réaliste) / caisse en carton (carton), exactement sur la collision ---
crate_p, crate_r = SM('SM_Space_CargoCrate_Prop'), SM('SM_Space_CargoCrate_Real')
for lbl, tr in block.items():
    if not lbl.startswith('Cover'): continue
    p = tr.translation; sx, sy = tr.scale3d.x * 100, tr.scale3d.y * 100
    long_x = sx >= sy
    length, depth = (sx, sy) if long_x else (sy, sx)
    yaw = 0.0 if long_x else 90.0
    n = max(1, round(length / depth))
    for k in range(n):
        off = (k + 0.5) * length / n - length / 2
        cx, cy = (p.x + off, p.y) if long_x else (p.x, p.y + off)
        spawn(f'{lbl}_Crate_Real_{k}', crate_r, (cx, cy, 0), yaw=yaw, scale=(length / n / 112, depth / 119, 100 / 70), **RD)
    spawn(f'{lbl}_Crate_Prop', crate_p, (p.x, p.y, 0), yaw=yaw, scale=(length / 120, depth / 105, 100 / 90), **PD)

# --- Châssis en carton : décor de panneaux à l'emplacement des futurs murs ---
flat = SM('SM_Space_CorridorPanel_Prop')
for wall in WALLS:
    half = WALLS[wall][2]; fz = 0.0
    a = DOOR_HALF + 300
    while a + 200 <= half:
        for side in (-1, 1):
            spawn(f'Flat_{wall}_{side * a:.0f}', flat, wpos(wall, side * a, 80, fz), yaw=wyaw(wall), check=True, **PD)
        a += 420

# --- Accessoires du hangar : les deux versions au même endroit ---
def twin(name, loc, yaw=0.0, scale=1.0):
    r = spawn(f'{name}_Real_{loc[0]:.0f}_{loc[1]:.0f}', SM(f'SM_Space_{name}_Real'), loc, yaw=yaw, scale=(scale,) * 3, check=True, **RD)
    if r:
        spawn(f'{name}_Prop_{loc[0]:.0f}_{loc[1]:.0f}', SM(f'SM_Space_{name}_Prop'), loc, yaw=yaw, scale=(scale,) * 3, **PD)
# Postes de contrôle sous la baie, face au vide
for a in (-1800, -1000, 1000, 1800):
    twin('Console', wpos('N', a, 110, 0), yaw=wyaw('N'), scale=0.8)
# Bidons et caisses en groupes, dans les angles et le long du mur sud
for (x, y, yaw) in [(-2300, -1800, 20), (-2150, -1820, 70), (2300, -1800, -30), (2320, 1820, 10), (-2320, 1800, 50)]:
    twin('Barrel', (x, y, 0), yaw=yaw, scale=0.7)
for (x, y, yaw) in [(-1500, -1880, 0), (-1380, -1880, 8), (1500, -1880, -5), (1620, -1885, 4)]:
    twin('CargoCrate', (x, y, 0), yaw=yaw)
# Quais : consoles de chargement et caisses au fond des mezzanines
for s in (-1, 1):
    twin('Console', (s * 2380, -500, 300), yaw=90 * s + 90, scale=0.8)
    twin('CargoCrate', (s * 2360, 450, 300), yaw=s * 5)
    twin('Barrel', (s * 2380, 750, 300), yaw=30, scale=0.7)

# --- Lumière de la planète : entre par la baie, froide et rasante ---
light('PlanetLight', unreal.DirectionalLight, (0, 0, 2500), (-18, -90, 0), color=(165, 200, 255), intensity=7.0)
light('BayGlow', unreal.RectLight, wpos('N', 0, 30, 800), (0, wyaw('N', 90), 0), color=(120, 170, 255), intensity=6.0, radius=4000, shadows=False,
      extra={'source_width': 4800.0, 'source_height': 650.0, 'intensity_units': unreal.LightUnits.CANDELAS})
sk = light('HangarAmbient', unreal.SkyLight, (0, 0, 600), intensity=0.6, shadows=False)
for i, (wall, a) in enumerate([('W', 0), ('E', 0), ('S', 0)]):
    spawn(f'AlarmLamp_{i}_Real', SM('SM_Space_AlarmLight_Real'), wpos(wall, a + 600, 10, 900), **RD)
    al = light(f'AlarmLight_{i}', unreal.PointLight, wpos(wall, a + 600, 80, 920), color=(255, 30, 20), intensity=9000, radius=2600, shadows=False,
               folder='Space/Event', tags=('GravityAlarm',))
    al.get_component_by_class(unreal.LightComponent).set_visibility(False)
haze('Hangar_Haze', 'Dressing_Real/Lighting', ['DressingReal'], 0.008, 0.06)
ppv('PPV_Hangar', 'Dressing_Real/Lighting', ['DressingReal'], HANGAR_BIAS, bloom_intensity=0.8, vignette_intensity=0.45)

# --- Dehors : ciel, planète, coque d'un autre vaisseau (vus par la baie) ---
_sky_tex = unreal.load_asset('/Game/RESHOOT/Space/Textures/T_Space_SkyPanorama')
_sky_tex.set_editor_property('address_x', unreal.TextureAddress.TA_MIRROR); eal.save_loaded_asset(_sky_tex)
SKY = dict(folder='Dressing_Real/Sky', tags=('DressingReal', 'DressingSwitch'), shadow=False)
spawn('Sky_Starfield', SPHERE, (0, 0, 0), scale=(1500, 1500, 1500), mat=unreal.load_asset('/Game/RESHOOT/Space/Materials/MI_Space_Sky'), **SKY)
spawn('Backdrop_Planet', PLANE, (2500, 24000, 2600), roll=90, scale=(260, 110, 1), mat=unreal.load_asset('/Game/RESHOOT/Space/Materials/MI_Space_PlanetBackdrop'), **SKY)
spawn('Backdrop_Hull', PLANE, (-9000, 16000, -1200), roll=90, scale=(220, 93, 1), mat=unreal.load_asset('/Game/RESHOOT/Space/Materials/MI_Space_HullBackdrop'), **SKY)
les.save_current_level()
les.load_level('/Game/RESHOOT/Core/Maps/L_Stage', unsaved_changes_policy=DISCARD)
out['hangar_conflicts'] = list(REPORT)
out.update(cnt)
out
