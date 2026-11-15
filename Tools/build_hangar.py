# RESHOOT — plateau de l'univers spatial : un hangar de station, fermé.
#
# Génère, à partir du blockout de L_Stage :
#   - les corrections du blockout (portes est/ouest au niveau des mezzanines, coursives fermées, points d'apparition) ;
#   - le studio de tournage (phase carton, commun aux univers) dans L_Stage ;
#   - le hangar réaliste et le décor en carton dans L_Space.
# Chaque objet libre est contrôlé contre les zones de passage (rampes, portes, colonne de vol des drones,
# couverts, paliers). Un objet en conflit n'est pas posé : il est listé dans le rapport.
#
# À lancer dans l'éditeur (Python), jamais pendant une partie. Kits Fab : Sci_fi_hallway (hangar), Hollywood_Vol3 (studio, fermes).
import math

les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
eal = unreal.EditorAssetLibrary; at = unreal.AssetToolsHelpers.get_asset_tools(); mel = unreal.MaterialEditingLibrary
ar = unreal.AssetRegistryHelpers.get_asset_registry()
DISCARD = unreal.LevelEditorUnsavedChangesPolicy.DISCARD
if les.is_in_play_in_editor():
    raise RuntimeError('Arrête la partie (PIE) avant de lancer ce script.')

STUDIO_BIAS = -3.7
HANGAR_BIAS = -2.5
CEIL_I = 110.0          # luminaires du plafond (candelas)
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
for root in ['/Game/Sci_fi_hallway', '/Game/Hollywood_Vol3', '/Game/RESHOOT/Space/Meshes', '/Game/RESHOOT/Core/Meshes']:
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

PLANET_C = (9000.0, 42000.0, -7000.0)     # sous la baie, à droite : on voit l'horizon de la planète
PLANET_R = 19000.0
SUN_DIR = (-0.55, -0.55, 0.63)            # soleil en haut à gauche, côté hangar : la face visible est éclairée
PLANET_SEED = (3.7, 1.3, 8.2)             # change la disposition des continents
SKY_BRIGHT = 1.0
PLANET_BRIGHT = 1.0

# Bruit en HLSL (nœud Custom) : tout est calculé par pixel, donc net à toutes les résolutions, sans texture.
HLSL_LIB = r'''
struct ReshootNoise
{
	float h1(float3 p) { p = frac(p * 0.3183099 + 0.1); p *= 17.0; return frac(p.x * p.y * p.z * (p.x + p.y + p.z)); }
	float3 h3(float3 p) { return float3(h1(p), h1(p + 19.19), h1(p + 47.47)); }
	float vn(float3 x)
	{
		float3 i = floor(x); float3 f = frac(x); f = f * f * (3.0 - 2.0 * f);
		return lerp(lerp(lerp(h1(i), h1(i + float3(1, 0, 0)), f.x), lerp(h1(i + float3(0, 1, 0)), h1(i + float3(1, 1, 0)), f.x), f.y),
		            lerp(lerp(h1(i + float3(0, 0, 1)), h1(i + float3(1, 0, 1)), f.x), lerp(h1(i + float3(0, 1, 1)), h1(i + float3(1, 1, 1)), f.x), f.y), f.z);
	}
	float fbm(float3 p, int o)
	{
		float a = 0.5; float s = 0.0;
		for (int k = 0; k < o; k++) { s += a * vn(p); p = p * 2.03 + 17.1; a *= 0.5; }
		return s;
	}
};
ReshootNoise L;
'''

