# RESHOOT — voix du réalisateur (George, Eleven v4 via fal ; WAV faits par Tools/director_voice_to_wav.py).
# Importe SourceArt/Audio/DirectorVoice/SW_Dir_<LineId>.wav dans /Game/RESHOOT/Audio/DirectorVoice,
# puis remplit la colonne Sound de DT_DirectorLines (sous-titres et durées inchangés).
# À lancer dans l'éditeur (console Python), hors partie.
import csv
import io
import os

import unreal

at = unreal.AssetToolsHelpers.get_asset_tools()
eal = unreal.EditorAssetLibrary
SRC = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()) + 'SourceArt/Audio/DirectorVoice/'
DEST = '/Game/RESHOOT/Audio/DirectorVoice'
DT_PATH = '/Game/RESHOOT/Audio/DT_DirectorLines'

dt = unreal.load_asset(DT_PATH)
rows = list(csv.reader(io.StringIO(unreal.DataTableFunctionLibrary.export_data_table_to_csv_string(dt))))
header, body = rows[0], rows[1:]
sound_col = header.index('Sound')

tasks = []
for row in body:
    wav = SRC + f'SW_Dir_{row[0]}.wav'
    if not os.path.exists(wav):
        continue
    t = unreal.AssetImportTask()
    t.filename = wav
    t.destination_path = DEST
    t.destination_name = f'SW_Dir_{row[0]}'
    t.automated = True
    t.replace_existing = True
    t.save = True
    tasks.append(t)
at.import_asset_tasks(tasks)

linked, missing = 0, []
for row in body:
    path = f'{DEST}/SW_Dir_{row[0]}'
    if eal.does_asset_exist(path):
        row[sound_col] = f"/Script/Engine.SoundWave'{path}.SW_Dir_{row[0]}'"
        linked += 1
    else:
        missing.append(row[0])

out = io.StringIO()
csv.writer(out, lineterminator='\n').writerows([header] + body)
unreal.DataTableFunctionLibrary.fill_data_table_from_csv_string(dt, out.getvalue())
eal.save_loaded_asset(dt)
result = f'{linked} lines linked, missing: {missing}'
unreal.log(result)
