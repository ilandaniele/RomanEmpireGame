"""Original project feedback tones, schema1: deterministic mono PCM16/24kHz.

These are synthetic UI/gameplay cues, not historical recordings or a music score.
Pure recipe module: no engine, filesystem, random state or external dependencies.
"""
import hashlib
import io
import math
import struct
import wave

SCHEMA = "1"
RATE = 24000
# (start seconds, frequency Hz, duration seconds, amplitude)
RECIPES = {
    "S_RomanSelect": ((0, 660, .10, .20), (.07, 880, .11, .16)),
    "S_RomanOrder": ((0, 440, .13, .20), (.11, 550, .14, .18)),
    "S_RomanComplete": ((0, 523.25, .35, .16), (.13, 659.25, .35, .16), (.26, 783.99, .40, .16)),
    "S_RomanImpact": ((0, 160, .13, .25), (0, 930, .10, .12), (.02, 1470, .15, .08)),
    "S_RomanVictory": ((0, 523.25, .35, .18), (.24, 659.25, .35, .18), (.48, 783.99, .35, .18), (.72, 1046.5, .55, .18)),
    "S_RomanDefeat": ((0, 392, .40, .18), (.28, 311.13, .40, .18), (.56, 261.63, .55, .18)),
}


def pcm(name):
    notes = RECIPES[name]
    frames = round((max(start + duration for start, _, duration, _ in notes) + .04) * RATE)
    result = bytearray()
    for index in range(frames):
        time = index / RATE
        sample = 0.0
        for start, frequency, duration, amplitude in notes:
            local = time - start
            if 0 <= local < duration:
                # Smooth attack/release and decaying harmonics, no clicks or clipping.
                envelope = min(1.0, local / .008, (duration - local) / .02) * math.exp(-3 * local / duration)
                phase = math.tau * frequency * local
                sample += amplitude * envelope * (math.sin(phase) + .25 * math.sin(2 * phase))
        if abs(sample) >= .8:
            raise ValueError(f"Unsafe recipe peak: {name}")
        result.extend(struct.pack("<h", round(sample * 32767)))
    return bytes(result)


def wav(name):
    output = io.BytesIO()
    with wave.open(output, "wb") as stream:
        stream.setnchannels(1)
        stream.setsampwidth(2)
        stream.setframerate(RATE)
        stream.writeframes(pcm(name))
    return output.getvalue()


def digest(name):
    return hashlib.sha256(pcm(name)).hexdigest()
