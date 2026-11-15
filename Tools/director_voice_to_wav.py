# RESHOOT — voix du réalisateur : PCM brut (Eleven v4 via fal, pcm_44100 = 16 bits mono 44,1 kHz) -> WAV.
# Entrée : SourceArt/Audio/DirectorVoice/pcm/<LineId>.pcm
# Sortie : SourceArt/Audio/DirectorVoice/SW_Dir_<LineId>.wav
# Coupe le silence au début et à la fin (garde 40 ms), normalise le pic à -1 dBFS.
# Usage : python -I Tools/director_voice_to_wav.py
import array
import pathlib
import wave

ROOT = pathlib.Path(__file__).resolve().parent.parent / 'SourceArt' / 'Audio' / 'DirectorVoice'
RATE = 44100
THRESHOLD = 300          # amplitude sous laquelle on considère que c'est du silence (sur 32767)
PAD = int(RATE * 0.04)   # marge gardée autour de la voix
PEAK = int(32767 * 0.89) # -1 dBFS

for pcm in sorted((ROOT / 'pcm').glob('*.pcm')):
    samples = array.array('h', pcm.read_bytes())
    loud = [i for i, s in enumerate(samples) if abs(s) > THRESHOLD]
    if loud:
        samples = samples[max(loud[0] - PAD, 0):min(loud[-1] + PAD, len(samples))]
    peak = max((abs(s) for s in samples), default=0)
    if peak:
        gain = PEAK / peak
        samples = array.array('h', (max(-32768, min(32767, int(s * gain))) for s in samples))
    out = ROOT / f'SW_Dir_{pcm.stem}.wav'
    with wave.open(str(out), 'wb') as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(RATE)
        w.writeframes(samples.tobytes())
    print(f'{out.name}: {len(samples) / RATE:.2f} s')
