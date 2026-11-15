# RESHOOT — bruitages du film noir synthétisés (numpy, aucune source extérieure) : tir du revolver et des gangsters,
# clic à vide, recharge, coupure et retour du courant. Lancé dans Blender (Python avec numpy), depuis la racine du dépôt.
# Écrit les WAV dans SourceArt/Noir/Audio ; Tools/setup_noir_sounds.py les importe et les branche.
import numpy as np, wave, os
R = 44100
OUT = os.environ.get("NOIR_SFX_OUT", os.path.join(os.getcwd(), "SourceArt", "Noir", "Audio"))
rng = np.random.default_rng(7)

def t_(d): return np.arange(int(d * R)) / R
def noise(d): return rng.uniform(-1, 1, int(d * R))
def lp(x, cutoff):
    n = max(3, int(R / cutoff))
    k = np.hanning(n); k /= k.sum()
    return np.convolve(x, k, mode='same')
def hp(x, cutoff): return x - lp(x, cutoff)
def add(*xs):
    n = max(len(x) for x in xs); out = np.zeros(n)
    for x in xs: out[:len(x)] += x
    return out
def place(buf, x, at):
    i = int(at * R); j = min(len(buf), i + len(x)); buf[i:j] += x[:j - i]; return buf
def save(name, x, gain=0.9):
    x = np.tanh(x / (np.max(np.abs(x)) + 1e-9) * 1.4)
    x = x / np.max(np.abs(x)) * gain
    data = (x * 32767).astype(np.int16)
    with wave.open(os.path.join(OUT, name + '.wav'), 'wb') as w:
        w.setnchannels(1); w.setsampwidth(2); w.setframerate(R); w.writeframes(data.tobytes())
    print(name, round(len(x) / R, 2), 's')

def gunshot(d=0.9, crack=1.0, body_cut=1800, boom_f=(130, 45), tail=0.35, tail_gain=0.3, distant=False):
    t = t_(d)
    x = crack * hp(noise(d), 3000) * np.exp(-t / 0.004)
    x += 0.9 * lp(noise(d), body_cut) * np.exp(-t / 0.05)
    f = boom_f[1] + (boom_f[0] - boom_f[1]) * np.exp(-t / 0.03)
    x += 0.8 * np.sin(2 * np.pi * np.cumsum(f) / R) * np.exp(-t / 0.09)
    x += tail_gain * lp(noise(d), 700) * np.exp(-t / tail)                      # écho de la ruelle
    x += 0.15 * lp(noise(d), 400) * np.exp(-((t - 0.18) ** 2) / 0.002)          # retour sur les façades
    if distant:
        x = lp(x, 2500)
    return x

def click(d=0.05, freq=3200, sharp=0.003, gain=1.0):
    t = t_(d)
    return gain * (hp(noise(d), 2500) * np.exp(-t / sharp) + 0.5 * np.sin(2 * np.pi * freq * t) * np.exp(-t / (sharp * 2)))

def thunk(d=0.15, f=90, decay=0.04):
    t = t_(d)
    return np.sin(2 * np.pi * f * t) * np.exp(-t / decay) + 0.3 * lp(noise(d), 600) * np.exp(-t / 0.02)

save('SW_Noir_Revolver_Fire', gunshot())
save('SW_Noir_Gangster_Fire', gunshot(d=1.0, crack=0.6, body_cut=1400, boom_f=(110, 40), tail=0.45, tail_gain=0.4, distant=True), gain=0.75)
save('SW_Noir_Revolver_Empty', place(np.zeros(int(0.25 * R)), add(click(0.06, 2600, 0.002), 0.4 * thunk(0.06, 300, 0.01)), 0.0), gain=0.6)

buf = np.zeros(int(1.5 * R))
place(buf, add(click(0.05, 1800, 0.004), 0.6 * thunk(0.08, 160, 0.02)), 0.05)        # barillet qui s'ouvre
for k in range(6):                                                                  # six cartouches
    place(buf, click(0.04, 2400 + 150 * k, 0.003, 0.5), 0.3 + 0.11 * k + rng.uniform(-0.01, 0.01))
place(buf, add(click(0.06, 1500, 0.005), thunk(0.12, 120, 0.03)), 1.25)                # barillet refermé
save('SW_Noir_Revolver_Reload', buf, gain=0.7)

def hum(d, f0, f1, env):
    t = t_(d); f = f0 + (f1 - f0) * (t / d)
    ph = 2 * np.pi * np.cumsum(f) / R
    s = np.sin(ph) + 0.5 * np.sin(2 * ph) + 0.3 * np.sign(np.sin(2 * ph)) * 0.3
    return lp(s, 900) * env(t)
buf = np.zeros(int(2.0 * R))
place(buf, add(thunk(0.3, 70, 0.08), click(0.05, 1200, 0.006)), 0.0)                 # gros disjoncteur
place(buf, 0.6 * hum(1.6, 120, 40, lambda t: np.exp(-t / 0.5)), 0.02)              # le courant meurt
save('SW_Noir_Blackout_Down', buf, gain=0.85)
buf = np.zeros(int(2.0 * R))
place(buf, add(thunk(0.3, 80, 0.06), click(0.05, 1400, 0.006)), 0.0)
t = t_(1.8)
flick = (np.sin(2 * np.pi * 9 * t) > -0.2).astype(float) * np.minimum(1, t / 0.6)  # néons qui grésillent en revenant
place(buf, 0.5 * hum(1.8, 60, 120, lambda tt: flick[:len(tt)] * np.exp(-np.maximum(0, tt - 1.2) / 0.3)), 0.05)
save('SW_Noir_Blackout_Up', buf, gain=0.8)
