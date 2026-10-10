"""Secuencia del promo en motion graphics (30 s). Fotogramas: frames/<M1..M5|WP|AI>/MovieFrameNNNNN.png."""
from promo_motion import AR7, BOOM, FLESH, ROTOR, C, T, burst

MAG = [(0.15, "Weapons/AR7/SW_AR7_MagOut.wav", 0.8), (0.65, "Weapons/AR7/SW_AR7_MagIn.wav", 0.8)]
CRACK = "Weapons/Bullet/SW_Bullet_Crack_01.wav"


def blood(t, g=0.7):
    return [(t, FLESH[0], g), (t + 0.05, CRACK, 0.5)]


SEQ = [
    T(0.9, "KESSRA", "2031 · REPÚBLICA DE VARANIA", "soft"),
    T(0.9, "LÍNEA NEGRA", "UNA CIUDAD PARTIDA EN DOS", "soft"),
    C("M1", 150, 1.0),
    C("WP", 58, 0.8, None, burst(0.0, 8)),
    T(0.6, "ENTRA"),
    C("M3", 355, 0.8, "BRECHA", [(0.05, BOOM[0], 0.9)]),
    C("AI", 1438, 0.8, None, burst(0.0, 4, 0.1, 0.5) + blood(0.15)),
    T(0.6, "DISPARA"),
    C("M2", 3145, 1.0, None, burst(0.05, 9)),
    C("M4", 1878, 0.9, "AMETRALLADORA", burst(0.0, 9, 0.1, 0.5)),
    C("AI", 826, 0.8, None, burst(0.0, 3) + blood(0.4)),
    T(0.6, "SOBREVIVE"),
    C("WP", 118, 1.0, "RECARGA", MAG),
    C("M1", 1893, 0.8, None, burst(0.3, 5)),
    C("M3", 1066, 0.8, None, burst(0.25, 5)),
    C("AI", 2198, 0.9, "BAJAS", burst(0.0, 6) + blood(0.35)),
    T(0.7, "5 MISIONES", "UNA SOLA CAMPAÑA"),
    C("M1", 3990, 0.9, "EXTRACCIÓN", [(0.0, ROTOR, 0.6)]),
    C("M4", 1942, 0.9, None, burst(0.05, 9, 0.1, 0.5)),
    C("M3", 510, 0.7, "TIRADOR", [(0.2, CRACK, 0.9)]),
    C("M4", 3960, 0.8, None, [(0.2, BOOM[1], 0.9)]),
    C("M2", 1838, 0.8, None, burst(0.1, 7)),
    T(0.6, "SIN MARGEN"),
    C("M5", 120, 0.8, None, burst(0.2, 5) + blood(0.6, 0.5)),
    C("M5", 2575, 0.8, None, [(0.0, ROTOR, 0.6)]),
    C("M5", 2700, 0.8, "OBJETIVO", [(0.05, BOOM[2], 0.9)]),
    # montaje final, cortes que se acortan
    C("WP", 250, 0.36, None, burst(0, 3), rapid=True),
    C("AI", 998, 0.34, None, blood(0.05)),
    C("M2", 3160, 0.32, None, burst(0, 3)),
    C("M4", 1950, 0.32, None, burst(0, 3)),
    C("AI", 1508, 0.30, None, blood(0.05)),
    C("M1", 505, 0.28),
    C("WP", 468, 0.28, None, burst(0, 3)),
    C("M3", 1073, 0.26, None, burst(0, 2)),
    C("AI", 2238, 0.26, None, blood(0.05)),
    C("M4", 1882, 0.24, None, burst(0, 2)),
    C("M2", 1848, 0.24, None, burst(0, 2)),
    C("WP", 66, 0.30, None, burst(0, 3)),
    ("black", 0.4, None),
]
_used = sum(s[1] for s in SEQ)
SEQ.append(("logo", round(30.0 - _used, 3), None))
