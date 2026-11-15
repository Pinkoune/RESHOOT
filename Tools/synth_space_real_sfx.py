# RESHOOT — sons réalistes du Spatial synthétisés (numpy, aucune source extérieure) : tir du fusil laser,
# tir des drones, deux sorties d'air des réacteurs. Lancé dans Blender (Python avec numpy), depuis la racine du dépôt.
# Écrit les WAV dans SourceArt/Space/Audio/Real ; Tools/setup_space_real_sounds.py les importe à la place des provisoires.
import numpy as np, wave, os
R = 44100
OUT = os.environ.get("SPACE_SFX_OUT", os.path.join(os.getcwd(), "SourceArt", "Space", "Audio", "Real"))
rng = np.random.default_rng(11)

def t_(d): return np.arange(int(d * R)) / R
def noise(d): return rng.uniform(-1, 1, int(d * R))
def lp(x, cutoff):
    n = max(3, int(R / cutoff)); k = np.hanning(n); k /= k.sum()
    return np.convolve(x, k, mode='same')
def hp(x, cutoff): return x - lp(x, cutoff)
def bp(x, lo, hi): return hp(lp(x, hi), lo)
def add(*xs):
    n = max(len(x) for x in xs); out = np.zeros(n)
    for x in xs: out[:len(x)] += x
    return out
def echo(x, delay, gain, n=3):
    out = np.concatenate([x, np.zeros(int(delay * R * n))])
    for k in range(1, n + 1):
        i = int(delay * R * k); out[i:i + len(x)] += x * gain ** k
    return out
def save(name, x, gain=0.9):
    x = np.tanh(x / (np.max(np.abs(x)) + 1e-9) * 1.3); x = x / np.max(np.abs(x)) * gain
    fade = min(len(x), int(0.01 * R)); x[-fade:] *= np.linspace(1, 0, fade)
    with wave.open(os.path.join(OUT, name + '.wav'), 'wb') as w:
        w.setnchannels(1); w.setsampwidth(2); w.setframerate(R); w.writeframes((x * 32767).astype(np.int16).tobytes())
    print(name, round(len(x) / R, 2), 's')

def zap(d, f0, f1, curve, body, sub, crack=0.6, fm=0.0):
    t = t_(d)
    f = f1 + (f0 - f1) * np.exp(-t / curve)                       # fréquence qui chute
    ph = 2 * np.pi * np.cumsum(f) / R
    mod = np.sin(2 * np.pi * 47 * t) * fm
    tone = np.sin(ph + mod) + 0.35 * np.sign(np.sin(ph * 0.5)) * 0.6 + 0.25 * np.sin(2 * ph)
    x = tone * np.exp(-t / body)
    x += sub * np.sin(2 * np.pi * np.cumsum(60 + 90 * np.exp(-t / 0.03)) / R) * np.exp(-t / 0.07)   # coup sourd
    x += crack * hp(noise(d), 4000) * np.exp(-t / 0.006)          # claquement d'attaque
    return x

# Fusil laser : décharge énergique, chute de 2,4 kHz à 180 Hz, petit écho métallique du hangar
laser = zap(0.45, 2400, 180, 0.05, 0.12, 0.7, crack=0.5, fm=1.5)
save('SW_Space_LaserFire_Real', echo(lp(laser, 9000), 0.085, 0.28))
# Tir de drone : plus aigu, plus sec, plus lointain
drone = zap(0.3, 3200, 600, 0.03, 0.07, 0.25, crack=0.3, fm=3.0)
save('SW_Space_DroneFire_Real_1', echo(lp(drone, 6000), 0.11, 0.22), gain=0.75)

# Réacteurs : sorties d'air comprimé (souffle filtré, attaque rapide, sifflement qui retombe)
def thrust(d, lo, hi, decay, hiss):
    t = t_(d)
    env = np.minimum(1, t / 0.015) * np.exp(-t / decay)
    x = bp(noise(d), lo, hi) * env
    x += hiss * bp(noise(d), 3500, 9000) * np.exp(-t / (decay * 0.5))
    x += 0.3 * lp(noise(d), 180) * env                              # grondement
    return x
save('SW_Space_Thruster_Real_1', thrust(0.55, 500, 2500, 0.18, 0.35), gain=0.7)
save('SW_Space_Thruster_Real_2', thrust(0.7, 300, 1600, 0.25, 0.25), gain=0.7)
