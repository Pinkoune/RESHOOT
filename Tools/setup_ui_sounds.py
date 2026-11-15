# RESHOOT — sons des menus, synthétisés par Tools/synth_ui_sfx.py (Python pur, aucune source extérieure).
# Sources : SourceArt/UI/Audio/*.wav, importées dans /Game/RESHOOT/UI/Audio. AReshootHUD les trouve par leur chemin.
# Le ronron du projecteur boucle. À lancer dans l'éditeur (console Python), hors partie.
import unreal

at = unreal.AssetToolsHelpers.get_asset_tools()
eal = unreal.EditorAssetLibrary
SRC = unreal.Paths.project_dir() + 'SourceArt/UI/Audio/'
DEST = '/Game/RESHOOT/UI/Audio'
out = {}
for name in ['SW_UI_Clap', 'SW_UI_Hover', 'SW_UI_Click', 'SW_UI_Whoosh', 'SW_UI_Projector']:
    t = unreal.AssetImportTask(); t.filename = SRC + name + '.wav'; t.destination_path = DEST; t.destination_name = name
    t.automated = True; t.replace_existing = True; t.save = True
    at.import_asset_tasks([t])
    s = unreal.load_asset(f'{DEST}/{name}')
    if s and name == 'SW_UI_Projector':
        s.set_editor_property('looping', True)
        eal.save_loaded_asset(s)
    out[name] = bool(s)
out
