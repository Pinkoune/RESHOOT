# RESHOOT — assets des effets laser (Space). À lancer dans l'éditeur (Unreal MCP, execute_python).
# Crée dans /Game/RESHOOT/Space/FX :
#   M_Space_LaserBeam   additif, éteint : Color × Intensity × Fade, plus brillant au centre du cylindre (halo doux)
#   M_Space_LaserGlow   opaque émissif : pièces lumineuses du fusil (Color × Intensity)
#   M_Space_LaserScorch décal de brûlure : métal noirci + point chaud orange qui refroidit (DecalLifetimeOpacity)
#   NS_Space_LaserSparks étincelles (copie du modèle Niagara DirectionalBurst)
# puis règle BP_SpaceLaser (fusil d'Epic + pièces lumineuses) et BP_SpaceDrone.
import unreal

eal = unreal.EditorAssetLibrary
at = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary
DIR = '/Game/RESHOOT/Space/FX'
out = {}

def new_material(name):
    path = f'{DIR}/{name}'
    if eal.does_asset_exist(path):     # on le vide plutôt que de le supprimer : il peut être référencé
        m = unreal.load_asset(path)
        mel.delete_all_material_expressions(m)
        return m
    return at.create_asset(name, DIR, unreal.Material, unreal.MaterialFactoryNew())

def param_vec(m, name, v, x, y):
    e = mel.create_material_expression(m, unreal.MaterialExpressionVectorParameter, x, y)
    e.set_editor_property('parameter_name', name); e.set_editor_property('default_value', unreal.LinearColor(*v))
    return e

def param_scalar(m, name, v, x, y):
    e = mel.create_material_expression(m, unreal.MaterialExpressionScalarParameter, x, y)
    e.set_editor_property('parameter_name', name); e.set_editor_property('default_value', v)
    return e

def custom(m, code, inputs, x, y, out_type=unreal.CustomMaterialOutputType.CMOT_FLOAT1):
    c = mel.create_material_expression(m, unreal.MaterialExpressionCustom, x, y)
    c.set_editor_property('code', code); c.set_editor_property('output_type', out_type)
    ins = []
    for n in inputs:
        ci = unreal.CustomInput(); ci.set_editor_property('input_name', n); ins.append(ci)
    c.set_editor_property('inputs', ins)
    return c

C = mel.connect_material_expressions

# --- Rayon ---
m = new_material('M_Space_LaserBeam')
m.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_UNLIT)
m.set_editor_property('blend_mode', unreal.BlendMode.BLEND_ADDITIVE)
m.set_editor_property('two_sided', True)
col = param_vec(m, 'Color', (0.1, 0.75, 1.0, 1), -900, -200)
inten = param_scalar(m, 'Intensity', 20.0, -900, 0)
fade = param_scalar(m, 'Fade', 1.0, -900, 100)
soft = custom(m, 'return pow(saturate(abs(dot(normalize(N), normalize(V)))), 1.6);', ['N', 'V'], -650, 250)
C(mel.create_material_expression(m, unreal.MaterialExpressionVertexNormalWS, -900, 250), '', soft, 'N')
C(mel.create_material_expression(m, unreal.MaterialExpressionCameraVectorWS, -900, 350), '', soft, 'V')
emis = custom(m, 'return Col * I * F * S;', ['Col', 'I', 'F', 'S'], -350, 0, unreal.CustomMaterialOutputType.CMOT_FLOAT3)
for e, n in ((col, 'Col'), (inten, 'I'), (fade, 'F'), (soft, 'S')):
    C(e, '', emis, n)
