# RESHOOT — réglages de jeu après le test du 15/11 (Unreal MCP, execute_python) :
#   - textes des événements d'univers affichés à l'écran (DA_Medieval, DA_Universe_Space, DA_Noir)
#   - musique moins forte, vagues plus petites, final moins dense (SS_Main)
import unreal

eal = unreal.EditorAssetLibrary
out = {}

EVENTS = {
    '/Game/RESHOOT/Medieval/DA_Medieval': ('Horse convoy', 'Dodge the wooden horses on the rails!'),
    '/Game/RESHOOT/Space/DA_Universe_Space': ('Gravity is back', 'You fall to the floor for 10 s: keep shooting the drones!'),
    '/Game/RESHOOT/Noir/DA_Noir': ('Blackout', 'Lights out for 10 s: only gunfire lights the set!'),
}
for path, (title, hint) in EVENTS.items():
    d = unreal.load_asset(path)
    d.set_editor_property('event_title', title)
    d.set_editor_property('event_hint', hint)
    eal.save_loaded_asset(d)
    out[path.split('/')[-1]] = title

WAVE_SCALE = 0.65   # moins de figurants à la fois (le test trouvait le jeu très difficile)
s = unreal.load_asset('/Game/RESHOOT/Director/SS_Main')
s.set_editor_property('music_volume', 0.35)
# Les tableaux de structures sont rendus en copies : on reconstruit chaque niveau et on le réassigne.
# Repère : SS_Main d'origine = vagues de 4 à 10 figurants ; ce script ne s'applique qu'une fois (les comptes baissent à chaque passage).
new_scenes, counts = [], []
for sc in s.get_editor_property('scenes'):
    new_takes = []
    for t in sc.get_editor_property('takes'):
        new_waves = []
        for w in t.get_editor_property('waves'):
            w.set_editor_property('count', max(2, round(w.get_editor_property('count') * WAVE_SCALE)))
            new_waves.append(w)
        t.set_editor_property('waves', new_waves)
        new_takes.append(t)
        counts.append([w.get_editor_property('count') for w in new_waves])
    sc.set_editor_property('takes', new_takes)
    new_scenes.append(sc)
s.set_editor_property('scenes', new_scenes)
new_fw = []
for w in s.get_editor_property('finale_waves'):
    w.set_editor_property('count', max(3, round(w.get_editor_property('count') * WAVE_SCALE)))
    new_fw.append(w)
s.set_editor_property('finale_waves', new_fw)
fw = s.get_editor_property('finale_waves')
eal.save_loaded_asset(s)
out['waves'] = counts
out['finale'] = [w.get_editor_property('count') for w in fw]
out['music'] = s.get_editor_property('music_volume')
out
