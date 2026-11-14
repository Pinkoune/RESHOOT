# RESHOOT — reconstruit l'habillage du plateau (studio carton dans L_Stage, pont spatial dans L_Space)
# à partir du blockout. À lancer dans l'éditeur (Python), jamais pendant une partie (PIE).
# Kits Fab utilisés : Hallway_sci_fi (modules de vaisseau), Hollywood_Vol3 (matériel de tournage).
import math

STUDIO_BIAS = -3.7   # exposition figée de la phase carton
SPACE_BIAS = -3.0    # exposition figée de la phase réaliste

les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
eal = unreal.EditorAssetLibrary; at = unreal.AssetToolsHelpers.get_asset_tools(); mel = unreal.MaterialEditingLibrary
ar = unreal.AssetRegistryHelpers.get_asset_registry()

# ------------------------------------------------------------------ assets
_cache = {}
for root in ['/Game/Hallway_sci_fi', '/Game/Hollywood_Vol3', '/Game/RESHOOT/Space/Meshes', '/Game/RESHOOT/Core/Meshes']:
    for d in ar.get_assets_by_path(root, recursive=True):
        if str(d.asset_class_path.asset_name) == 'StaticMesh':
            _cache[str(d.asset_name)] = str(d.package_name)
def SM(name):
    return unreal.load_asset(_cache[name])
CUBE = unreal.load_asset('/Engine/BasicShapes/Cube.Cube')

