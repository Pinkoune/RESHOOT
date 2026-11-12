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
- 
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

### 2026-11-12 23:20 — Réalisateur : « CUT! » en pleine prise, audimat du GDD
- Qui : Enzo + Claude (Claude Code, Opus 5.5)
- Fait : la prise démarre sans directive ; 20 à 40 s après « Action! », « CUT! » : gel, commentaire tiré au hasard (`Cut_01`…`Cut_08`), annonce d'une directive (celle de la prise, sinon tirée dans `RandomDirectives` sans répéter la précédente), « Action! ». Audimat du GDD : kill +10, +25 si la directive est respectée, +5 sinon, +40 par prise terminée, −60 à la mort. Vérifié en PIE (CUT! réglé à 4–5 s pour le test) : « ne t'arrête jamais » puis « joue-la blessé » tirées au hasard, kills d'un joueur immobile et en pleine forme à +5, dernier kill de prise à +45, mort 100 → 40.
- Comment : écart relevé par l'IA en relisant le GDD : le « CUT! » n'arrivait qu'en fin de prise et la directive était annoncée avant l'action, ce qui retirait l'interruption au cœur du concept. Réglages ajoutés au `UShootScript` (un seul lot de headers, une recompilation complète). Reste hors GDD : le gel dure le temps des deux répliques (~6 s au lieu de 4 s max), à revoir avec les vraies voix.
- Capture : aucune

### 2026-11-12 22:30 — Médiéval : compilé, carte de test, jouable
- Qui : Ileana + Claude (Claude Code, Opus 5.5)
- Fait : `DA_Medieval` (épée, chevalier, archer), carte `Content/RESHOOT/Medieval/Test/L_MedievalTest` (sol, mur, caisses, 3 chevaliers, archer sur une tour) avec un joueur et un GameMode de test qui donnent l'épée sans passer par le réalisateur. Épée, bouclier, arc, chevaliers et archer testés en PIE : tout marche.
- Comment : Live Coding lancé par l'IA via le MCP a fait planter l'éditeur (nouvelles classes et nouvelles `UPROPERTY` : « VTable did not change »), il a fallu un build complet. Ensuite, deux éditeurs ouverts se disputaient le port 8000 du MCP ; relancé sans redémarrer avec la commande console `ModelContextProtocol.StartServer`. Leçon : nouvelles classes = build complet éditeur fermé, corps de fonctions = Live Coding.
- Capture : aucune

### 2026-11-12 23:00 — Médiéval : épée, bouclier, arc, chevalier, archer (code)
- Qui : Ileana + Claude (Claude Code, Opus 5.5)
- Fait : `AMedievalSword` (épée en cône 2,5 m / 50 dégâts / 1,5 coup par s, bouclier qui passe par `SetFrontalBlock(0.2)`, arc chargé 40→100 dégâts en 1 s), `AMedievalArrow` (projectile commun au joueur et à l'archer, gelé au CUT!), `AMedievalKnight` (poursuite navmesh ou ligne droite, coup préparé de 0,35 s, mort en physique avec heaume qui s'envole), `AMedievalArcher` (fixe, flèche lente avec anticipation partielle). Tout en boîtes grises avec les formes de base du moteur. Pas encore compilé ni testé.
- Comment : conflit repéré entre le GDD (deux armes médiévales) et l'architecture (une `WeaponClass` par univers) : résolu sans toucher au socle avec une seule arme à deux modes, qui ajoute elle-même son contexte Enhanced Input (1, 2, molette). Le déplacement du socle est réutilisé tel quel ; seul manque une API pour bloquer le sprint bouclier levé, demandée au socle.
- Capture : aucune
- 
### 2026-11-12 23:30 — Médiéval : sous-niveau L_Medieval, combat à l'arc
- Qui : Ileana + Claude (Claude Code, Opus 5.5)
- Fait : `L_Medieval` (71 éléments en formes de base, 5 matériaux de couleur) : rempart en panneau peint avec créneaux, étais visibles derrière, porte et bannières ; chemin de ronde à 4 m avec parapet, relié au sol par deux rampes de 11 m (glissade) ; deux tours inaccessibles ; 3 archers (centre du chemin de ronde, haut des tours) ; tonneaux, charrettes et bottes de paille comme couverts. Comme `L_Noir` : pas de sol ni de lumière, prévu pour être chargé dans `L_Stage`. Ajouté en sous-niveau de `L_MedievalTest`.
- Comment : vérifié en PIE par l'IA via le MCP : chaque archer tombe en une flèche chargée tirée depuis le sol (il faut viser la tête au-dessus du parapet), et un chevalier monte la rampe pour rejoindre le joueur sur le chemin de ronde. Trois corrections après test : les créneaux avant des tours bloquaient les flèches (retirés), le parapet fermait le haut des rampes (ouvert), et le navmesh ne reliait pas la rampe au chemin de ronde tant que la reconstruction n'avait pas fini.
- Capture : docs/devlog-media/2026-11-12-medieval-l-medieval.png

