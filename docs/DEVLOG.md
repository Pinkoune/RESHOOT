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

### 2026-11-12 — Réalisateur : sous-niveaux, post-process, retour au départ
- Qui : Enzo + Claude (Claude Code, Opus 5.5)
- Fait : `UShootDirectorSubsystem::SetUniverse` affiche le sous-niveau de l'univers (`SubLevelName`) et décharge ceux des autres univers du script ; applique le `PostProcessMaterial` de l'univers sur un volume sans borne créé à la volée. À la reprise après la mort, le joueur revient au PlayerStart dont le Player Start Tag est le nom de l'univers (« Noir »), sinon à son point d'apparition.
- Comment : tout dans le `.cpp`, aucun header touché, pour rester en Live Coding. Pas de membre ajouté : le volume est retrouvé par un tag d'acteur, les sous-niveaux par leur nom (préfixe PIE retiré).
- Capture : aucune

### 2026-11-12 22:25 — Plateau L_Stage et premier tournage complet
- Qui : Enzo + Claude (Claude Code, Opus 5.5)
- Fait : `L_Stage` (plateau 60 × 60 m, sol à Z=0, murs, ciel, PlayerStart, WaveSpawner, NavMesh, `L_Noir` en sous-niveau non chargé au départ), `SS_Main` (scène Noir : 2 prises, final 60 s), `BP_StageGameMode` (lance le tournage tout seul) et `BP_StageGameState` (branche `DT_DirectorLines`, jusque-là reliée à rien : aucun sous-titre ne sortait). Vérifié en PIE : le réalisateur charge `L_Noir`, applique le filtre noir et blanc, donne le revolver, lance la vague ; le joueur tué à 20 m du départ y revient avec sa vie remise à 100.
- Comment : tout créé par l'IA via le serveur MCP de l'éditeur (Python). Le premier PIE a révélé un bug d'ordre : le BeginPlay du joueur passait après celui du GameMode et remettait son univers à None (pas d'arme). Corrigé côté réalisateur en lançant la première scène une frame plus tard, recompilé par Live Coding. Constat : les gangsters tirent déjà pendant l'intro de la scène (4,5 s), un joueur immobile meurt avant le « Action! ».
- Capture : aucune

### 2026-11-12 22:35 — Réalisateur : gel pendant l'intro de scène
- Qui : Enzo + Claude (Claude Code, Opus 5.5)
- Fait : pendant l'annonce du réalisateur, joueur et figurants sont gelés ; « Action! » dégèle. Le sous-niveau de l'univers est chargé tout de suite (`FlushLevelStreaming`) pour que ses gangsters soient gelés eux aussi. Vérifié en PIE : 3 gangsters gelés et joueur à 100 PV de 0 à 4,5 s, premiers dégâts ~2,5 s après « Action! ».
- Comment : sans le chargement immédiat, les gangsters du sous-niveau arrivaient après l'appel au gel et tiraient pendant l'intro. Mesure prise par l'IA en échantillonnant toutes les 0,4 s (vie du joueur, figurants gelés) depuis la console Python de l'éditeur.
- Capture : aucune

### 2026-11-12 23:05 — Réalisateur : 8 directives
- Qui : Enzo + Claude (Claude Code, Opus 5.5)
- Fait : 8 sous-classes de `UDirective` (tirs à la tête, ne t'arrête jamais, corps-à-corps, joue-la blessé, plus vite, pas un seul coup, pas de recharge, du style), un asset chacune dans `Content/RESHOOT/Director/Directives/`. Kill qui respecte la directive ×2, sinon ×0,25 (réglables par asset). Le réalisateur annonce la nouvelle directive pendant le gel, puis « Action! ». Branchées dans `SS_Main` : prise 2 « tirs à la tête », nouvelle prise 3 « plus vite ». Vérifié en PIE sur 20 kills : prise 1 +10 par kill, prise 2 +20 à la tête et +2,5 au corps, prise 3 +20 pour des kills rapprochés ; enchaînement jusqu'au final.
- Comment : nouvelles classes = un seul lot de headers et un redémarrage. Deux pièges : relancer l'éditeur ne recompile pas un module modifié quand Live Coding a déjà tourné (l'ancienne DLL était rechargée, les classes n'existaient pas et l'asset créé sortait en DataAsset vide) ; puis `C4458` traité en erreur (variable locale `Player` qui masquait un membre de la classe de base). « Pas de recharge » repère la recharge par un timer qui surveille les munitions, sans toucher aux armes du socle.
- Capture : aucune
