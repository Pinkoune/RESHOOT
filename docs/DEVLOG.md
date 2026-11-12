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
