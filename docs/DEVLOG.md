# RESHOOT — dev log

Journal de fabrication, tenu par les humains et les IA. Il sert de base au dev log public (prix Glass Box).
Ajoutez les entrées **en bas**. Les captures vont dans `docs/devlog-media/`.

## Modèle d'entrée

```
### AAAA-MM-JJ HH:MM — <titre court>
- Qui : <dev> + <IA utilisée>
- Fait : <ce qui marche maintenant, en une ou deux lignes>
- Comment : <le prompt ou l'approche qui a marché, ce qui a raté avant>
- Capture : <chemin du fichier, ou "aucune">
```

Ce qui rend le journal intéressant : les échecs, les prompts qui ont débloqué une situation, les moments où une IA en a corrigé une autre, les chiffres (temps gagné, nombre d'essais).

## Journal

### 2026-11-12 — Cadrage
- Qui : Jérémy + Claude
- Fait : concept recadré d'un arena FPS 1v1/2v2 vers un FPS solo contre bots, nom RESHOOT choisi, architecture et règles pour agents écrites avant la première ligne de code.
- Comment : critique du concept initial par l'IA (multijoueur impossible à juger par un testeur seul), puis désaccord de l'équipe sur les trois styles de combat, conservés avec des simplifications.
- Capture : aucune

### 2026-11-12 18:30 — Squelette du socle
- Qui : Jérémy + Claude (Claude Code, Opus 5.5)
- Fait : module `RESHOOT` structuré en Core/Director/Medieval/Space/Noir/UI, toutes les classes d'ARCHITECTURE.md créées (headers stables), `UHealthComponent` et le déplacement complet de `AReshootCharacter` (sprint, saut, accroupi, glissade, saut de glissade, mode Space en vol) implémentés. Une arme et un ennemi vides par univers. Build Editor Win64 OK.
- Comment : l'IA a d'abord lu le projet (module nommé `RESHOOT`, pas `Reshoot`) et corrigé les docs. Un seul échec de compilation : `bCanWalkOffLedgesWhenCrouched` renommé `bCanWalkOffLedgesWhenCrouching` en 5.9. Choix notable : si aucun Input Mapping Context n'est assigné, le personnage crée ses actions Enhanced Input en C++ au lancement, pour jouer sans aucun asset. Autre piège moteur contourné : UE interdit le saut accroupi, `CanJumpInternal` est surchargé pour le saut en sortie de glissade.
- Capture : aucune

### 2026-11-12 18:45 — Socle testé en jeu par l'IA
- Qui : Jérémy + Claude (Claude Code, Opus 5.5)
- Fait : carte de test `Content/RESHOOT/Core/Maps/L_SocleTest` (sol, rebord, mur, un figurant, un WaveSpawner). Vérifié en PIE avec de vraies entrées Enhanced Input : marche 600, sprint 950, glissade (pic ~1300 puis freinage), saut de glissade (1270 conservés en l'air), accroupi 300, saut accroupi, mode Space (montée/descente, vol suivant la caméra, boost 950, inertie), chute de 30 m sans dégâts, dégâts/tête x2/bouclier frontal, mort unique, audimat +10 par kill, reprise après mort du joueur, vague de 3 figurants + OnWaveCleared, CUT!/Action!.
- Comment : le serveur MCP de l'éditeur (outils Python, InputToolset, LiveCodingToolset, capture) piloté en HTTP par l'IA. Un bug trouvé : au CUT!, les figurants posés à la main dans le niveau ne gelaient pas (seuls ceux du spawner) ; corrigé dans le .cpp et recompilé par Live Coding sans redémarrer. Faux bug évité : en Python, `unreal.Rotator(45, 0, 0)` règle le roulis, pas le tangage.
- Capture : docs/devlog-media/2026-11-12-socle-test.png

### 2026-11-12 22:00 — Second lot de headers, répartition finale
- Qui : Jérémy + Claude (Claude Code, Opus 5.5)
- Fait : un seul lot de headers calé sur les 8 directives du GDD. Les armes diffusent ce qu'elles font (tir, coup, touche, rechargement, bouclier, mode, surchauffe) ; le joueur porte les restrictions des directives (bouclier, arc, rechargement interdits, surchauffe x2) pour qu'elles survivent au changement d'arme du final ; `FKillInfo` sait si le joueur glissait ou volait (« Cascade ») ; casting, nom du joueur et bilan pour l'affiche ; valeurs d'audimat du GDD (25 / 5 / 40 / − 60, étoiles à 200-400-600-800). Headers complets du spatial écrits d'avance (laser, drone, événement de gravité) pour que Claude, Codex et Argon travaillent en parallèle sans fichier commun et sans redémarrage.
- Comment : on a relu chaque directive du GDD en se demandant « quel signal manque pour la coder sans toucher au socle ? ». Piège relevé au passage : `CustomTimeDilation = 0` ne gèle pas les timers du monde, donc un figurant gelé continuerait de tirer ; ajout de `TrackFreezableTimer` dans `AReshootEnemy`. Pas de compilation par l'IA (règle d'AGENTS.md) : relecture ligne à ligne, compilation au redémarrage de l'éditeur.
- Capture : aucune

### 2026-11-12 22:45 — Fusil laser (code)
- Qui : Jérémy + Claude (Claude Code, Opus 5.5)
- Fait : `ASpaceLaser` codé selon le GDD : hitscan 12 dégâts à 8 tirs/s avec légère dispersion, surchauffe après 3 s de tir continu (deux fois plus vite sous « Le laser coûte cher ! » via `GetCostMultiplier`), refroidissement en 1,5 s puis reprise automatique si la gâchette est tenue, zoom au clic droit, recul léger, rayon visible (cylindre étiré, cyan sans aucun asset). Boîte grise en guise de modèle. Pas encore testé en jeu : il faut d'abord le redémarrage de l'éditeur.
- Comment : le spatial a été découpé en trois lots sans fichier commun (laser pour Claude, drone pour Codex, gravité et décor pour Argon), headers écrits d'avance pour que chacun ne touche qu'à son `.cpp`.
- Capture : aucune

### 2026-11-12 23:45 — Spatial jouable sur le plateau
- Qui : Jérémy + Claude (Claude Code, Opus 5.5), seul : Jérémy a demandé à Claude de tout faire lui-même, sans Codex ni Argon.
- Fait : drone (ligne droite vers le joueur, arrêt à 5 m, tir à moins de 15 m, gel au CUT, chute pendant l'événement, mort en tournoyant) et événement « la gravité se rallume » (joueur et drones retombent 10 s, lumières d'alarme qui clignotent). `L_Stage` en boîtes grises (50 × 40 m, 4 portes avec apparitions en coulisses, mezzanines et rampes, couverts) et `L_Space` en sous-niveau (panneaux de décor avec étais, hublots, caisses, consoles, néons bleus). `DA_Universe_Space`, `BP_SpaceDrone`, `BP_ReshootGameMode`, `SS_SpaceTest`. Testé en PIE : le laser tue un drone en 4 tirs (0,5 s), tir à la tête x2, surchauffe à 2,9 s et refroidissement en 1,5 s, x2 sous « Le laser coûte cher ! », zoom 90 → 70, gel complet au CUT, gravité sur joueur et drones, audimat + 10 par kill.
- Comment : deux bugs trouvés par les tests et invisibles à la relecture : (1) le profil de collision « Pawn » ignore le canal Visibility, donc aucune trace de tir ne touchait un personnage, ni pour le joueur ni pour les ennemis ; les traces passent par type d'objet, et la tête est détectée sur le quart haut de la capsule. (2) le joueur réappliquait « aucun univers » à son apparition, après que le réalisateur avait lancé la scène. Piège de script éditeur : `unreal.Color(...)` en positionnel est en ordre B, G, R : les néons « bleus » éclairaient orange. Compilation : Build.bat une fois (éditeur fermé proprement, rien de non sauvegardé), puis Live Coding via le MCP (28 min la première fois, 2 min ensuite).
- Capture : docs/devlog-media/2026-11-12-space-stage.png