### 2026-11-12 23:50 — Médiéval : gravité plus lourde, arc tenu à gauche
- Qui : Ileana + Claude (Claude Code, Opus 5.5)
- Fait : `DA_Medieval` passe à `GravityScale` 1,5 (saut de 1,96 m à 1,31 m, 0,84 s en l'air : on ne flotte plus, les couverts de 1 m restent franchissables). La carte de test lit enfin `DA_Medieval` via un petit script de tournage `SS_MedievalTest`. L'arc en boîte grise n'est plus un bâton vertical au centre de l'écran : 50 cm, tenu à gauche, incliné, recule quand on bande.
- Comment : réglage par univers prévu par l'architecture, sans toucher au déplacement du socle. Piège : récupérer le travail des autres (rebase sur le Noir de Jules, qui ajoutait une classe) rend Live Coding dangereux ; il faut un build complet juste après un pull qui touche des headers.
- Capture : aucune

### 2026-11-13 00:15 — Médiéval : la carte de test devient un plateau de tournage
- Qui : Ileana + Claude (Claude Code, Opus 5.5)
- Fait : `L_MedievalTest` est maintenant un hangar de studio de 80 × 60 m et 14 m de haut autour du décor médiéval : murs nus, toit et poutres, 4 portes de plateau, scotch jaune au sol qui délimite le plateau de 50 × 40 m, passerelles à 9 m, trois grills avec 8 projecteurs chauds qui éclairent le décor, deux caméras sur pied, un travelling sur rails avec sa caméra, le fauteuil du réalisateur avec moniteur et clap, quatre boîtes à lumière sur pied, câbles au sol, flight cases, panneaux « SILENCE ! ON TOURNE ». Exposition fixée par un post-process pour garder l'ambiance « décor éclairé dans un hangar sombre ».
- Comment : tout en formes de base et 7 nouveaux matériaux `MI_Studio_*`, construit par l'IA via le MCP en trois scripts Python, puis vérifié en capture et en PIE (surexposition corrigée, navmesh reconstruit sur tout le hangar). Ce n'est pas `L_Stage` (réservé au socle) : c'est une maquette dans le dossier Médiéval, qui peut servir de référence pour le vrai plateau.
- Capture : docs/devlog-media/2026-11-12-medieval-studio-vue.png, docs/devlog-media/2026-11-12-medieval-studio-joueur.png

### 2026-11-13 00:45 — Médiéval : combos de points, bouclier qui ne traverse plus l'épée
- Qui : Ileana + Claude (Claude Code, Opus 5.5)
- Fait : combos sur `AMedievalSword` : chaque coup d'épée ou flèche du joueur qui touche prolonge le combo, multiplicateur +1 tous les 3 coups (max x5), 10 points par coup, +50 par kill, +25 par tir à la tête. Le combo retombe après 2,5 s sans toucher ou dès que le joueur est blessé, et se fige pendant le CUT!. Affichage provisoire en haut de l'écran ; pour le HUD : `OnComboChanged`, `GetCombo`, `GetComboMultiplier`, `GetComboScore`, `GetComboTimeLeftPct`. Bouclier levé : plaque en bas à gauche, épée baissée sur le côté, plus de lame qui traverse le bouclier. Positions de l'arc, du bouclier et de l'épée en garde désormais réglables dans l'éditeur.
- Comment : correction du bouclier en Live Coding lancé par l'IA via le MCP, sans redémarrage. Les combos demandaient un header : un seul build complet, avec tous les changements de header regroupés. Test en PIE : 1er coup sur 3 chevaliers = combo 3, x2, 40 pts ; 2e coup qui les tue = combo 6, x3, 460 pts ; 3 archers abattus à l'arc = +60, +60, +120. Piège de test : le temps de jeu avance moins vite que le script Python quand l'éditeur n'a pas le focus, il faut attendre en temps de jeu (`GetTimeSeconds`).
- Capture : aucune

### 2026-11-13 00:00 — Réalisateur : Médiéval branché, partie complète Médiéval → Noir → final
- Qui : Enzo + Claude (Claude Code, Opus 5.5)
- Fait : branche `medieval` fusionnée, `L_Medieval` en sous-niveau de `L_Stage`, scène Médiéval en tête de `SS_Main` (2 prises, « corps-à-corps » imposée sur la deuxième), final qui alterne Médiéval et Noir. Vérifié en PIE de bout en bout : épée et gravité 1,5 puis revolver et noir et blanc, sous-niveaux qui se remplacent, CUT! et directives à chaque prise, final qui bascule, écran de fin (665 d'audimat). Final corrigé : ses vagues tournent en boucle (56 figurants en 30 s au test), une mort pendant le final renvoie au départ sans relancer de prise, plus de vague relancée par la fin de partie.
- Comment : les trous du final sont sortis en suivant le test à la trace (univers, arme, sous-niveaux visibles, gravité). Raté de l'IA : pour tester vite, elle a modifié `SS_Main` en mémoire et sa « sauvegarde » Python n'était qu'une référence vers le même tableau ; le script a été reconstruit à l'identique. Les tests suivants passent par un script transitoire jamais sauvegardé.
- Capture : aucune

### 2026-11-13 00:30 — Réalisateur : portes du plateau, étoiles du GDD
- Qui : Enzo + Claude (Claude Code, Opus 5.5)
- Fait : `L_Stage` a 6 portes (2 au nord, 2 au sud, 2 à l'est de part et d'autre du rempart médiéval) avec un point d'apparition derrière chacune. Le `WaveSpawner` n'utilise plus une porte dans le dos du joueur à moins de 10 m (`MinDistanceBehindPlayer`). Étoiles recalées sur le GDD (200 / 400 / 600 / 800). Vérifié en PIE : joueur collé à 5 m de la porte nord-ouest, dos tourné, 20 figurants sortis par les 5 autres portes et aucun par celle-ci.
- Comment : murs reconstruits en segments + linteaux par script Python dans l'éditeur. Pour ne plus faire fermer/compiler/relancer l'éditeur à la main à chaque header, l'IA a un script local (hors git) qui attend la fermeture, compile et relance l'éditeur, et lance Live Coding elle-même par MCP pour les `.cpp`.
- Capture : aucune

### 2026-11-13 00:50 — Réalisateur : répliques du final, des reprises et de la fin
- Qui : Enzo + Claude (Claude Code, Opus 5.5)
- Fait : le réalisateur joue `Intro_Finale` à l'entrée du final, une réplique `Switch_xx` à chaque bascule d'univers, une réplique `Retake_xx` différente à chaque mort, et `Wrap_Bad` au lieu de `Wrap` quand la partie finit à 2 étoiles ou moins. Listes et seuil réglables dans `UShootScript`. Vérifié en PIE en écoutant `OnSubtitle` : Switch_01/03 aux bascules, Retake/Retake_02/05/03 aux morts, Wrap_Bad à 1 étoile.
- Comment : première recompilation complète faite entièrement par l'IA : sauvegarde et fermeture de l'éditeur par MCP, script local qui attend la fermeture, compile et relance l'éditeur (~15 s de redémarrage), puis vérification que les nouveaux réglages existent. Zéro manipulation humaine.
- Capture : aucune

### 2026-11-13 01:40 — Réalisateur : Spatial branché, branche `space` fusionnée, partie complète à trois univers
- Qui : Enzo + Claude (Claude Code, Opus 5.5)
- Fait : branche `space` de Jérémy fusionnée dans `dev-enzo`. API du réalisateur alignée sur son second lot de headers (`IsKillRespected`, `DirectivePool` filtré par univers, `CutDelayMin/Max`, `TakeClearedBonus`, `DeathPenalty`, `OnKillScored`, statistiques de partie, `ResetForTake` à la reprise) avec l'implémentation testée d'Enzo dessous. `L_Stage` d'Enzo gardé, `L_Space` ajouté en sous-niveau ; `SS_Main` dans l'ordre du GDD (Médiéval, Spatial, Noir) et final sur les trois. Directives restreintes : corps-à-corps en Médiéval, pas de recharge en Noir, du style en Médiéval et Noir. Vérifié en PIE de bout en bout : épée et gravité 1,5, puis laser en vol sans gravité dans `L_Space`, puis revolver dans `L_Noir`, CUT! et directives tirées selon l'univers, final Médiéval → Spatial → Noir → Médiéval, fin de partie (920, 5 étoiles avec un joueur qui tue tout instantanément).
- Comment : Jérémy et Enzo avaient codé en parallèle les mêmes idées sous des noms différents ; fusion faite en prenant les noms de Jérémy (le socle et le HUD s'appuient dessus). Deux `L_Stage` binaires : celui d'Enzo gardé (celui de Jérémy, 50 × 40 m avec mezzanines, reste dans l'historique de `origin/space`). La fusion a d'abord échoué parce que l'éditeur verrouillait `L_Stage.umap` : l'IA a sauvegardé, fermé l'éditeur, refait la fusion, compilé et relancé l'éditeur elle-même. Deux erreurs de compilation nées de la rencontre du nouveau socle et des univers, corrigées au minimum hors périmètre (voir « Demandes au socle » dans TASKS).
- Capture : aucune
