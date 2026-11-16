# RESHOOT — partie raccourcie pour les testeurs de la jam (5 à 10 minutes en tout, casting compris).
# Chaque scène : 2 prises de 2 vagues (la 1re prise garde le CUT! de mise en scène, la 2e l'événement d'univers),
# figurants qui arrivent vite, CUT! en pleine prise plus tôt, final d'une minute. Public moins impatient.
# À lancer dans l'éditeur (console Python), hors partie. Rejouable : réécrit les valeurs à chaque fois.
import unreal

ss = unreal.load_asset('/Game/RESHOOT/Director/SS_Main')

def wave(count, interval=0.3, delay=0.5):
    w = unreal.WaveDef()
    w.set_editor_property('count', count)
    w.set_editor_property('spawn_interval', interval)
    w.set_editor_property('start_delay', delay)
    return w

WAVES = [[(4, 0.5), (5, 0.8)], [(5, 0.5), (6, 0.8)]]
scenes = []
for s in ss.get_editor_property('scenes'):
    old = list(s.get_editor_property('takes'))
    takes = []
    for i, spec in enumerate(WAVES):
        t = old[min(i, len(old) - 1)]
        t.set_editor_property('waves', [wave(c, 0.3, d) for c, d in spec])
        takes.append(t)
    s.set_editor_property('takes', takes)
    s.set_editor_property('event_take_index', 1)
    s.set_editor_property('event_delay', 8.0)
    scenes.append(s)
ss.set_editor_property('scenes', scenes)
ss.set_editor_property('finale_waves', [wave(5, 0.3, 0.5), wave(6, 0.3, 0.8)])
for k, v in {
    'finale_duration': 60.0, 'finale_switch_interval': 15.0,
    'cut_delay_min': 12.0, 'cut_delay_max': 20.0, 'staging_cut_delay': 12.0,
    'base_kill_rating': 15.0, 'directive_kill_rating': 35.0, 'ignored_directive_kill_rating': 8.0,
    'take_cleared_bonus': 50.0, 'audience_bored_delay': 9.0, 'audience_bored_rate': 6.0,
}.items():
    ss.set_editor_property(k, v)
unreal.EditorAssetLibrary.save_loaded_asset(ss)
[(len(s.get_editor_property('takes')), [len(t.get_editor_property('waves')) for t in s.get_editor_property('takes')]) for s in ss.get_editor_property('scenes')]
