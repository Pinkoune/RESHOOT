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

### 2026-11-13 01:15 — Médiéval : combat aux poings (touche 3), bouclier centré
- Qui : Ileana + Claude (Claude Code, Opus 5.5)
- Fait : 3e mode de l'arme médiévale, les poings : coups gauche-droite (3 par seconde, 20 dégâts, portée 1,6 m) qui repoussent le figurant, garde aux poings qui bloque 50 % des dégâts de face, coups comptés dans les combos. Touche 3, et la molette fait le cycle épée → arc → poings. Bouclier levé recentré. L'arme passe par `BP_MedievalSword` (dans `DA_Medieval`) pour régler valeurs et positions sans recompiler.
- Comment : le recentrage du bouclier s'est fait sans compilation, en surchargeant une valeur dans un Blueprint enfant. Les poings demandaient un header (nouveau mode, deux composants) : un build complet, puis vérification en PIE via le MCP (touche 3 → mode Poings ; un coup = 20 dégâts, 1,6 m de recul, combo 1 ; garde = 10 dégâts sur 20 ; molette = épée, arc, poings). Le coup d'épée et le coup de poing partagent maintenant la même détection en cône (`MeleeSweep`).
- Capture : aucune

### 2026-11-13 01:45 — Médiéval : l'événement du convoi de chevaux de bois
- Qui : Ileana + Claude (Claude Code, Opus 5.5)
- Fait : `AMedievalHorseConvoyEvent` (dérivé de `AUniverseEvent`) : rails permanents en travers du plateau, convoi de 3 chevaux de bois peints sur plateformes à roulettes poussés par 2 techniciens en noir. Au déclenchement, le convoi trépigne 1,5 s en bout de rail puis traverse à 11 m/s et renverse tout : joueur 25 dégâts et projeté sur le côté, figurants fauchés. Repart dans l'autre sens au passage suivant, se fige au CUT!. Posé dans `L_Medieval` (deux tonneaux déplacés hors de la voie) et branché dans `DA_Medieval.EventClass`. Les aides visuelles de test (cônes de l'arme, sphères des chevaliers) sont coupées par défaut.
- Comment : vérifié en PIE via le MCP : les 3 chevaliers posés sur la voie sont fauchés l'un après l'autre, le joueur prend 25 et est projeté à 5 m, l'événement s'arrête seul en bout de rail. Le réalisateur ne déclenche pas encore les événements d'univers : demande au socle ajoutée ; en attendant, le convoi se relance toutes les 20 s (`AutoStartInterval`, à remettre à 0 quand le réalisateur s'en chargera).
- Capture : docs/devlog-media/2026-11-13-medieval-convoi.png

### 2026-11-13 02:30 — Interface : HUD de jeu
- Qui : Ileana + Claude (Claude Code, Opus 5.5)
- Fait : `AReshootHUD` (dossier UI), dessiné au Canvas sans asset, placé comme dans le GDD : REC clignotant et « Scène, prise » en haut à gauche, directive en haut au centre, audimat 0-1000 en haut à droite (pulse et « + 25 » à chaque gain, paliers d'étoiles), score et combo médiéval dessous, vie en bas à gauche, arme en bas à droite (munitions, modes médiévaux, charge de l'arc, garde), sous-titres du réalisateur en bas au centre, réticule selon l'arme, marqueur de touche, bord rouge quand on est touché, bandes noires et « CUT ! » pendant le cut. Branché sur le GameMode de test médiéval ; demande au socle pour l'utiliser dans le vrai GameMode.
- Comment : l'interface n'appelle aucune logique, elle écoute les delegates (audimat, sous-titres, vie du joueur et des figurants) et lit les getters. Piège : Live Coding a compilé le correctif mais Windows (Smart App Control, erreur 0xc0e90002) a bloqué le fichier du patch ; il a fallu un build complet. Autre piège : `FString::ToUpper` ne met pas « é » en majuscule (« ÉPéE »), on garde donc les noms d'armes tels quels.
- Capture : docs/devlog-media/2026-11-13-hud.png