def mi(path, parent, real=None, scalars=None, vectors=None, textures=None):
    folder, name = path.rsplit('/', 1)
    m = unreal.load_asset(path) if eal.does_asset_exist(path) else at.create_asset(name, folder, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    m.set_editor_property('parent', unreal.load_asset(parent))
    if real is not None: mel.set_material_instance_static_switch_parameter_value(m, 'bIsReal', real)
    for k, v in (scalars or {}).items(): mel.set_material_instance_scalar_parameter_value(m, k, v)
    for k, v in (vectors or {}).items(): mel.set_material_instance_vector_parameter_value(m, k, unreal.LinearColor(*v))
    for k, v in (textures or {}).items(): mel.set_material_instance_texture_parameter_value(m, k, v)
    mel.update_material_instance(m); eal.save_loaded_asset(m)
    return m
TILED = '/Game/RESHOOT/Core/Materials/M_SetDressing_Tiled'
MAT_STUDIO_FLOOR = mi('/Game/RESHOOT/Core/Materials/MI_Stage_Floor_Prop', TILED, False, {'Roughness': 0.35, 'Metallic': 0.0}, {'BaseColorTint': (0.035, 0.035, 0.04, 1)})
MAT_CURTAIN = mi('/Game/RESHOOT/Core/Materials/MI_Stage_Curtain_Prop', TILED, False, {'Roughness': 1.0, 'Metallic': 0.0}, {'BaseColorTint': (0.006, 0.006, 0.007, 1)})
MAT_PLYWOOD = mi('/Game/RESHOOT/Core/Materials/MI_Stage_Plywood_Prop', TILED, False, {'Roughness': 0.8, 'Metallic': 0.0}, {'BaseColorTint': (0.42, 0.29, 0.17, 1)})
MAT_DECK = mi('/Game/RESHOOT/Space/Materials/MI_Space_DeckMetal_Real', TILED, True, {'Roughness': 0.45, 'Metallic': 0.8}, {'BaseColorTint': (0.05, 0.055, 0.065, 1)})

# ------------------------------------------------------------------ helpers
def spawn(label, mesh, loc, yaw=0.0, pitch=0.0, roll=0.0, scale=(1, 1, 1), folder='Dressing_Real', tags=('DressingReal',), mat=None, shadow=True):
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

def light(label, cls, loc, rot=(0, 0, 0), color=(255, 255, 255), intensity=1.0, folder='Dressing_Real/Lighting', tags=('DressingReal',), radius=None, cones=None, shadows=True, temp=None, extra=None):
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
    # Exposition figée : plus de surexposition quand les lumières basculent.
    put('auto_exposure_method', unreal.AutoExposureMethod.AEM_MANUAL)
    put('auto_exposure_apply_physical_camera_exposure', False)
    put('auto_exposure_bias', bias)
    for k, val in kw.items(): put(k, val)
    v.set_editor_property('settings', s)
    v.set_actor_label(label); v.set_folder_path(folder); v.tags = list(tags)
    return v

# Repère d'un mur : a = le long du mur, b = vers l'intérieur du plateau (b < 0 : coulisses)
WALLS = {'S': ((0, -2000), 0.0, 2500, 300), 'N': ((0, 2000), 180.0, 2500, 300), 'E': ((2500, 0), 90.0, 2000, 400), 'W': ((-2500, 0), -90.0, 2000, 400)}
def wpos(wall, a, b, z=0.0):
    (ox, oy), th, _, _ = WALLS[wall]
    c, s = math.cos(math.radians(th)), math.sin(math.radians(th))
    return (ox + a * c - b * s, oy + a * s + b * c, z)
def wyaw(wall, local=0.0):
    return WALLS[wall][1] + local

def clear(prefixes=(), folders=()):
    n = 0
    for a in eas.get_all_level_actors():
        f = str(a.get_folder_path())
        if a.get_actor_label().startswith(tuple(prefixes)) or (folders and f.startswith(tuple(folders))):
            eas.destroy_actor(a); n += 1
    return n

out = {}
if les.is_in_play_in_editor():
    raise RuntimeError('Arrête la partie (PIE) avant de lancer ce script.')

# =========================================================================================
# L_Stage : blockout invisible en jeu, studio de tournage (phase carton, commun aux univers)
# =========================================================================================
les.load_level('/Game/RESHOOT/Core/Maps/L_Stage', unsaved_changes_policy=unreal.LevelEditorUnsavedChangesPolicy.DISCARD)
out['stage_removed'] = clear(folders=('Dressing', 'Lighting'))
block = []
for a in eas.get_all_level_actors():
    if isinstance(a, unreal.StaticMeshActor) and str(a.get_folder_path()).startswith('Stage'):
        a.set_actor_hidden_in_game(True)          # collision seule : c'est l'habillage qui se voit
        block.append((a.get_actor_label(), a.get_actor_transform()))
# Plafond de collision : en apesanteur, le joueur ne sort plus par le haut
if not [a for a in eas.get_all_level_actors() if a.get_actor_label() == 'Ceiling']:
    c = eas.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(0, 0, 1250), unreal.Rotator(0, 0, 0))
    c.static_mesh_component.set_static_mesh(CUBE); c.set_actor_scale3d(unreal.Vector(66, 56, 1))
    c.set_actor_label('Ceiling'); c.set_folder_path('Stage'); c.set_actor_hidden_in_game(True)
    c.static_mesh_component.set_editor_property('cast_shadow', False)

P = dict(folder='Dressing_Prop/Studio', tags=('DressingProp',))
# Sol de studio, rideaux noirs, praticables en contreplaqué : copies du blockout, 3 cm plus grandes
for lbl, tr in block:
    s = tr.scale3d
    big = (s.x + 0.03, s.y + 0.03, s.z + 0.03)
    if lbl.startswith('Floor'): m = MAT_STUDIO_FLOOR
    elif lbl.startswith(('Wall', 'Lintel')): m = MAT_CURTAIN
    elif lbl.startswith(('Mezzanine', 'Ramp')): m = MAT_PLYWOOD
    else: continue   # les couverts sont habillés par l'univers
    a = spawn('Studio_' + lbl, CUBE, (0, 0, 0), mat=m, folder='Dressing_Prop/Shell', tags=('DressingProp',))
    a.set_actor_transform(unreal.Transform(tr.translation, tr.rotation.rotator(), unreal.Vector(*big)), False, False)