HLSL_SKY = HLSL_LIB + r'''
float3 d = normalize(-D);
float fwd = length(fwidth(d));
float3 bandN = normalize(float3(0.35, -0.25, 0.9));
float bd = dot(d, bandN);
float band = exp(-bd * bd * 14.0);
float3 col = 0;
float dens[3] = { 70.0, 160.0, 360.0 };
float gain[3] = { 1.6, 1.0, 0.55 };
for (int k = 0; k < 3; k++)
{
	float3 p = d * dens[k];
	float3 c = floor(p); float3 f = p - c;
	float3 pos = 0.25 + 0.5 * L.h3(c + k * 31.7);
	float3 hb = L.h3(c * 1.31 + 7.3 + k);
	float present = step(lerp(0.94, 0.80, band), hb.x);
	float sz = lerp(0.05, 0.10, hb.y);
	float s2 = max(sz, fwd * dens[k] * 0.75);
	float r = length(f - pos);
	float I = (0.25 + 10.0 * pow(hb.z, 10.0)) * (sz * sz) / (s2 * s2) * gain[k];
	float3 tint = lerp(float3(1.0, 0.74, 0.52), float3(0.72, 0.84, 1.0), frac(hb.z * 7.13));
	col += present * tint * I * exp(-r * r / (s2 * s2));
}
float n1 = L.fbm(d * 2.5 + 3.1, 6);
float n2 = L.fbm(d * 6.0 + 11.7, 5);
float dust = smoothstep(0.45, 0.68, L.fbm(d * 8.0 + 21.3, 5));
float3 milky = band * (0.6 * n1 * n1 + 0.4 * n2 * n2) * float3(0.55, 0.52, 0.62) * (1.0 - 0.8 * dust);
float neb = pow(saturate((L.fbm(d * 1.6 + 40.0, 6) - 0.42) * 3.0), 2.5);
float3 nebc = lerp(float3(0.30, 0.06, 0.24), float3(0.05, 0.14, 0.30), L.fbm(d * 3.0 + 70.0, 4));
col += milky * 0.18 + neb * nebc * 0.25;
return col * Bright;
'''

HLSL_PLANET = HLSL_LIB + r'''
float3 C = float3(%(cx)f, %(cy)f, %(cz)f); float R = %(r)f;
float3 Ls = normalize(float3(%(sx)f, %(sy)f, %(sz)f));
float3 ro = CP - C; float3 rd = normalize(WP - CP);
float b = dot(ro, rd); float c = dot(ro, ro) - R * R; float h = b * b - c;
float3 v = -rd;
if (h > 0.0 && b < 0.0)
{
	float3 nw = normalize(ro + rd * (-b - sqrt(h)));
	float3 ax = normalize(float3(0.25, -0.3, 0.92));
	float3 ex = normalize(cross(float3(0.0, 1.0, 0.0), ax));
	float3 ey = cross(ax, ex);
	float3 n = float3(dot(nw, ex), dot(nw, ey), dot(nw, ax));
	float lat = abs(n.z);
	float3 q = n * 2.0 + float3(%(seedx)f, %(seedy)f, %(seedz)f);
	float3 w = float3(L.fbm(q * 0.8, 4), L.fbm(q * 0.8 + 5.2, 4), L.fbm(q * 0.8 + 9.7, 4));
	float hgt = L.fbm(q + w * 1.5, 8);
	float sea = 0.53;
	float land = smoothstep(sea, sea + 0.004, hgt);
	float moist = L.fbm(n * 5.0 + 3.3, 6);
	float3 desert = float3(0.40, 0.31, 0.19);
	float3 forest = float3(0.035, 0.075, 0.03);
	float3 grass = float3(0.10, 0.12, 0.05);
	float3 rock = float3(0.22, 0.20, 0.18);
	float3 lc = lerp(lerp(grass, forest, smoothstep(0.48, 0.62, moist)), desert, smoothstep(0.47, 0.36, moist));
	lc = lerp(lc, rock, smoothstep(sea + 0.09, sea + 0.17, hgt));
	float ice = smoothstep(0.80, 0.87, lat + (hgt - sea) * 0.5);
	float depth = saturate((sea - hgt) / 0.08);
	float3 oc = lerp(float3(0.018, 0.06, 0.09), float3(0.003, 0.012, 0.035), depth);
	float3 alb = lerp(lerp(oc, lc, land), float3(0.80, 0.83, 0.87), ice);

	float3 cq = n * float3(3.0, 3.0, 6.5) + w * 1.6 + float3(T * 0.002, 0.0, 0.0);
	float cl = L.fbm(cq + 17.0, 6) + (L.fbm(cq * 5.0 + 3.0, 5) - 0.5) * 0.45;
	float cov = smoothstep(0.50, 0.58, cl) * (0.45 + 0.55 * smoothstep(0.35, 0.65, L.fbm(n * 1.3 + 50.0, 3)));

	float ndl = dot(nw, Ls);
	float day = smoothstep(-0.06, 0.35, ndl);
	float mu = saturate(dot(nw, v));
	float3 term = lerp(float3(1.0, 0.55, 0.32), float3(1.0, 1.0, 1.0), smoothstep(0.0, 0.3, ndl));
	float3 hlf = normalize(Ls + v);
	float spec = pow(saturate(dot(nw, hlf)), 160.0) * 5.0 * (1.0 - land) * (1.0 - ice) * (1.0 - cov);
	float3 ground = alb * (1.0 - 0.45 * cov) * day * term * 2.4 + spec * float3(1.0, 0.95, 0.88) * day;
	float3 cloudc = float3(0.92, 0.93, 0.95) * (0.01 + day * term * 1.9);
	float3 col = lerp(ground, cloudc, cov);
	float city = land * (1.0 - ice) * (1.0 - cov) * smoothstep(0.66, 0.82, L.fbm(n * 70.0, 4)) * smoothstep(0.02, -0.2, ndl);
	col += city * float3(1.0, 0.62, 0.28) * 0.6;
	float rim = pow(1.0 - mu, 2.5);
	col = lerp(col, float3(0.32, 0.58, 1.0) * (day * 1.3 + 0.01), rim * 0.5);
	return float4(col * Bright, 1.0);
}
float3 pc = ro - rd * b;
float dmin = length(pc);
float alt = max(dmin - R, 0.0) / (R * 0.03);
float sl = dot(normalize(pc), Ls);
float3 ac = lerp(float3(1.0, 0.45, 0.2), float3(0.32, 0.58, 1.0), smoothstep(0.0, 0.45, sl));
float a = saturate(exp(-alt * 3.0) * smoothstep(-0.3, 0.35, sl) * 1.1) * step(b, 0.0);
return float4(ac * a * 1.8 * Bright, a);
'''

