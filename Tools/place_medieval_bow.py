# RESHOOT — place l arc recourbé Fab dans la main (repos et tir) : debout, courbure vers l avant, legerement incline.
# Calcule la pose depuis la boite englobante du modele. A lancer dans l editeur (console Python), hors partie.
import sys
YAW = float(globals().get('YAW', 90.0))
sw = unreal.load_asset('/Game/RESHOOT/Medieval/BP_MedievalSword')
sc = unreal.get_default_object(sw.generated_class())
mesh = unreal.load_asset('/Game/Fab/Recurve_Bow/recurve_bow/StaticMeshes/Object_45')
bb = mesh.get_bounding_box(); c = (bb.min + bb.max) * 0.5
s = 75.0 / (bb.max.z - bb.min.z)
def make(loc, roll):
    rot = unreal.Rotator(roll=roll, pitch=0.0, yaw=YAW)
    off = unreal.MathLibrary.greater_greater_vector_rotator(c * s, rot)
    return unreal.Transform(loc - off, rot, unreal.Vector(s, s, s))
rest = make(unreal.Vector(100, -36, -2), -12.0)
draw = make(unreal.Vector(92, -16, 2), -5.0)
sc.set_editor_property('bow_real_rest_transform', rest)
sc.set_editor_property('bow_real_draw_transform', draw)
unreal.BlueprintEditorLibrary.compile_blueprint(sw); unreal.EditorAssetLibrary.save_loaded_asset(sw)
[str(rest.rotation.rotator()), str(rest.translation), s]
