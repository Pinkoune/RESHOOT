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

### 2026-11-12 21:00 — Film noir : revolver, gangster, répliques
- Qui : Jules + Claude (Claude Code, Opus 5.5)
- Fait : `ANoirRevolver` (hitscan coup par coup, chargeur de 6, recharge R ou auto à vide, annulée au gel), `ANoirGangster` (reste à couvert, se tourne et tire quand il voit le joueur, probabilité de toucher, timer mis en pause au CUT!). 40 répliques du réalisateur écrites dans `Content/RESHOOT/Audio/DirectorLines.csv`, prêtes à importer en `DT_DirectorLines` (colonne Sound vide en attendant les voix).
- Comment : tout le code en un seul lot de headers pour un seul redémarrage de l'éditeur. Gangster sans navmesh : il est placé derrière une caisse, le « couvert » vient du level design. Emplacements de sons (tir, vide, recharge) prévus dès maintenant pour brancher l'audio samedi sans toucher aux headers. Voix pas encore générées : plus de crédits ElevenLabs, on attend les crédits fal du jam.
- Capture : aucune

### 2026-11-12 21:40 — Film noir : assets créés par l'IA dans l'éditeur
- Qui : Jules + Claude (Claude Code, Opus 5.5)
- Fait : `M_PP_Noir` (post-process noir et blanc : SceneTexture PostProcessInput0 → Desaturation à 1 → Emissive), `DA_Noir` (univers Noir, revolver, gangster, sous-niveau `L_Noir`, post-process), `L_Noir` (5 caisses, 3 gangsters derrière les 3 premières, tournés vers le centre).
- Comment : premier poste sans compilateur : installation des Build Tools 2022 (MSVC 14.44, car UE 5.9 bannit le MSVC 14.51 de VS 2026) puis du .NET Framework 4.8 SDK oublié (erreur `NetFxSDK` à la compilation). Le serveur MCP de l'éditeur ne répondait pas sur ce poste : l'IA a pris la main sur l'éditeur et lancé un script Python depuis la console de l'Output Log (mode Python), qui crée les trois assets.
- Capture : aucune

### 2026-11-12 22:00 — Film noir testé en jeu
- Qui : Jules + Claude (Claude Code, Opus 5.5)
- Fait : carte `L_NoirTest` (sol, lumière, filtre N&B, `L_Noir` en sous-niveau, GameMode RESHOOT). Vérifié en PIE : les gangsters tuent un joueur immobile en ~14 s, le revolver fait 40 dégâts et 80 à la tête, recharge automatique à vide. `DT_DirectorLines` créée depuis le CSV (40 lignes, 0 problème à l'import). Gangsters désormais visibles : cylindre, tête et chapeau de gangster en formes de base.
- Comment : tests pilotés par l'IA depuis la console Python de l'éditeur (lancer la PIE, équiper le revolver, viser une tête, tirer, lire les vies). Deux bugs trouvés : les balles traversaient les gangsters (le profil Pawn ignore le canal Visibility du tir de base, signalé au socle) et les figurants étaient invisibles (aucun mesh). Corrigés côté Noir.
- Capture : docs/devlog-media/2026-11-12-noir-test.jpg

### 2026-11-12 22:15 — Film noir aligné sur le GDD
- Qui : Jules + Claude (Claude Code, Opus 5.5)
- Fait : gangster conforme au GDD (80 PV, rafales de 3 balles à 15 toutes les 2 s, 450 cm/s) : il rejoint la caisse la plus proche, reste baissé derrière, se relève pour tirer, fait le tour de sa caisse si le joueur le contourne et court vers un autre couvert si le joueur approche à moins de 6 m ; il touche moins un joueur qui bouge. Revolver à 45 dégâts, clic droit = visée avec zoom, éclair de lumière à chaque tir, API pour la directive « Plan-séquence ». Événement « coupure de courant » (`ANoirBlackoutEvent`) : lumières éteintes 10 s, exposition figée, seuls les tirs éclairent.
- Comment : tout vérifié en PIE depuis la console Python : gangsters baissés/relevés, fuite de la caisse 1 à la caisse 4 quand le joueur est téléporté à côté. Premier essai de coupure raté : lumières à 2 % mais l'exposition manuelle rendait la scène presque aussi claire ; réglé en direct (lumières à 0, éclairs de 3000 cd) avant de figer les valeurs.
- Capture : docs/devlog-media/2026-11-12-noir-coupure.jpg

### 2026-11-13 08:00 — Premiers assets générés dans le spatial
- Qui : Jérémy (génération Tripo / Rodin) + Claude (Claude Code, Opus 5.5) pour l'import
- Fait : 8 modèles importés (fusil laser, caisse, console, bidons, panneau de coursive, hublot, néon, lampe d'alarme) avec matériaux PBR, tailles réelles et collisions simples. `BP_SpaceLaser` porte le fusil ; `L_Space` est habillé (panneaux, hublots, caisses, consoles, néons, alarmes, 4 bidons en plus). Les sources brutes (OBJ + textures) sont rangées dans `SourceArt/Space/`, hors de `Content/`.
- Comment : les OBJ arrivent Y-haut et en mètres normalisés (~1 unité) : import par Interchange avec un pipeline surchargé (rotation de 90° en roulis, échelle par objet pour la taille réelle). Piège : l'import crée bien le matériau PBR avec les 4 textures, mais laisse la normal map désactivée (poids 0) : réactivée partout. Le fusil (Rodin) n'avait pas de `.mtl` : matériau monté à la main, émissif compris. Autre piège découvert : après un redémarrage de l'éditeur, les correctifs passés par Live Coding sont perdus tant qu'on n'a pas recompilé pour de vrai.
- Capture : aucune

### 2026-11-13 09:00 — Recompilation et drones qui passent les murs
- Qui : Jérémy + Claude (Claude Code, Opus 5.5)
- Fait : build complet de l'éditeur (correctifs d'hier rendus permanents, code film noir de Jules compilé). Vérifié en PIE sur `L_Stage` sans rien forcer : scène spatiale lancée au démarrage avec `BP_SpaceLaser`, tir à la tête 24 / corps 12, drones qui blessent le joueur. Ajout d'un évitement minimal aux drones : un mur devant eux, ils montent par-dessus (le plateau n'a pas de plafond). Les 4 drones atteignent le joueur en moins de 7 s, quelle que soit leur porte.
- Comment : le test a montré que les drones nés derrière une porte latérale restaient collés au mur, puisqu'ils volent en ligne droite sans navmesh. Un balayage de sphère de 4 m devant eux suffit.
- Capture : aucune