# Gril : quatre tours et un cadre de poutres à 8 m, projecteurs suspendus
GX, GY, GZ = 1010, 808, 808
for sx in (-1, 1):
    for sy in (-1, 1):
        for k in range(2):
            spawn(f'Truss_Tower_{sx}_{sy}_{k}', SM('SM_Structural_Beam_01b'), (sx * GX, sy * GY, k * 404), **P)
for sy in (-1, 1):
    for k in range(5):
        spawn(f'Truss_X_{sy}_{k}', SM('SM_Structural_Beam_01b'), (-GX + 404 * (k + 1), sy * GY, GZ + 17), pitch=90, **P)
for sx in (-1, 1):
    for k in range(4):
        spawn(f'Truss_Y_{sx}_{k}', SM('SM_Structural_Beam_01b'), (sx * GX, -GY + 404 * (k + 1), GZ + 17), pitch=90, yaw=90, **P)
L = dict(folder='Dressing_Prop/Lighting', tags=('DressingProp',))
hang = [((-GX, 0), (-1100, 0)), ((GX, 0), (1100, 0)), ((-500, GY), (-350, 250)), ((500, GY), (450, 300)), ((-500, -GY), (-350, -250)), ((500, -GY), (450, -300)), ((0, GY), (0, 900)), ((0, -GY), (0, -900))]
for i, ((x, y), (tx, ty)) in enumerate(hang):
    r = look_rot((x, y, GZ - 30), (tx, ty, 0))
    spawn(f'Stage_HangLight_{i}', SM('SM_Stage_Lights_01a'), (x, y, GZ - 40), pitch=r[0], yaw=r[1], **P)
    light(f'Stage_HangSpot_{i}', unreal.SpotLight, (x, y, GZ - 70), r, intensity=9000, radius=2600, cones=(16, 30), temp=3400, **L)
# Projecteurs de production sur pied, en contre-jour depuis les bords
stands = [(-2250, -1650), (-2250, 1650), (2250, -1650), (2250, 1650), (-250, -1800), (250, 1800)]
for i, (x, y) in enumerate(stands):
    r = look_rot((x, y, 250), (x * 0.25, y * 0.25, 100))
    spawn(f'Stage_ProdLight_{i}', SM('SM_Production_Light_01a'), (x, y, 0), yaw=r[1] + 180, **P)
    light(f'Stage_ProdSpot_{i}', unreal.SpotLight, (x * 0.97, y * 0.97, 255), r, intensity=14000, radius=4200, cones=(18, 34), temp=4300, **L)
# Petits pieds lumière et matériel au sol
for i, (x, y, yaw) in enumerate([(-1750, -1050, 20), (1750, 1050, 200), (-800, 1750, -60), (900, -1750, 120)]):
    spawn(f'Stage_LightStand_{i}', SM('SM_Light_Stand_01a'), (x, y, 0), yaw=yaw, **P)
for i, (x, y, yaw) in enumerate([(-1350, -1700, 55), (1450, 1650, -125)]):
    spawn(f'Stage_Camera_{i}', SM('SM_Stage_Camera'), (x, y, 0), yaw=yaw, **P)
spawn('Stage_BoomMic', SM('SM_Stage_BoomMic'), (-650, 1650, 0), yaw=-80, **P)
for i, (x, y, yaw) in enumerate([(-2300, -1300, 0), (2300, 1300, 180), (-1900, 1750, 90), (1950, -1750, -90), (-2380, 1100, 15)]):
    spawn(f'Stage_FlightCase_{i}', SM('SM_Stage_FlightCase'), (x, y, 0), yaw=yaw, **P)