mel.connect_material_property(emis, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
mel.recompile_material(m); eal.save_loaded_asset(m)
BEAM = m

# --- Pièces lumineuses ---
m = new_material('M_Space_LaserGlow')
m.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_UNLIT)
col = param_vec(m, 'Color', (0.1, 0.75, 1.0, 1), -600, -100)
inten = param_scalar(m, 'Intensity', 8.0, -600, 100)
mul = mel.create_material_expression(m, unreal.MaterialExpressionMultiply, -300, 0)
C(col, '', mul, 'A'); C(inten, '', mul, 'B')
mel.connect_material_property(mul, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
mel.recompile_material(m); eal.save_loaded_asset(m)
GLOW = m

# --- Brûlure ---
m = new_material('M_Space_LaserScorch')
m.set_editor_property('material_domain', unreal.MaterialDomain.MD_DEFERRED_DECAL)
m.set_editor_property('blend_mode', unreal.BlendMode.BLEND_ALPHA_COMPOSITE)   # seuls modes admis pour un décal (Substrate)
hot = param_vec(m, 'HotColor', (1.0, 0.35, 0.08, 1), -900, -200)
uv = mel.create_material_expression(m, unreal.MaterialExpressionTextureCoordinate, -900, 0)
life = mel.create_material_expression(m, unreal.MaterialExpressionDecalLifetimeOpacity, -900, 150)
mask = custom(m, 'float r = length(UV - 0.5) * 2.0; float n = frac(sin(dot(floor(UV * 23.0), float2(12.9898, 78.233))) * 43758.5453); return saturate((1.0 - r) * 1.6 - n * 0.25);', ['UV'], -650, 0)
C(uv, '', mask, 'UV')
opac = custom(m, 'return saturate(M * 1.4) * saturate(Life * 1.5);', ['M', 'Life'], -350, 100)
C(mask, '', opac, 'M'); C(life, '', opac, 'Life')
emis = custom(m, 'return Hot * pow(M, 4.0) * pow(saturate(Life), 10.0) * 25.0;', ['Hot', 'M', 'Life'], -350, -150, unreal.CustomMaterialOutputType.CMOT_FLOAT3)
C(hot, '', emis, 'Hot'); C(mask, '', emis, 'M'); C(life, '', emis, 'Life')
base = mel.create_material_expression(m, unreal.MaterialExpressionConstant3Vector, -350, 250)
base.set_editor_property('constant', unreal.LinearColor(0.012, 0.011, 0.010, 1))
rough = mel.create_material_expression(m, unreal.MaterialExpressionConstant, -350, 350); rough.set_editor_property('r', 0.9)
mel.connect_material_property(base, '', unreal.MaterialProperty.MP_BASE_COLOR)
mel.connect_material_property(rough, '', unreal.MaterialProperty.MP_ROUGHNESS)
mel.connect_material_property(opac, '', unreal.MaterialProperty.MP_OPACITY)
mel.connect_material_property(emis, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
mel.recompile_material(m); eal.save_loaded_asset(m)
SCORCH = m

# --- Étincelles ---
NS = f'{DIR}/NS_Space_LaserSparks'
if not eal.does_asset_exist(NS):
    eal.duplicate_loaded_asset(unreal.load_asset('/Niagara/DefaultAssets/Templates/Systems/DirectionalBurst'), NS)
SPARKS = unreal.load_asset(NS)
USE_SPARKS = False   # le modèle DirectionalBurst fait des gerbes de 2 m, trop grosses : désactivé tant qu'il n'est pas réglé
out['assets'] = [BEAM.get_path_name(), GLOW.get_path_name(), SCORCH.get_path_name(), SPARKS.get_path_name() if SPARKS else None]

# Pièces lumineuses ajoutées sur le fusil (espace du mesh, tailles en cm). Vide : le fusil Fab est déjà lumineux.
def GLOW_PARTS(part):
    return [
    ]   # le fusil laser Fab a déjà ses propres lumières

# Pose de l'arme en main (relative à l'acteur arme, lui-même à (30, 15, -20) de la caméra) : grande, dans le coin bas-droit,
# comme dans les FPS modernes. Le canon est orienté pour que son axe passe par le centre de l'écran à 10 m : le tir
# sort tout droit du canon vers le viseur. Le carton est plus petit (modèle plus épais que le réaliste).
CAM = (-30.0, -15.0, 20.0)      # caméra dans l'espace de l'acteur arme
MUZZLE = (49.0, 0.0, 7.5)       # bout du canon dans l'espace du mesh (identique pour les deux modèles)
CONVERGE = 1000.0               # distance où l'axe du canon croise le centre de l'écran (cm)

def aim_pose(loc, scale, extra_yaw=0.0):
    import math
    def rot(v, pitch, yaw):
        x, y, z = v
        p = math.radians(pitch); x, z = x * math.cos(p) - z * math.sin(p), x * math.sin(p) + z * math.cos(p)
        w = math.radians(yaw); x, y = x * math.cos(w) - y * math.sin(w), x * math.sin(w) + y * math.cos(w)
        return (x, y, z)
    pitch = yaw = 0.0
    for _ in range(20):
        m = rot(tuple(scale * c for c in MUZZLE), pitch, yaw)
        t = [CAM[i] + (CONVERGE if i == 0 else 0.0) - loc[i] - m[i] for i in range(3)]
        yaw = math.degrees(math.atan2(t[1], t[0]))
        pitch = math.degrees(math.atan2(t[2], math.hypot(t[0], t[1])))
    return unreal.Transform(unreal.Vector(*loc), unreal.Rotator(roll=0, pitch=pitch, yaw=yaw + extra_yaw), unreal.Vector(scale, scale, scale))

POSE = aim_pose((-17, -6, 3), 1.15, -1.0)   # fusil réaliste : décollé du bord droit, proche de la caméra, presque droit
# (NearClipPlane=2 dans DefaultEngine.ini, sinon l'arrière du fusil est coupé par la caméra)
PROP_POSE = aim_pose((-9, 1, 0.5), 0.92)    # fusil en carton

# --- Blueprints (seulement si les classes compilées ont les nouvelles propriétés) ---
def setup_fx(fx, color, sparks=True):
    fx.set_editor_property('beam_material', BEAM)
    fx.set_editor_property('scorch_material', SCORCH)
    fx.set_editor_property('beam_color', unreal.LinearColor(*color, 1))
    fx.set_editor_property('impact_sparks', SPARKS if (sparks and USE_SPARKS) else None)

bp = unreal.load_asset('/Game/RESHOOT/Space/Weapons/BP_SpaceLaser')
cdo = unreal.get_default_object(bp.generated_class())
if hasattr(unreal, 'SpaceLaserFXComponent') and cdo.get_editor_property('laser_fx'):
    setup_fx(cdo.get_editor_property('laser_fx'), (0.1, 0.75, 1.0))
    # Fusil laser Fab (canon vers +X, bout du canon à 49 cm, rail de visée à 13,5 cm de haut)
    cdo.set_editor_property('real_mesh', unreal.load_asset('/Game/Fab/Laserrifle/laserrifle/StaticMeshes/laserrifle'))
    cdo.set_editor_property('real_mesh_transform', POSE)
    cdo.set_editor_property('real_muzzle_local', unreal.Vector(49, 0, 7.5))
    cdo.set_editor_property('aim_fov', 75.0)
    cdo.set_editor_property('glow_part_mesh', unreal.load_asset('/Engine/BasicShapes/Cube.Cube'))
    cdo.set_editor_property('glow_part_material', GLOW)
    cdo.set_editor_property('real_sight_local', unreal.Vector(-14, 0, 15))   # guidon arrière : en visée, on regarde le long du dessus de l'arme
    # Carton : sa propre pose (plus petit), même bout de canon (le modèle carton est calé sur le réaliste : 1 m, canon vers +X)
    cdo.set_editor_property('prop_mesh', unreal.load_asset('/Game/RESHOOT/Space/Meshes/LaserRifle/Prop/SM_Space_LaserRifle_Cardboard'))
    mesh_comp = cdo.get_editor_property('mesh')
    mesh_comp.set_editor_property('static_mesh', cdo.get_editor_property('prop_mesh'))
    mesh_comp.set_editor_property('relative_location', PROP_POSE.translation)
    mesh_comp.set_editor_property('relative_rotation', PROP_POSE.rotation.rotator())
    mesh_comp.set_editor_property('relative_scale3d', PROP_POSE.scale3d)
    cdo.set_editor_property('muzzle_offset', PROP_POSE.transform_location(unreal.Vector(*MUZZLE)))
    cdo.get_editor_property('laser_fx').set_editor_property('muzzle_flash_size', 4.5)
    cdo.get_editor_property('laser_fx').set_editor_property('core_intensity', 30.0)
    # Pièces lumineuses sur le fusil (espace du mesh : canon vers +Y, haut = +Z) : bandes le long du garde-main, cellule d'énergie
    def part(loc, size):
        return unreal.Transform(unreal.Vector(*loc), unreal.Rotator(0, 0, 0), unreal.Vector(size[0] / 100, size[1] / 100, size[2] / 100))
    cdo.set_editor_property('real_glow_parts', GLOW_PARTS(part))
    unreal.BlueprintEditorLibrary.compile_blueprint(bp); eal.save_loaded_asset(bp)
    out['laser_bp'] = 'ok'
bpd = unreal.load_asset('/Game/RESHOOT/Space/Enemies/BP_SpaceDrone')
dcdo = unreal.get_default_object(bpd.generated_class())
if hasattr(unreal, 'SpaceLaserFXComponent') and dcdo.get_editor_property('laser_fx'):
    setup_fx(dcdo.get_editor_property('laser_fx'), (1.0, 0.12, 0.06))
    unreal.BlueprintEditorLibrary.compile_blueprint(bpd); eal.save_loaded_asset(bpd)
    out['drone_bp'] = 'ok'
out