### 2026-11-13 23:30 — « On améliore la mise en scène ! » : du carton au réaliste
- Qui : Jérémy (idée, GDD) + Claude (Claude Code, Opus 5.5)
- Fait : la signature visuelle du jeu marche en jeu. Chaque scène commence dans un studio en carton : lumière plate, projecteurs sur pied, caméras, perche, passerelles, techniciens, panneaux de décor en carton, fusil en carton. Un rideau de lumière balaie le plateau en 2,5 s avec une lisière orange et des étincelles. Derrière lui, le studio disparaît et le spatial apparaît à la même place : ciel étoilé, planète, néons froids, vrais objets, vrai fusil (avec un flash quand le rideau passe sur le joueur). Gameplay inchangé : la collision reste sur le blockout.
- Comment : un seul matériau maître pour tout le décor (`M_SetDressing`). Chaque pixel compare sa position à celle du rideau, lue dans une Material Parameter Collection. Le carton s'affiche devant, le réaliste derrière, avec un fondu tramé et une lisière émissive. Les deux versions sont simplement superposées. Matériau, collection, textures et niveaux montés par script Python via le MCP de l'éditeur, sans un clic. Pièges : les dossiers de l'éditeur n'existent plus dans le jeu packagé (le rideau lit des tags) ; `DitherTemporalAA` n'est plus un nœud mais une fonction de matériau en UE5 ; le ciel, très loin, n'était jamais atteint par le rideau (il bascule désormais d'un coup, au passage du rideau sur le centre). Défaut restant : l'exposition automatique surexpose l'image pendant le balayage.
- Capture : docs/devlog-media/2026-11-13-dressing-1-avant.png, docs/devlog-media/2026-11-13-dressing-2-pendant.png, docs/devlog-media/2026-11-13-dressing-3-apres.png

### 2026-11-14 00:30 — Le spatial a ses deux habillages
- Qui : Jérémy (génération Tripo / Rodin des versions réalistes) + Claude (Claude Code, Opus 5.5) pour l'import
- Fait : 8 modèles réalistes importés (caisse, console, bidons, panneau, hublot, néon, alarme, fusil) avec Nanite, recentrés sur leur base et mis exactement à la taille de leur version carton. Les anciens modèles deviennent les `_Prop` (teinte carton). Dans `L_Space`, chaque objet a son jumeau réaliste posé au même endroit : 27 paires + 8 panneaux. Le fusil change de modèle au passage du rideau.
- Comment : le panneau réaliste était debout (plus haut que large) ; il est couché à l'import par une double rotation, puis recentré automatiquement (le script mesure la boîte englobante et réimporte avec le bon décalage). Piège : une PIE restée ouverte bloquait silencieusement imports et duplications (« The Editor is currently in a play mode »). Les modèles réalistes font environ 185 000 triangles : Nanite activé, textures 4K plafonnées à 2K.
- Capture : aucune

### 2026-11-14 10:00 — Spatial habillé jusqu'à l'horizon
- Qui : Jérémy (Tripo, fal) + Claude (Claude Code, Opus 5.5)
- Fait : drone en carton et en réaliste (il change au passage du rideau, ceux qui arrivent après sont réalistes). Studio réel pendant la phase carton : caméras de cinéma, perche micro, 24 éléments de passerelle, câbles, sacs de sable, flight cases. Après le rideau : sol et murs de vaisseau texturés posés sur le blockout, ciel avec nébuleuse, coque géante et planète en plans peints.
- Comment : un second matériau maître, `M_SetDressing_Tiled`, répète la texture dans le monde (projection sur les trois axes) : une image carrée de fal suffit pour 50 m de sol sans étirement. Les images lointaines utilisent un matériau additif : le fond noir disparaît, pas besoin de détourage. Le revêtement est une copie du blockout gonflée de 3 cm, créée par script à partir de `L_Stage`. Premier réglage trop chargé (motif mural répété tous les 5 m, effet papier peint) : passé à 12 m.
- Capture : docs/devlog-media/2026-11-14-space-habille.png

### 2026-11-14 15:00 — « C'est horrible, il faut tout refaire »
- Qui : Jérémy (verdict, choix des kits Fab) + Claude (Claude Code, Fable 5.2)
- Fait : le spatial a été refait de zéro. Avant : un cube de 50 × 40 m aux murs plats de 12 m, des objets générés posés par coordonnées, six lampes identiques. Verdict de Jérémy : trop moche, tout refaire. Après : un vrai plateau de tournage plongé dans le noir (gril de poutres, projecteurs suspendus et sur pied, rideaux noirs, décor de châssis en carton), qui devient au passage du rideau un pont d'envol sous les étoiles (murs bas, sol en dalles, sas d'où sortent les drones, planète au-dessus des murs). 120 images par seconde, 77 pendant le balayage.
- Comment : l'IA a reconnu qu'elle savait faire la technique mais pas composer un décor. Trois décisions ont tout changé. (1) Des kits modulaires Fab (autorisés par le règlement s'ils sont déclarés, non notés) plutôt que des textures sur des cubes : un générateur Python pose 1 400 modules sur la grille de 2 m du kit, à partir du blockout. (2) Le blockout devient invisible : on ne voit plus que de l'habillage. (3) La lumière raconte quelque chose : noir de plateau et faisceaux chauds, puis lumière froide rasante et sas rouges, avec une exposition figée. Les modules du kit n'utilisent pas le matériau du rideau : ils apparaissent un par un quand le rideau passe sur eux, ce qui donne un effet de construction. Les couverts sont enfin habillés à l'endroit exact de leur collision.
- Capture : docs/devlog-media/2026-11-14-plateau-1-studio.png, docs/devlog-media/2026-11-14-plateau-2-rideau.png, docs/devlog-media/2026-11-14-plateau-3-spatial.png

### 2026-11-14 16:00 — Hangar de station, deuxième version
- Qui : Jérémy (verdict : « pas réaliste ») + Claude (Claude Code, Opus 5.5)
- Fait : le hangar est un vrai lieu, fermé : baie vitrée sur la planète au nord, coursives éclairées derrière les quatre portes, quais sur les mezzanines, plafond à fermes métalliques et luminaires. Sol, murs et plafond refaits avec le kit Fab le plus réaliste des trois (`Sci_fi_hallway` : panneaux métalliques, éclairages intégrés), fermes du kit Hollywood. Blockout corrigé : les portes est et ouest débouchaient dans le bloc des mezzanines (1 m de passage). Chaque objet libre est désormais contrôlé contre les zones de passage avant d'être posé.
- Comment : on a d'abord fixé le lieu (« un hangar de station », déjà écrit dans le GDD) avant de poser quoi que ce soit, puis étudié comment la démo du kit assemble ses pièces (hauteurs, pivots, orientation) au lieu de deviner. Le générateur refuse un objet qui coupe une rampe ou bouche une porte, et le rapport arrive avant les captures. Correctif de performance : le rideau re-traitait à chaque image les 3 700 acteurs déjà basculés (15 images par seconde pendant le balayage, environ 38 après correction).
- Capture : docs/devlog-media/2026-11-14-hangar-kit.png