for i, (x, y) in enumerate(stands):
    spawn(f'Stage_Sandbags_{i}', SM('SM_Stage_Sandbags'), (x * 0.96, y * 0.96 + 70, 0), yaw=40 * i, **P)
    spawn(f'Stage_Cables_{i}', SM('SM_Stage_Cables'), (x * 0.9, y * 0.9, 0), yaw=70 * i, **P)
# Ambiance : une lumière de service très faible, une brume légère pour voir les faisceaux
light('Stage_HouseLight', unreal.PointLight, (0, 0, 1100), color=(200, 215, 255), intensity=2500, radius=7000, shadows=False, **L)
fog = eas.spawn_actor_from_class(unreal.ExponentialHeightFog, unreal.Vector(0, 0, 0), unreal.Rotator(0, 0, 0))
fc = fog.get_component_by_class(unreal.ExponentialHeightFogComponent)
fc.set_editor_property('fog_density', 0.012); fc.set_editor_property('fog_height_falloff', 0.05)
fc.set_editor_property('enable_volumetric_fog', True); fc.set_editor_property('volumetric_fog_extinction_scale', 0.6)
fc.set_editor_property('fog_inscattering_luminance', unreal.LinearColor(0.0, 0.0, 0.0, 1))
fog.set_actor_label('Stage_Haze'); fog.set_folder_path('Dressing_Prop/Lighting'); fog.tags = ['DressingProp']
ppv('PPV_Studio', 'Dressing_Prop/Lighting', ['DressingProp'], STUDIO_BIAS, bloom_intensity=0.35, vignette_intensity=0.45)
les.save_current_level()
out['stage'] = 'ok'

# =========================================================================================
# L_Space : pont d'envol (réaliste) + décor en carton aux mêmes places
# =========================================================================================
les.load_level('/Game/RESHOOT/Space/Maps/L_Space', unsaved_changes_policy=unreal.LevelEditorUnsavedChangesPolicy.DISCARD)   # il est reconstruit en entier
n = 0
for a in eas.get_all_level_actors():
    # on ne retire que l'habillage : meshes, lumières, brouillard, post-process (l'événement de gravité reste)
    if isinstance(a, (unreal.StaticMeshActor, unreal.Light, unreal.SkyLight, unreal.PostProcessVolume, unreal.ExponentialHeightFog, unreal.SkyAtmosphere)):
        eas.destroy_actor(a); n += 1
out['space_removed'] = n
R = dict(folder='Dressing_Real/Kit', tags=('DressingReal', 'DressingSwitch'))
cnt = {'floor': 0, 'wall': 0, 'prop': 0}

# --- Sol : dalles de 2 m, y compris sur les mezzanines ---
for i in range(25):
    cx = -2400 + 200 * i
    for j in range(20):
        y0 = -2000 + 200 * j
        on_mezz = abs(cx) > 1500 and -900 <= y0 and y0 + 200 <= 900
        spawn(f'Deck_{i}_{j}', SM('SM_floor_a1_2_2'), (cx, y0, 300 if on_mezz else 0), **R); cnt['floor'] += 1

def wall_bay(wall, a, b=0.0, local_yaw=0.0, z=0.0, tag=''):
    for k, n_ in enumerate(('SM_wall_bot_a1', 'SM_wall_mid_a1', 'SM_wall_top_a1')):
        spawn(f'Wall_{wall}{tag}_{a:.0f}_{b:.0f}_{k}', SM(n_), wpos(wall, a, b, z + 100 * k), yaw=wyaw(wall, local_yaw), **R); cnt['wall'] += 1
def column(wall, a, b=0.0, local_yaw=0.0, tag=''):
    spawn(f'Col_{wall}{tag}_{a:.0f}_{b:.0f}', SM('SM_column_a1_1'), wpos(wall, a, b, 0), yaw=wyaw(wall, local_yaw), **R)

