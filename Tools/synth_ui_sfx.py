# RESHOOT — bruitages des menus synthétisés en Python pur (aucune source extérieure, aucun paquet) :
# clap du clap de cinéma, survol et clic des boutons, souffle de transition, ronron de projecteur en boucle.
# Lancé depuis la racine du dépôt : python Tools/synth_ui_sfx.py. Écrit les WAV dans SourceArt/UI/Audio ;
# Tools/setup_ui_sounds.py les importe dans /Game/RESHOOT/UI/Audio.
import math, os, random, struct, wave

R = 44100
OUT = os.path.join(os.getcwd(), "SourceArt", "UI", "Audio")
os.makedirs(OUT, exist_ok=True)
rng = random.Random(11)


def n(d):
    return int(d * R)


def noise(d):
    return [rng.uniform(-1, 1) for _ in range(n(d))]


def lowpass(x, cutoff):
    a = 1 - math.exp(-2 * math.pi * cutoff / R)
    y, s = [], 0.0
    for v in x:
        s += a * (v - s)
        y.append(s)
    return y


def highpass(x, cutoff):
    l = lowpass(x, cutoff)
    return [a - b for a, b in zip(x, l)]


def env(x, decay, attack=0.0):
    out = []
    for i, v in enumerate(x):
        t = i / R
        a = min(1.0, t / attack) if attack > 0 else 1.0
        out.append(v * a * math.exp(-t / decay))
    return out


def tone(d, f, decay):
    return [math.sin(2 * math.pi * f * i / R) * math.exp(-i / R / decay) for i in range(n(d))]


def mix(*xs):
    m = max(len(x) for x in xs)
    out = [0.0] * m
    for x in xs:
        for i, v in enumerate(x):
            out[i] += v
    return out


def place(buf, x, at):
    i0 = n(at)
    for i, v in enumerate(x):
        if i0 + i < len(buf):
            buf[i0 + i] += v
    return buf


def save(name, x, gain=0.9, drive=1.3):
    peak = max(abs(v) for v in x) or 1.0
    x = [math.tanh(v / peak * drive) for v in x]
    peak = max(abs(v) for v in x) or 1.0
    data = b"".join(struct.pack("<h", int(v / peak * gain * 32767)) for v in x)
    with wave.open(os.path.join(OUT, name + ".wav"), "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(R)
        w.writeframes(data)
    print(name, round(len(x) / R, 2), "s")


# Clap : deux planchettes de bois qui claquent (attaque sèche + résonance courte du bois).
d = 0.35
clap = mix(
    env(highpass(noise(d), 2500), 0.006),
    [0.9 * v for v in env(lowpass(noise(d), 1800), 0.025)],
    [0.5 * v for v in tone(d, 820, 0.03)],
    [0.3 * v for v in tone(d, 1730, 0.018)],
    [0.25 * v for v in tone(d, 190, 0.05)],
)
save("SW_UI_Clap", clap, gain=0.95, drive=1.6)

# Survol : petit tic feutré.
d = 0.06
save("SW_UI_Hover", mix(env(highpass(noise(d), 3000), 0.003), [0.6 * v for v in tone(d, 2100, 0.008)]), gain=0.45)

# Clic : déclic de projecteur (deux temps).
d = 0.18
buf = [0.0] * n(d)
place(buf, mix(env(highpass(noise(0.05), 2000), 0.004), [0.7 * v for v in tone(0.05, 1300, 0.01)]), 0.0)
place(buf, [0.6 * v for v in mix(env(highpass(noise(0.05), 1500), 0.005), tone(0.05, 700, 0.015))], 0.055)
save("SW_UI_Click", buf, gain=0.7)

# Souffle : bruit filtré qui enfle puis retombe (transition vers le casting).
d = 1.1
x = noise(d)
lo, hi = lowpass(x, 600), lowpass(x, 3500)
whoosh = []
for i in range(n(d)):
    t = i / n(d)
    shape = math.sin(math.pi * min(1.0, t * 1.3)) ** 2
    whoosh.append((lo[i] * (1 - t) + hi[i] * t) * shape)
save("SW_UI_Whoosh", whoosh, gain=0.6, drive=1.1)

# Projecteur : cliquetis à 24 images/s sur un ronron de moteur, en boucle de 4 s (24 cliquetis par seconde, entier).
d = 4.0
buf = [0.0] * n(d)
hum = lowpass(noise(d), 160)
for i in range(len(buf)):
    t = i / R
    buf[i] += 0.5 * hum[i] + 0.12 * math.sin(2 * math.pi * 50 * t) + 0.05 * math.sin(2 * math.pi * 100 * t)
tick = mix(env(highpass(noise(0.02), 1800), 0.002), [0.4 * v for v in tone(0.02, 950, 0.004)])
for k in range(int(d * 24)):
    place(buf, [v * rng.uniform(0.55, 0.8) for v in tick], k / 24 + rng.uniform(-0.0015, 0.0015))
save("SW_UI_Projector", buf, gain=0.5, drive=1.0)
