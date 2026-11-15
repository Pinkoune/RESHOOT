# RESHOOT — revolver du film noir, modelé par Geometry Script (Unreal MCP, execute_python).
# Le revolver n'avait aucun modèle : invisible en main au casting et pendant toute la scène noire.
# Crée dans /Game/RESHOOT/Noir/Meshes/Revolver :
#   SM_Noir_Revolver_Prop  version carton (formes épaisses, matériau du carton du Noir)
#   SM_Noir_Revolver_Real  version réaliste (plus fine, acier sombre), M_Noir_Revolver_Real
# puis les pose comme PropMesh / RealMesh de BP_NoirRevolver. Canon vers +X, pivot sous le barillet, 1 unité = 1 cm.
import unreal

eal = unreal.EditorAssetLibrary
at = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary
GP = unreal.GeometryScript_Primitives
DIR = '/Game/RESHOOT/Noir/Meshes/Revolver'
out = {}

def T(loc, rot=(0, 0, 0)):
    return unreal.Transform(unreal.Vector(*loc), unreal.Rotator(roll=rot[0], pitch=rot[1], yaw=rot[2]), unreal.Vector(1, 1, 1))

def box(m, loc, size, rot=(0, 0, 0)):
    o = unreal.GeometryScriptPrimitiveOptions()
    GP.append_box(m, o, T(loc, rot), size[0], size[1], size[2], 0, 0, 0, unreal.GeometryScriptPrimitiveOriginMode.CENTER)

def cyl_x(m, start_x, length, radius, y, z, steps=16):
    # Cylindre le long de +X (le cylindre de base est vertical : on le couche).
    o = unreal.GeometryScriptPrimitiveOptions()
    GP.append_cylinder(m, o, T((start_x, y, z), (0, -90, 0)), radius, length, steps, 0, True, unreal.GeometryScriptPrimitiveOriginMode.BASE)

def revolver(k):
    """k = épaisseur relative : 1.35 pour le carton (grossier), 1.0 pour le réaliste."""
    m = unreal.DynamicMesh()
    cyl_x(m, 3.0, 11.0, 0.9 * k, 0, 3.4, 8 if k > 1.1 else 20)          # canon
    box(m, (8.0, 0, 4.6), (10.0, 0.5 * k, 0.6 * k))                      # bande de visée sur le canon
    cyl_x(m, -1.0, 4.4, 2.1 * k, 0, 2.6, 8 if k > 1.1 else 24)          # barillet
    box(m, (0.5, 0, 1.9), (9.0, 2.0 * k, 3.6))                           # carcasse
    box(m, (-3.8, 0, 3.7), (1.6, 1.0 * k, 1.6), (0, -25, 0))             # chien
    box(m, (-5.2, 0, -3.2), (3.2, 2.4 * k, 9.0), (0, 18, 0))             # crosse
    box(m, (0.6, 0, -1.6), (4.4, 0.6 * k, 0.6))                          # pontet (bas)
    box(m, (2.6, 0, -0.6), (0.6, 0.6 * k, 2.2))                          # pontet (avant)
    box(m, (-0.4, 0, -0.5), (0.5, 0.5 * k, 1.8), (0, 15, 0))             # détente
    return m

def make_mesh(name, k):
    path = f'{DIR}/{name}'
    if not eal.does_asset_exist(path):
        eal.duplicate_asset('/Engine/BasicShapes/Cube', path)    # coquille de static mesh, remplie ci-dessous
    asset = unreal.load_asset(path)
    opts = unreal.GeometryScriptCopyMeshToAssetOptions()
    opts.set_editor_property('enable_recompute_normals', True)
    opts.set_editor_property('enable_recompute_tangents', True)
    opts.set_editor_property('replace_materials', False)
    unreal.GeometryScript_AssetUtils.copy_mesh_to_static_mesh(revolver(k), asset, opts, unreal.GeometryScriptMeshWriteLOD())
    return asset

# Matériau réaliste : acier bleui sombre (l'image passe en noir et blanc dans le Noir, seul le contraste compte).
MAT = '/Game/RESHOOT/Noir/Materials/M_Noir_Revolver_Real'
if eal.does_asset_exist(MAT):
    mat = unreal.load_asset(MAT); mel.delete_all_material_expressions(mat)
else:
    mat = at.create_asset('M_Noir_Revolver_Real', '/Game/RESHOOT/Noir/Materials', unreal.Material, unreal.MaterialFactoryNew())
c = mel.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -400, 0)
c.set_editor_property('constant', unreal.LinearColor(0.03, 0.032, 0.036, 1))
mt = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -400, 150); mt.set_editor_property('r', 1.0)
ro = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -400, 250); ro.set_editor_property('r', 0.32)
mel.connect_material_property(c, '', unreal.MaterialProperty.MP_BASE_COLOR)
mel.connect_material_property(mt, '', unreal.MaterialProperty.MP_METALLIC)
mel.connect_material_property(ro, '', unreal.MaterialProperty.MP_ROUGHNESS)
mel.recompile_material(mat); eal.save_loaded_asset(mat)

prop = make_mesh('SM_Noir_Revolver_Prop', 1.35)
real = make_mesh('SM_Noir_Revolver_Real', 1.0)
prop.set_material(0, unreal.load_asset('/Game/RESHOOT/Noir/Materials/MI_Noir_Flat_Prop'))   # aplat carton : les primitives n'ont pas d'UV adaptés à la texture
real.set_material(0, mat)
eal.save_loaded_asset(prop); eal.save_loaded_asset(real)

bp = unreal.load_asset('/Game/RESHOOT/Noir/Blueprints/BP_NoirRevolver')
cdo = unreal.get_default_object(bp.generated_class())
cdo.set_editor_property('prop_mesh', prop)
cdo.set_editor_property('real_mesh', real)
mc = cdo.get_editor_property('mesh')
mc.set_editor_property('static_mesh', prop)
# En main : plus petit qu'un fusil, plus près de l'écran, tourné vers le centre (l'acteur arme est à (30, 15, -20) de la caméra).
mc.set_editor_property('relative_location', unreal.Vector(-8, -4, 10))
mc.set_editor_property('relative_rotation', unreal.Rotator(roll=0, pitch=2, yaw=-4))
mc.set_editor_property('relative_scale3d', unreal.Vector(1.4, 1.4, 1.4))
unreal.BlueprintEditorLibrary.compile_blueprint(bp); eal.save_loaded_asset(bp)
out['prop'] = [round(v, 1) for v in prop.get_bounding_box().max.to_tuple()]
out['real'] = [round(v, 1) for v in real.get_bounding_box().max.to_tuple()]
out