# --- Murs bas du pourtour, colonnes, corniche, portes et sas ---
for wall, (_, _, half, door) in WALLS.items():
    first = -half + 100
    bays = [first + 200 * k for k in range(int(half / 100))]
    for a in bays:
        if abs(a) < door: continue
        wall_bay(wall, a)
        spawn(f'Trim_{wall}_{a:.0f}', SM('SM_decel_a1'), wpos(wall, a, 0, 300), yaw=wyaw(wall), **R)
    edges = [-half + 400 * k for k in range(int(half / 200) + 1)]
    for a in edges:
        if abs(a) < door: continue
        column(wall, a)
    column(wall, -door); column(wall, door)
    # Grande porte : cadre du kit, ajusté à la largeur du passage
    spawn(f'Gate_{wall}', SM('SM_door2frame'), wpos(wall, 0, 20, 0), yaw=wyaw(wall), scale=(2 * door / 650, 1, 400 / 350), **R)
    # Sas derrière la porte : c'est de là que sortent les figurants
    cols = [-door + 100 + 200 * k for k in range(int(door / 100))]
    for a in cols:
        for bb in (-600, -400, -200):
            spawn(f'SasFloor_{wall}_{a:.0f}_{bb}', SM('SM_floor_a1_2_2'), wpos(wall, a, bb, 0), yaw=wyaw(wall), **R)
            spawn(f'SasCeil_{wall}_{a:.0f}_{bb}', SM('SM_celling_a1_2_2'), wpos(wall, a, bb, 400), yaw=wyaw(wall), **R)
        wall_bay(wall, a, -600, 0.0, tag='SasEnd')
        spawn(f'SasEndTop_{wall}_{a:.0f}', SM('SM_wall_top_a1'), wpos(wall, a, -600, 300), yaw=wyaw(wall), **R)
    for side in (-1, 1):
        for bb in (-500, -300, -100):
            # mur latéral du sas, face tournée vers l'axe de la porte
            for k, n_ in enumerate(('SM_wall_bot_a1', 'SM_wall_mid_a1', 'SM_wall_top_a1', 'SM_wall_top_a1')):
                spawn(f'SasSide_{wall}_{side}_{bb}_{k}', SM(n_), wpos(wall, side * door, bb, 100 * k), yaw=wyaw(wall, 90.0 * side), **R); cnt['wall'] += 1
    light(f'SasLight_{wall}', unreal.PointLight, wpos(wall, 0, -330, 320), color=(255, 60, 40), intensity=2200, radius=850, shadows=False)

# --- Mezzanines : parement de 3 m côté plateau et sur les flancs ---
for s, wall in ((1, 'E'), (-1, 'W')):
    for a in (-400, -200, 0, 200, 400):
        wall_bay(wall, a, 1000, 0.0, tag='Mezz')          # face tournée vers le centre
    for side in (-1, 1):
        for bb in (100, 300, 500, 700, 900):
            for k, n_ in enumerate(('SM_wall_bot_a1', 'SM_wall_mid_a1', 'SM_wall_top_a1')):
                spawn(f'MezzSide_{wall}_{side}_{bb}_{k}', SM(n_), wpos(wall, side * 900, bb, 100 * k), yaw=wyaw(wall, -90.0 * side), **R); cnt['wall'] += 1

# --- Rampes : structure sombre (copie du blockout) + dalles ---
for lbl, tr in block:
    if lbl.startswith('Ramp'):
        s = tr.scale3d
        a = spawn('Deck_' + lbl, CUBE, (0, 0, 0), mat=MAT_DECK, folder='Dressing_Real/Kit', tags=('DressingReal',))
        a.set_actor_transform(unreal.Transform(tr.translation, tr.rotation.rotator(), unreal.Vector(s.x + 0.03, s.y + 0.03, s.z + 0.03)), False, False)
run, rise = 1400.0, 300.0
ang = math.degrees(math.atan2(rise, run)); ln = math.hypot(run, rise)
for s in (-1, 1):
    for y0 in (-850, -650, 450, 650):
        for k in range(7):
            t = (k + 0.5) / 7
            x = s * (1500 - run * t); z = rise * (1 - t) + 12
            spawn(f'RampTile_{s}_{y0}_{k}', SM('SM_floor_a1_2_2'), (x, y0, z), pitch=s * ang, scale=(ln / 7 / 200, 1, 1), **R)