### 2026-11-13 02:50 — Réalisateur : HUD et sous-titres sur le plateau
- Qui : Enzo + Claude (Claude Code, Opus 5.5)
- Fait : branche `interface` (HUD d'Ileana) fusionnée ; `AReshootHUD` assigné à `BP_StageGameMode`, donc actif dans `L_Stage` sans toucher au GameMode du socle. Vérifié en PIE : sous-titres du réalisateur en bas au centre, « Scène 1 – Prise 1 » et REC, audimat, vie, arme.
- Comment : le HUD appelait `AMedievalSword::GetMode()` en attendant le type énuméré, renommé `GetMedievalMode()` à la fusion de `space` : 3 appels corrigés dans `UI/`. L'épée a gagné un 3e mode (poings) entre-temps : `GetModeCount` passé à 3. Fermeture, fusion, compilation et relance de l'éditeur faites par l'IA, capture d'écran prise par MCP pendant la PIE.
- Capture : docs/devlog-media/2026-11-13-stage-hud-soustitres.png

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

### 2026-11-14 17:00 — Portes réparées, planète en 3D
- Qui : Jérémy (« les portes sont buguées, la planète fait PNG ») + Claude (Claude Code, Opus 5.5)
- Fait : portes du hangar corrigées (on voyait les étoiles par le plafond de la coursive au-dessus de la porte du fond, panneau de linteau étiré, balise qui rougissait tout le mur). La planète et la coque, qui étaient des images à plat, sont remplacées par une vraie planète en 3D : sphère avec océans, continents, nuages, côté jour et côté nuit, atmosphère lumineuse sur le bord.
- Comment : matériau procédural (bruit 3D pour les continents et les nuages, éclairage calculé dans le matériau). Premier essai en neige de télévision : le nœud de bruit ignorait la position mise à l'échelle et lisait la position monde brute, des dizaines de milliers de fois trop fine ; corrigé en réglant l'échelle sur le nœud lui-même.
- Capture : docs/devlog-media/2026-11-14-hangar-planete-3d.png

### 2026-11-14 21:00 — Fini l'extérieur « PS2 », un vrai fusil laser
- Qui : Jérémy + Claude (Claude Code, Opus 5.5)
- Fait : ciel et planète calculés pixel par pixel (étoiles nettes, voie lactée discrète ; planète avec océans, côtes, déserts, glace, nuages en bandes, reflet du soleil, villes côté nuit, atmosphère au bord). Fusil laser réaliste : le fusil d'Epic (modèle FPS du moteur) avec des bandes lumineuses qui chauffent, animé en code (recul, retard à la rotation, balancement, visée, levée après le CUT, inclinaison en surchauffe). Tirs du joueur et des drones : rayon cœur + halo, trait qui file vers l'impact, éclair de bouche avec lumière, impact lumineux, brûlure qui refroidit.
- Comment : verdict de Jérémy, « l'extérieur fait très PS2 ». Le flou ne venait pas que de la texture : le brouillard du hangar s'appliquait aussi à la planète, à 42 km. Il est maintenant coupé au-delà de 150 m (`fog_cutoff_distance`). La planète est un calcul rayon-sphère dans un nœud Custom HLSL : bord parfaitement rond quelle que soit la sphère support. Premier essai : nuages cotonneux ; ils sont devenus nets avec un seuil plus serré et un bruit de détail. Côté effets, tout est dans un composant partagé (`USpaceLaserFXComponent`), en pool, qui ne tique que pendant un tir. Trois ratés vus sur captures : une boule blanche devant la caméra quand le drone touche le joueur, plus d'effet d'impact dans ce cas ; une brûlure invisible parce que le décal ne descendait que de 4 cm alors que le sol visible est au-dessus du sol de collision ; des étincelles Niagara du modèle en gerbes de 2 m, désactivées.
- Capture : `docs/devlog-media/2026-11-14-planete-procedurale.png`, `docs/devlog-media/2026-11-14-fusil-laser.png`, `docs/devlog-media/2026-11-14-tirs-drones.png`

### 2026-11-14 21:45 — Le carton reste du carton, fusil laser Fab, transition réparée
- Qui : Jérémy + Claude (Claude Code, Opus 5.5)
- Fait : en carton, le fusil tire de nouveau un simple tube gris, sans visée, sans animation ni effet ; les drones en carton aussi. Le réaliste prend le fusil laser Fab ajouté par Jérémy, plus grand et plus à gauche, avec une visée plus dégagée. Le passage en réaliste à la prise 2 remarche, le final se joue en réaliste, et le matériel du studio (fermes, projecteurs, caméras) disparaît au passage du rideau au lieu d'attendre la fin du balayage.
- Comment : la transition « cassée » venait du script de test `SS_SpaceTest`, revenu à une seule prise : l'ajout de la prise 2 n'avait jamais été enregistré. Le déroulé est maintenant vérifié en PIE sans forcer le rideau (log « scene 0 reached take 1 -> upgrade »). Les fermes basculaient en retard parce que leurs matériaux (kit Hollywood) ignorent le rideau : elles ne disparaissaient qu'à la fin, quand tout le carton est masqué. Elles sont maintenant taguées pour basculer une à une. Le fusil Fab est un mesh sans repères : orientation et position du canon lues sur une capture de profil.
- Capture : aucune

### 2026-11-14 22:15 — Le fusil tenu comme dans COD
- Qui : Jérémy + Claude (Claude Code, Opus 5.5)
- Fait : arme plus proche, plus grande, partant du coin bas-droit et tournée vers le centre. En visée, elle se met dans l'axe et le guidon arrière vient à 16 cm de l'œil : on regarde le long du dessus de l'arme. Le fusil laser en carton (Tripo, à partir du prompt du guide) est importé, calé sur le réaliste (1 m, même pivot, même bout de canon).
- Comment : retour de Jérémy, « pas dans la même position que les FPS auxquels j'ai l'habitude ». Premier essai de visée raté : redresser l'arme en la tournant autour de l'acteur la décalait de 20 cm sur le côté. La visée est maintenant une pose à part, calculée pour poser le viseur sur l'axe de la caméra, vers laquelle on fond depuis la pose en main. Deux points de visée testés sur capture : le premier mettait la crosse en plein milieu de l'écran.
- Capture : aucune

### 2026-11-14 23:00 — Tirs dans l'axe du canon, fusil en carton plus petit
- Qui : Jérémy + Claude (Claude Code, Opus 5.5)
- Fait : arme décalée vers la droite. Le canon est maintenant orienté pour que son axe croise le centre de l'écran à 10 m : le tir sort tout droit du canon vers le viseur, en carton comme en réaliste. Le fusil en carton a sa propre pose, plus petite (échelle 0,92 au lieu de 1,15).
- Comment : avant, le canon était tourné de 7° vers le centre et croisait l'axe de la caméra à 30 cm ; le rayon repartait donc en biais depuis le bout du canon. L'orientation est calculée dans `Tools/build_laser_fx.py` (`aim_pose`), vérifiée sur captures avec des rayons ralentis.
- Capture : aucune

### 2026-11-14 23:20 — Fusil réaliste pivoté vers la gauche
- Qui : Jérémy + Claude (Claude Code, Opus 5.5)
- Fait : le fusil réaliste en main est pivoté de 5° vers la gauche en plus de l'orientation vers le centre (`extra_yaw` dans `Tools/build_laser_fx.py`). Le carton et la visée ne changent pas.
- Comment : 3° et 6° testés sur capture ; à 5°, le rayon reste dans le prolongement du canon à l'œil.
- Capture : aucune

### 2026-11-14 23:45 — Fusil réaliste décollé du bord, plus coupé par la caméra
- Qui : Jérémy + Claude (Claude Code, Opus 5.5)
- Fait : fusil réaliste en main décollé du bord droit, rapproché de la caméra et presque droit face à elle (pose 2 parmi trois proposées en capture). Plan de coupe proche passé de 10 à 2 cm (`NearClipPlane` dans `Config/DefaultEngine.ini`) : l'arrière du fusil n'est plus coupé au bord de l'écran.
- Comment : le fusil fait plus d'un mètre et sa crosse passe derrière la caméra ; à 10 cm, le plan de coupe tranchait le tube lumineux encore visible en bas à droite. Testé en jeu avec `r.SetNearClipPlane 2` avant de l'écrire dans la config (pris en compte au prochain lancement).
- Capture : aucune

### 2026-11-15 00:00 — Fusil réaliste encore rapproché
- Qui : Jérémy + Claude (Claude Code, Opus 5.5)
- Fait : fusil réaliste rapproché de 8 cm de la caméra (X -17 → -25) : il paraissait trop long. Essai à -32 sur capture, jugé trop envahissant.
- Comment : `POSE` dans `Tools/build_laser_fx.py`, enregistré dans `BP_SpaceLaser`.
- Capture : aucune

### 2026-11-15 00:20 — Le fusil réagit à la direction de déplacement
- Qui : Jérémy + Claude (Claude Code, Opus 5.5)
- Fait : en réaliste, le fusil penche (5°) et traîne (1 cm) du côté du pas de côté, recule de 1,5 cm et baisse un peu le canon en avançant. Atténué de 75 % en visée. Le carton reste fixe.
- Comment : calculé dans `ASpaceLaser::Tick` à partir de la vitesse du personnage dans son repère, rapportée à sa vitesse max (elle monte et descend progressivement, pas besoin de lissage). Réglages en constantes dans `SpaceLaser.cpp` pour rester compatible Live Coding ; à passer en `UPROPERTY` au prochain changement de header.
- Capture : aucune

### 2026-11-15 00:50 — Sons du carton faits à la bouche (Spatial)
- Qui : Jérémy + Claude (Claude Code, Opus 5.5)
- Fait : en décor carton seulement, tir du fusil, tir des drones et bouffées de réacteur en apesanteur, enregistrés à la bouche par Jérémy (deux prises par son, une tirée au hasard, hauteur variée de ±8 à 10 %). Réacteurs : une bouffée dès qu'on appuie sur une touche de déplacement ou Espace, puis toutes les 0,7 s tant qu'on la garde. Le réaliste reste muet en attendant ses sons. Réglages d'animation du fusil (déplacement) passés en `UPROPERTY`.
- Comment : nouvelles propriétés `PropFireSounds` (`ASpaceLaser`, `ASpaceDrone`) et `PropThrusterSounds` (`AReshootCharacter`), `FireSound` réservé au réaliste. Le joueur n'avait pas de Blueprint : `BP_ReshootCharacter` (enfant de `AReshootCharacter`, assets seulement) est créé et devient le pion de `BP_ReshootGameMode`. Branchement par `Tools/setup_prop_sounds.py`. mp3 sources dans `SourceArt/Space/Audio/Prop/`, sortis de `Content`. Compilation complète, câblage vérifié en jeu (pion, sons assignés au fusil, aux 4 drones et au joueur) ; pas d'écoute possible de mon côté.
- Capture : aucune

### 2026-11-15 01:20 — Sons provisoires du réaliste, réacteurs à l'appui seulement
- Qui : Jérémy + Claude (Claude Code, Opus 5.5)
- Fait : sons provisoires du réaliste branchés (tir du fusil, tir des drones, deux sorties d'air). Les réacteurs sont des sorties d'air : un son par appui sur une touche de direction (nouvelle touche ou changement de sens), plus de répétition tant qu'on garde la touche, plus rien sur Espace.
- Comment : `RealThrusterSounds` ajouté à `AReshootCharacter` (`PropThrusterInterval` retiré), axe précédent mémorisé et remis à zéro sur `Completed` de l'action de déplacement. mp3 dans `SourceArt/Space/Audio/Real/`, assets dans `/Game/RESHOOT/Space/Audio/Real`, branchés par `Tools/setup_prop_sounds.py`. Compilation complète, câblage vérifié en jeu, pas d'écoute de mon côté.
- Capture : aucune

### 2026-11-15 01:45 — Drones en réaliste, propulseurs moins forts
- Qui : Jérémy + Claude (Claude Code, Opus 5.5)
- Fait : `SS_SpaceTest` avait de nouveau une seule prise et un final sans vague : après la première prise, on passait au final en réaliste sans aucun drone. Prise 2 rajoutée (6 drones, en réaliste) et vague de final (6 drones, remplacée à chaque changement d'univers). Sorties d'air : volume à 0,4 sur les quatre sons, et aussi jouées à l'appui de Espace (montée) et Ctrl (descente).
- Comment : l'ajout de la prise 2 du 14/11 n'avait pas été enregistré sur disque ; cette fois l'asset est bien modifié (vérifié par `git status`). Vérifié en jeu : 6 drones réalistes visibles à la prise 2.
- Capture : aucune

### 2026-11-14 22:30 — Réalisateur terminé : casting, événements d'univers, directives du GDD, transition carton
- Qui : Enzo + Claude (Claude Code, Opus 5.5)
- Fait : casting jouable (fond vert derrière le plateau, glissade sur une rampe puis 3 cibles en carton, le réalisateur lance la scène 1) ; événements d'univers déclenchés en prise 2 (annonce, déclenchement 3 s après, gel au CUT!) ; les 8 directives du GDD dont 4 qui interdisent une action (« Sans le bouclier ! », « Range cet arc ! », « Pas de coupure ! », « Le laser coûte cher ! ») ; « On améliore la mise en scène ! » à la fin de la 1re prise (paramètre de matériau + tags d'acteurs) ; une réplique ne coupe plus jamais l'autre (file d'attente, abandon au-delà de 4 s) ; un CUT! par prise garanti même si le joueur vide les vagues avant ; `SS_Main` à 3 prises par scène, vagues de 4 à 10 figurants comme le GDD. Vérifié en PIE de bout en bout, casting compris, avec de vraies entrées clavier simulées (sprint + accroupi pour la glissade, visée et tir sur les cibles) : convoi, gravité et coupure de courant déclenchés, bouclier bloqué, coût du laser ×2, rideau 0 → 1 en 2,5 s, partie jusqu'au final en 7,5 min avec un tueur automatique.
- Comment : le serveur MCP de l'éditeur ne se reconnectait plus après le redémarrage de la session ; l'IA l'a piloté directement en HTTP (JSON-RPC) depuis un petit client Python. Premier essai du casting raté : les balles passaient au-dessus des cibles, dont le panneau était centré sur le pied de l'acteur (à moitié dans le sol) ; corrigé en posant le panneau sur un pivot au sol. Faux bug évité : un écouteur Python de sous-titres se détache à la fin de son script, la file d'attente marchait.
- Capture : aucune

### 2026-11-14 23:45 — Casting : un vrai studio de cinéma (L_Casting)
- Qui : Enzo + Claude (Claude Code, Opus 5.5)
- Fait : le couloir vert a été remplacé par un plateau de cinéma de 40 × 26 × 12 m dans son propre sous-niveau `L_Casting` (chargé par le réalisateur au début du casting, déchargé à la scène 1). Le joueur part d'une estrade, glisse sur un toboggan à LED (« GLISSE ! » peint dessus), remonte un tapis rouge entre poteaux en laiton et cordons de velours, jusqu'à un cyclo vert à marques de tracking sous une enseigne « CASTING » à 50 ampoules, où l'attendent trois cibles en carton peintes. Tout autour : treillis d'éclairage avec barreaux et diagonales sur tours au sol, palans à chaîne, 12 projecteurs à lyre et volets, faisceaux dans la brume, passerelles techniques et échelle à crinoline, gaines de ventilation, écrans verts mobiles, coin en écran bleu, butterfly de soie, panneaux LED, grue de caméra, caméra de cinéma sur rails, chaise du réalisateur, régie vidéo, perche, miroirs de loge à ampoules, affiches des trois films, flight-cases, apple boxes, sacs de lest, témoins « SILENCE ! ON TOURNE ». Vérifié en PIE : chargement, glissade (pic 1 385), trois cibles, retour au plateau, déchargement.
- Comment : premier jet (couloir vert à ciel ouvert) jugé « ultra nul » à juste titre ; refait entièrement par script Python dans l'éditeur avec un matériau maître et 30 instances, en formes de base mais composé et éclairé comme un studio. Itéré sur captures : brume trop dense qui délavait l'image, panneaux muraux tournés de 90°, exposition automatique qui surexposait la salle en jeu (passée en manuel), centre des cibles étiré en barre rouge parce qu'il héritait de l'échelle du panneau. 4 115 pièces fusionnées en 33 maillages pour rester fluide.
- Capture : docs/devlog-media/2026-11-14-casting-studio.png, docs/devlog-media/2026-11-14-casting-cyclo.png, docs/devlog-media/2026-11-14-casting-tournage.png

### 2026-11-15 00:10 — Glissade : touche C en plus de Ctrl
- Qui : Enzo + Claude (Claude Code, Opus 5.5)
- Fait : la glissade demandait Maj (sprint) et Ctrl (accroupi) en même temps, deux touches sous le même petit doigt. C fait maintenant aussi accroupi/glissade (petit doigt sur Maj, index sur C). La glissade part aussi dès que le joueur va à la vitesse du sprint, même si Maj vient d'être relâché. Aide peinte sur le toboggan : « MAJ + C ». Vérifié en PIE avec la vraie touche C : glissade (pic 1 370).
- Comment : remarqué par Enzo en jouant ; les tests automatiques injectaient les actions sans doigts et ne pouvaient pas le voir. Reste : relâcher Maj puis appuyer sur C dans la foulée ne suffit pas (la vitesse retombe sous le seuil en 0,04 s) ; un délai de grâce demanderait une variable de plus dans `ReshootCharacter.h`.
- Capture : aucune

### 2026-11-15 02:10 — Fusion de space dans main
- Qui : Jérémy + Claude (Claude Code, Opus 5.5)
- Fait : la branche space (hangar réaliste, transition carton/réaliste, fusil laser et ses effets, sons du carton et provisoires du réaliste, joueur en Blueprint) est fusionnée dans main avec le travail d'Enzo (casting, directives, sous-niveaux Médiéval et Noir, `BP_StageGameMode`). Vérifié en jeu sur la partie complète : casting, scène médiévale, puis scène spatiale en carton à la prise 1 et en réaliste à la prise 2, drones présents.
- Comment : deux conflits. `DEVLOG.md` : les entrées des deux branches sont gardées. `L_Stage.umap` (binaire, modifié des deux côtés) : on part de la version d'Enzo, puis les 15 acteurs propres à space (rideau `SetDressingSweep`, 14 projecteurs du studio) y sont recopiés depuis la version space, chargée en sous-niveau temporaire. Les points d'entrée des drones restent ceux d'Enzo. `AReshootGameMode` prend `BP_ReshootCharacter` comme pion par défaut, pour que `BP_StageGameMode` (qui en hérite) ait les sons de réacteur sans toucher au dossier Director. Bug révélé par la fusion et corrigé : à l'intro d'une scène, le réalisateur d'Enzo garde encore l'index de prise de la scène précédente, et le rideau passait la nouvelle scène directement en réaliste ; il ne suit plus l'index que pendant les prises.
- Capture : aucune

### 2026-11-15 03:00 — Médiéval : décor carton, puis prairie fantasy en extérieur après le rideau
- Qui : Ileana + Claude (Claude Code, Opus 5.5)
- Fait : 19 modèles générés (Tripo, Rodin) importés à la taille du guide, pivot au sol, textures et instance de `M_SetDressing` (`Tools/import_medieval_assets.py`). `L_Medieval` reconstruit sur le modèle de `build_hangar.py` (`Tools/build_medieval_set.py`, relançable) : les volumes de jeu deviennent un blockout invisible avec collision ; carton = château en carton peint, tonneaux, charrettes, bottes de paille, plancher de studio, et matériel de tournage réaliste (3 caméras, perche, 6 projecteurs sur pied, passerelles, grille de poutres avec projecteurs, câbles, sacs de sable, flight cases) ; réaliste = remparts en vraie pierre, enceinte qui cache les murs gris du plateau, herbe 3D (Light_Foliage, 489 touffes), buissons, rochers, chemins de terre (kit Farm Paths), montagnes enneigées sur 4 panneaux à 32 km, nuages volumétriques, exposition propre à chaque phase. Les rails du convoi disparaissent au passage du rideau. Le réalisateur déclenche lui-même le convoi : relance automatique coupée.
- Comment : vérifié en PIE via le MCP (univers médiéval, `ResetDressing`, captures, `UpgradeDressing`, captures). Ratés : une exposition manuelle trop forte a rendu tout l'éditeur blanc ; le matériau de fond de l'espace est additif, donc invisible sur un ciel de jour (nouveau `M_Medieval_Backdrop`, le noir de l'image devient transparent) ; les panneaux de montagnes ont d'abord affiché l'image couchée, puis à l'envers ; le kit de chemins est arrivé sans textures (matériau de terre maison).
- Capture : docs/devlog-media/2026-11-15-medieval-carton.png, docs/devlog-media/2026-11-15-medieval-reel.png, docs/devlog-media/2026-11-15-medieval-montagnes.png

### 2026-11-15 15:00 — Médiéval réaliste : textures partout, herbe dense, enceinte sans trous
- Qui : Ileana + Claude (Claude Code, Opus 5.5)
- Fait : sol de prairie photographié (GrassyMeadowGround4) et chemins en terre boueuse avec cailloux (MuddyGroundPack) à la place des couleurs unies ; herbe 2,5 fois plus dense (1 161 touffes) ; pans de rempart qui se chevauchent (plus de fentes sombres) ; linteaux de pierre au-dessus des 6 portes du plateau pour cacher ses murs gris.
- Comment : retour d'Ileana après une vraie partie (« trous d'herbe, sol vert uni, chemins unis, on voit les murs du plateau »). Les textures sont trouvées automatiquement par le script dans les packs installés (`find_basecolor`).
- Capture : aucune

### 2026-11-15 16:00 — Médiéval réaliste : en pleine nature, forêt et montagnes
- Qui : Ileana + Claude (Claude Code, Opus 5.5)
- Fait : plus d'enceinte fermée. En médiéval réaliste, les murs gris du plateau sont cachés (collision gardée) ; la prairie texturée s'étend sur 66 km jusqu'aux montagnes ; forêt de bouleaux réalistes (pack FreshCan, 333 arbres) autour du plateau, avec une grande clairière à l'ouest pour voir les montagnes ; 1 774 touffes d'herbe hors du plateau ; fond de montagnes aux bonnes proportions, le bas de l'image (plaine, lac) passe sous la prairie et seul le ciel noir devient transparent.
- Comment : retour d'Ileana sur une capture de l'éditeur (« je dois avoir l'impression d'être au centre de la nature »). Confusion levée au passage : l'éditeur montre tous les sous-niveaux à la fois (spatial + médiéval) et cache le matériau réaliste ; seul le jeu montre le vrai rendu. Pour les murs de `L_Stage` (niveau du socle), un tag `MedievalHideReal` plutôt que `DressingProp`, sinon ils disparaissaient aussi dans le spatial et le film noir ; c'est le convoi médiéval qui les cache. Live Coding encore bloqué par Windows : build complet nécessaire.
- Capture : docs/devlog-media/2026-11-15-medieval-nature.png, docs/devlog-media/2026-11-15-medieval-foret.png