def sky_materials():
    """Ciel étoilé (opaque, intérieur de sphère) et planète analytique (rayon-sphère par pixel, bord net, atmosphère)."""
    out_ = []
    params = dict(cx=PLANET_C[0], cy=PLANET_C[1], cz=PLANET_C[2], r=PLANET_R, sx=SUN_DIR[0], sy=SUN_DIR[1], sz=SUN_DIR[2],
                  seedx=PLANET_SEED[0], seedy=PLANET_SEED[1], seedz=PLANET_SEED[2])
    for name in ('M_Space_DeepSky', 'M_Space_PlanetAnalytic'):
        path = '/Game/RESHOOT/Space/Materials/' + name
        if eal.does_asset_exist(path):
            eal.delete_asset(path)
        m = at.create_asset(name, '/Game/RESHOOT/Space/Materials', unreal.Material, unreal.MaterialFactoryNew())
        m.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_UNLIT)
        E = lambda cls, x, y: mel.create_material_expression(m, cls, x, y)
        cus = E(unreal.MaterialExpressionCustom, -400, 0)
        bright = E(unreal.MaterialExpressionScalarParameter, -800, 300)
        bright.set_editor_property('parameter_name', 'Brightness')
        if name == 'M_Space_DeepSky':
            m.set_editor_property('two_sided', True)
            bright.set_editor_property('default_value', SKY_BRIGHT)
            cus.set_editor_property('code', HLSL_SKY)
            cus.set_editor_property('output_type', unreal.CustomMaterialOutputType.CMOT_FLOAT3)
            ins = []
            for n_ in ('D', 'Bright'):
                ci = unreal.CustomInput(); ci.set_editor_property('input_name', n_); ins.append(ci)
            cus.set_editor_property('inputs', ins)
            cam = E(unreal.MaterialExpressionCameraVectorWS, -800, 0)
            mel.connect_material_expressions(cam, '', cus, 'D')
            mel.connect_material_expressions(bright, '', cus, 'Bright')
            mel.connect_material_property(cus, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
        else:
            m.set_editor_property('blend_mode', unreal.BlendMode.BLEND_ALPHA_COMPOSITE)
            bright.set_editor_property('default_value', PLANET_BRIGHT)
            cus.set_editor_property('code', HLSL_PLANET % params)
            cus.set_editor_property('output_type', unreal.CustomMaterialOutputType.CMOT_FLOAT4)
            ins = []
            for n_ in ('WP', 'CP', 'T', 'Bright'):
                ci = unreal.CustomInput(); ci.set_editor_property('input_name', n_); ins.append(ci)
            cus.set_editor_property('inputs', ins)
            mel.connect_material_expressions(E(unreal.MaterialExpressionWorldPosition, -800, -200), '', cus, 'WP')
            mel.connect_material_expressions(E(unreal.MaterialExpressionCameraPositionWS, -800, -100), '', cus, 'CP')
            mel.connect_material_expressions(E(unreal.MaterialExpressionTime, -800, 100), '', cus, 'T')
            mel.connect_material_expressions(bright, '', cus, 'Bright')
            rgb = E(unreal.MaterialExpressionComponentMask, -150, 0)
            rgb.set_editor_property('r', True); rgb.set_editor_property('g', True); rgb.set_editor_property('b', True); rgb.set_editor_property('a', False)
            alpha = E(unreal.MaterialExpressionComponentMask, -150, 150)
            for ch in 'rgb': alpha.set_editor_property(ch, False)
            alpha.set_editor_property('a', True)
            mel.connect_material_expressions(cus, '', rgb, '')
            mel.connect_material_expressions(cus, '', alpha, '')
            mel.connect_material_property(rgb, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
            mel.connect_material_property(alpha, '', unreal.MaterialProperty.MP_OPACITY)
        mel.recompile_material(m); eal.save_loaded_asset(m)
        out_.append(m)
    return out_

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
    fc.set_editor_property('fog_cutoff_distance', 15000.0)   # le ciel et la planète, au-delà, restent nets
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
# Matériel de tournage : ses matériaux (kit Hollywood, modèles générés) ne réagissent pas au rideau,
# donc chaque objet bascule d'un coup au passage du rideau (DressingSwitch) au lieu d'attendre la fin du balayage.
P = dict(folder='Dressing_Prop/Studio', tags=('DressingProp', 'DressingSwitch'))
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

# ---- Kit Sci_fi_hallway : pivots centrés, face décorée côté +Y local ----
WALL_D = 117.0        # épaisseur du panneau SM_Sci_fi_wall (346 x 117 x 339)
def panel(label, wall, a, width, z0, height, b0=0.0, local_yaw=0.0):
    """Panneau de mur dont la face avant affleure le plan b0, de z0 à z0 + height."""
    spawn(label, SM('SM_Sci_fi_wall'), wpos(wall, a, b0 - WALL_D / 2, z0 + height / 2), yaw=wyaw(wall, local_yaw),
          scale=(width / 346, 1, height / 339), **K)
    count('wall')
def pillar(label, wall, a, z0, height, b0=0.0):
    spawn(label, SM('Sci_fi_wall_pillar'), wpos(wall, a, b0 - 40, z0 + height / 2), yaw=wyaw(wall), scale=(1.09, 1.09, height / 316), **K)
def bays(half):
    """Travées d'un mur : une travée de porte (400) au centre, le reste en travées régulières."""
    n = 7 if half >= 2500 else 5
    w = (half - DOOR_HALF) / n
    out_ = [(0.0, 2 * DOOR_HALF, True)]
    for k in range(n):
        c = DOOR_HALF + w * (k + 0.5)
        out_ += [(c, w, False), (-c, w, False)]
    return out_, w
TILE = 100.0          # dalle du kit (106) ramenée à 1 m : elle tombe juste sur les bords des mezzanines
TS = TILE / 106.0

# --- Sol ---
for i in range(50):
    cx = -2450 + 100 * i
    for j in range(40):
        cy = -1950 + 100 * j
        on_mezz = abs(cx) > 1500 and abs(cy) < 900
        spawn(f'Tile_{i}_{j}', SM('SM_floor_tile_01'), (cx, cy, (300 if on_mezz else 0) - 6), scale=(TS, TS, 1), **K); count('floor')

# --- Plafond : panneaux du kit, fermes métalliques tous les 5 m, luminaires du kit ---
for i in range(40):
    for j in range(32):
        spawn(f'Roof_{i}_{j}', SM('SM_Roof_panel'), (-2437.5 + 125 * i, -1937.5 + 125 * j, H + 5), yaw=180, roll=180, scale=(125 / 120, 125 / 120, 1), **K); count('ceiling')
for k in range(9):
    x = -2000 + 500 * k
    for j in range(10):
        spawn(f'Truss_{k}_{j}', SM('SM_Structural_Beam_01b'), (x, -2000 + 400 * (j + 1), H - 35), pitch=90, yaw=90, scale=(1, 1, 400 / 404), **K)
    for y in (-1300, 0, 1300):
        spawn(f'CeilFixture_{k}_{y}', SM('SM_Ceiling_light_01'), (x + 40, y, H - 30), yaw=-90, **K)
        light(f'CeilLight_{k}_{y}', unreal.RectLight, (x + 40, y, H - 50), (-90, 0, 0), color=(250, 232, 215), intensity=CEIL_I, radius=2200,
              shadows=(y == 0 and k % 2 == 0), extra={'source_width': 240.0, 'source_height': 30.0, 'barn_door_angle': 80.0})

# --- Murs pleins (sud, est, ouest) : trois rangs de panneaux de 4 m, piliers aux joints ---
for wall in ('S', 'E', 'W'):
    half = WALLS[wall][2]; fz = DOOR_FLOOR[wall]
    bl, w = bays(half)
    for a, width, is_door in bl:
        if is_door:
            top = fz + DOOR_H
            if top <= 400:      # porte au sol : le linteau reprend le rang du milieu
                panel(f'Lintel_{wall}', wall, a, width, 400, 400)
            else:               # porte sur quai (700) : un panneau de 4 m au-dessus, légèrement étiré
                panel(f'Lintel_{wall}', wall, a, width, top, 1000 - top)
            spawn(f'UpperPanel_{wall}_door', SM('SM_Sci_fi_wall_straight_01'), wpos(wall, a, -15, 1100 if top > 400 else 1000), yaw=wyaw(wall, 90),
                  scale=(1, width / 331, (200 if top > 400 else 400) / 329), **K)
            continue
        for r in range(2):
            panel(f'Panel_{wall}_{a:.0f}_{r}', wall, a, width, 400 * r, 400)
        # rang du haut : panneaux lisses (moins répétitif, lit comme une structure de hangar)
        spawn(f'UpperPanel_{wall}_{a:.0f}', SM('SM_Sci_fi_wall_straight_01'), wpos(wall, a, -15, 1000), yaw=wyaw(wall, 90),
              scale=(1, width / 331, 400 / 329), **K)
    joints = sorted(set([round(a + s * width / 2) for a, width, d in bl for s in (-1, 1)]))
    for jx in joints:
        for r in range(3):
            if abs(jx) <= DOOR_HALF and 400 * r < fz + DOOR_H:
                continue
            pillar(f'Pillar_{wall}_{jx}_{r}', wall, jx, 400 * r, 400)

# --- La baie (nord) : soubassement de 4 m, cadre métallique, champ de force, la planète derrière ---
half = WALLS['N'][2]
bl, w = bays(half)
for a, width, is_door in bl:
    if not is_door:
        panel(f'Panel_N_{a:.0f}_0', 'N', a, width, 0, 400)
joints = sorted(set([round(a + s * width / 2) for a, width, d in bl for s in (-1, 1)]))
for jx in joints:
    if abs(jx) > DOOR_HALF:
        pillar(f'Pillar_N_{jx}_0', 'N', jx, 0, 400)
    spawn(f'Mullion_{jx}', SM('SM_Straight_pillar_02'), wpos('N', jx, -32, 775), yaw=wyaw('N'), scale=(1, 1, 750 / 331), **K)
pillar('Pillar_N_door_L', 'N', -DOOR_HALF - 25, 0, 400)
pillar('Pillar_N_door_R', 'N', DOOR_HALF + 25, 0, 400)
for k in range(13):
    a = -2500 + 5000 * (k + 0.5) / 13
    for z in (400, H - 34):
        spawn(f'BayBeam_{k}_{z:.0f}', SM('SM_Structural_Beam_01b'), wpos('N', a - 5000 / 26, -17, z), pitch=90, yaw=wyaw('N'), scale=(1, 1, 5000 / 13 / 404), **K)
spawn('Bay_ForceField', PLANE, wpos('N', 0, -60, 775), roll=90, yaw=wyaw('N'), scale=(50, 7.5, 1), mat=MAT_FF, shadow=False, **K)

# --- Portes : piliers, coursive en kit, porte fermée au fond, balise rouge ---
for wall in WALLS:
    fz = DOOR_FLOOR[wall]
    for side in (-1, 1):
        pillar(f'DoorPillar_{wall}_{side}', wall, side * (DOOR_HALF + 25), fz, DOOR_H, b0=40)
        spawn(f'CorWall_{wall}_{side}', SM('SM_Sci_fi_wall_straight_01'), wpos(wall, side * (DOOR_HALF + 15), -100 - CORRIDOR_DEPTH / 2, fz + 250),
              yaw=wyaw(wall), scale=(1, (CORRIDOR_DEPTH + 100) / 331, 500 / 329), **K)
    for li in range(4):
        for bj in range(7):
            spawn(f'CorTile_{wall}_{li}_{bj}', SM('SM_floor_tile_01'), wpos(wall, -150 + 100 * li, -50 - 100 * bj, fz - 6), yaw=wyaw(wall), scale=(TS, TS, 1), **K)
            spawn(f'CorRoof_{wall}_{li}_{bj}', SM('SM_Roof_panel'), wpos(wall, -150 + 100 * li, -50 - 100 * bj, fz + 505), yaw=wyaw(wall, 180), roll=180, scale=(100 / 120, 100 / 120, 1), **K)
    spawn(f'CorDoorway_{wall}', SM('SM_Doorway_01'), wpos(wall, 0, -100 - CORRIDOR_DEPTH + 45, fz + 166), yaw=wyaw(wall), scale=(400 / 341, 1, 1), **K)
    for side in (-1, 1):
        spawn(f'CorDoor_{wall}_{side}', SM('SM_Door_half_01'), wpos(wall, side * 36, -100 - CORRIDOR_DEPTH + 45, fz + 110), yaw=wyaw(wall, 0 if side < 0 else 180), **K)
    spawn(f'CorEndTop_{wall}', SM('SM_Sci_fi_wall_straight_01'), wpos(wall, 0, -100 - CORRIDOR_DEPTH + 15, fz + 332 + 90), yaw=wyaw(wall, 90),
          scale=(1, 420 / 331, 190 / 329), **K)
    spawn(f'CorFixture_{wall}', SM('SM_Ceiling_light_01'), wpos(wall, 0, -400, fz + 490), yaw=wyaw(wall), **K)
    light(f'CorLight_{wall}', unreal.PointLight, wpos(wall, 0, -400, fz + 440), color=(235, 242, 255), intensity=2500, radius=900, shadows=False)
    light(f'GateBeacon_{wall}', unreal.PointLight, wpos(wall, 0, 120, fz + DOOR_H + 30), color=(255, 50, 30), intensity=120, radius=380, shadows=False)

# --- Mezzanines (quais) : parement en panneaux du kit ---
for wall in ('E', 'W'):
    for k in range(5):
        a = -900 + 360 * (k + 0.5)
        panel(f'MezzFront_{wall}_{k}', wall, a, 360, 0, 300, b0=1000)
    for side in (-1, 1):
        for k in range(3):
            b = 1000 * (k + 0.5) / 3
            spawn(f'MezzSide_{wall}_{side}_{k}', SM('SM_Sci_fi_wall'), wpos(wall, side * (900 - WALL_D / 2), b, 150), yaw=wyaw(wall, -90 * side),
                  scale=(1000 / 3 / 346, 1, 300 / 339), **K)

# --- Rampes : structure sombre + dalles du kit ---
for lbl, tr in block.items():
    if lbl.startswith('Ramp'):
        s = tr.scale3d
        a = spawn('Deck_' + lbl, CUBE, (0, 0, 0), mat=MAT_DECK, **RD)
        a.set_actor_transform(unreal.Transform(tr.translation, tr.rotation.rotator(), unreal.Vector(s.x + 0.03, s.y + 0.03, s.z + 0.03)), False, False)
run, rise = 1400.0, 300.0
ang = math.degrees(math.atan2(rise, run)); ln = math.hypot(run, rise)
for s in (-1, 1):
    for yc in (-800, -700, -600, -500, 500, 600, 700, 800):
        for k in range(14):
            t = (k + 0.5) / 14
            spawn(f'RampTile_{s}_{yc}_{k}', SM('SM_floor_tile_01'), (s * (1500 - run * t), yc, rise * (1 - t) + 6), pitch=s * ang, scale=(ln / 14 / 106, TS, 1), **K)

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
light('PlanetLight', unreal.DirectionalLight, (0, 0, 2500), (-18, -90, 0), color=(165, 200, 255), intensity=10.0)
light('BayGlow', unreal.RectLight, wpos('N', 0, 30, 800), (0, wyaw('N', 90), 0), color=(120, 170, 255), intensity=40.0, radius=4000, shadows=False,
      extra={'source_width': 4800.0, 'source_height': 650.0, 'intensity_units': unreal.LightUnits.CANDELAS})
sk = light('HangarAmbient', unreal.SkyLight, (0, 0, 600), intensity=1.6, shadows=False)
for i, (wall, a) in enumerate([('W', 0), ('E', 0), ('S', 0)]):
    spawn(f'AlarmLamp_{i}_Real', SM('SM_Space_AlarmLight_Real'), wpos(wall, a + 600, 10, 900), **RD)
    al = light(f'AlarmLight_{i}', unreal.PointLight, wpos(wall, a + 600, 80, 920), color=(255, 30, 20), intensity=9000, radius=2600, shadows=False,
               folder='Space/Event', tags=('GravityAlarm',))
    al.get_component_by_class(unreal.LightComponent).set_visibility(False)
haze('Hangar_Haze', 'Dressing_Real/Lighting', ['DressingReal'], 0.008, 0.06)
ppv('PPV_Hangar', 'Dressing_Real/Lighting', ['DressingReal'], HANGAR_BIAS, bloom_intensity=0.6, vignette_intensity=0.4,
    color_saturation=unreal.Vector4(0.9, 0.9, 0.9, 1), white_temp=7000.0)

# --- Dehors : ciel étoilé et planète, procéduraux (vus par la baie) ---
SKY = dict(folder='Dressing_Real/Sky', tags=('DressingReal', 'DressingSwitch'), shadow=False)
sky_mat, planet_mat = sky_materials()
spawn('Sky_Starfield', SPHERE, (0, 0, 0), scale=(1500, 1500, 1500), mat=sky_mat, **SKY)
# La sphère n'est qu'un support : la planète est calculée par pixel (rayon-sphère), l'atmosphère déborde jusqu'à 1,12 R.
spawn('Planet', SPHERE, PLANET_C, scale=(PLANET_R * 1.12 / 50,) * 3, mat=planet_mat, **SKY)
les.save_current_level()
les.load_level('/Game/RESHOOT/Core/Maps/L_Stage', unsaved_changes_policy=DISCARD)
out['hangar_conflicts'] = list(REPORT)
out.update(cnt)
out