# --- Couverts : muret du kit (réaliste) et caisse en carton (carton), posés SUR le couvert du blockout ---
PD = dict(folder='Dressing_Prop', tags=('DressingProp',))
RD = dict(folder='Dressing_Real', tags=('DressingReal',))
crate_p, crate_r = SM('SM_Space_CargoCrate_Prop'), SM('SM_Space_CargoCrate_Real')
for lbl, tr in block:
    if not lbl.startswith('Cover'): continue
    p = tr.translation; sx, sy = tr.scale3d.x * 100, tr.scale3d.y * 100
    long_x = sx >= sy
    length, depth = (sx, sy) if long_x else (sy, sx)
    yaw = 0.0 if long_x else 90.0
    for side in (0, 180):
        for k, n_ in enumerate(('SM_wall_bot_a1',)):
            spawn(f'{lbl}_Kit_{side}', SM(n_), (p.x, p.y, 0), yaw=yaw + side, scale=(length / 200, depth / 106, 1), **R)
    # caisse réaliste posée dessus, caisse carton étirée à la taille du couvert
    spawn(f'{lbl}_Crate_Real', crate_r, (p.x, p.y, 101), yaw=yaw + 12, scale=(0.8, 0.8, 0.8), **RD)
    spawn(f'{lbl}_Crate_Prop', crate_p, (p.x, p.y, 0), yaw=yaw, scale=(length / 120, depth / 105, 100 / 90), **PD); cnt['prop'] += 1

# --- Châssis en carton : un décor de panneaux devant l'emplacement des vrais murs ---
flat = SM('SM_Space_CorridorPanel_Prop')
for wall, (_, _, half, door) in WALLS.items():
    a = door + 200
    while a + 200 <= half:
        for side in (-1, 1):
            spawn(f'Flat_{wall}_{side * a:.0f}', flat, wpos(wall, side * a, 75, 0), yaw=wyaw(wall), **PD); cnt['prop'] += 1
        a += 400

# --- Accessoires : les deux versions au même endroit, adossées aux murs ---
def twin(name, loc, yaw=0.0, scale=1.0):
    spawn(f'{name}_Prop_{loc[0]:.0f}_{loc[1]:.0f}', SM(f'SM_Space_{name}_Prop'), loc, yaw=yaw, scale=(scale,) * 3, **PD)
    spawn(f'{name}_Real_{loc[0]:.0f}_{loc[1]:.0f}', SM(f'SM_Space_{name}_Real'), loc, yaw=yaw, scale=(scale,) * 3, **RD)
for wall, spots in {'N': [(-1700, 'Console'), (-900, 'Barrel'), (1300, 'Console'), (2100, 'Barrel')], 'S': [(-2100, 'Barrel'), (-1300, 'Console'), (900, 'Barrel'), (1700, 'Console')]}.items():
    for a, name in spots:
        twin(name, wpos(wall, a, 150 if name == 'Console' else 130, 0), yaw=wyaw(wall, 180 if name == 'Console' else 25), scale=0.8)
for s in (-1, 1):   # sur les mezzanines
    twin('Barrel', (s * 2250, -700, 300), yaw=40, scale=0.8); twin('CargoCrate', (s * 2200, 650, 300), yaw=-20)
    twin('Console', (s * 1650, 0, 300), yaw=90 * s + 90, scale=0.8)

# --- Lumière de l'univers ---
# Lampes du kit sur les murs + lumière d'appoint sans ombre
for wall, (_, _, half, door) in WALLS.items():
    a = door + 300
    while a < half:
        for side in (-1, 1):
            spawn(f'Lamp_{wall}_{side * a:.0f}', SM('SM_lamp_1'), wpos(wall, side * a, 62, 268), yaw=wyaw(wall), roll=90, **R)
            light(f'LampLight_{wall}_{side * a:.0f}', unreal.PointLight, wpos(wall, side * a, 140, 250), color=(190, 225, 255), intensity=1100, radius=900, shadows=False)
        a += 800
# Lumière de la planète : rasante, froide, avec de longues ombres
light('PlanetLight', unreal.DirectionalLight, (0, 0, 2500), (-24, -105, 0), color=(170, 205, 255), intensity=9.0)
sk = light('SpaceAmbient', unreal.SkyLight, (0, 0, 600), intensity=1.0, color=(255, 255, 255), shadows=False)
sk.light_component.set_editor_property('sky_distance_threshold', 20000.0)   # capte la sphère du ciel et les plans peints
for i, (x, y) in enumerate([(-2380, 0), (2380, 0), (0, 1900)]):
    spawn(f'AlarmLamp_{i}_Real', SM('SM_Space_AlarmLight_Real'), (x, y, 352), **RD)
    al = light(f'AlarmLight_{i}', unreal.PointLight, (x * 0.96, y * 0.96, 400), color=(255, 30, 20), intensity=9000, radius=2600, shadows=False, folder='Space/Event', tags=('GravityAlarm',))
    al.get_component_by_class(unreal.LightComponent).set_visibility(False)
fog = eas.spawn_actor_from_class(unreal.ExponentialHeightFog, unreal.Vector(0, 0, 0), unreal.Rotator(0, 0, 0))
fc = fog.get_component_by_class(unreal.ExponentialHeightFogComponent)
fc.set_editor_property('fog_density', 0.006); fc.set_editor_property('fog_height_falloff', 0.08)
fc.set_editor_property('enable_volumetric_fog', True)
fc.set_editor_property('fog_inscattering_luminance', unreal.LinearColor(0.0, 0.0, 0.0, 1))
fog.set_actor_label('Space_Haze'); fog.set_folder_path('Dressing_Real/Lighting'); fog.tags = ['DressingReal']
ppv('PPV_Space', 'Dressing_Real/Lighting', ['DressingReal'], SPACE_BIAS, bloom_intensity=0.9, vignette_intensity=0.5)

# --- Ciel et lointains ---
_sky_tex = unreal.load_asset('/Game/RESHOOT/Space/Textures/T_Space_SkyPanorama')
_sky_tex.set_editor_property('address_x', unreal.TextureAddress.TA_MIRROR); eal.save_loaded_asset(_sky_tex)   # pas de couture
SKY = dict(folder='Dressing_Real/Sky', tags=('DressingReal', 'DressingSwitch'), shadow=False)
PLANE = unreal.load_asset('/Engine/BasicShapes/Plane.Plane'); SPH = unreal.load_asset('/Engine/BasicShapes/Sphere.Sphere')
spawn('Sky_Starfield', SPH, (0, 0, 0), scale=(1500, 1500, 1500), mat=unreal.load_asset('/Game/RESHOOT/Space/Materials/MI_Space_Sky'), **SKY)
spawn('Backdrop_Planet', PLANE, (3000, 26000, 7000), roll=90, scale=(260, 110, 1), mat=unreal.load_asset('/Game/RESHOOT/Space/Materials/MI_Space_PlanetBackdrop'), **SKY)
spawn('Backdrop_Hull', PLANE, (-26000, -2000, 2500), yaw=90, roll=90, scale=(300, 127, 1), mat=unreal.load_asset('/Game/RESHOOT/Space/Materials/MI_Space_HullBackdrop'), **SKY)
les.save_current_level()
les.load_level('/Game/RESHOOT/Core/Maps/L_Stage', unsaved_changes_policy=unreal.LevelEditorUnsavedChangesPolicy.DISCARD)
out.update(cnt)
out
