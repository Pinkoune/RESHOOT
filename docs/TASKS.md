# RESHOOT — tâches

Une tâche = un propriétaire, un dossier, un critère de fin.

## Rôles

| Rôle | Dev | Dossiers |
| --- | --- | --- |
| Socle, plateau, builds | Jérémy (+ Claude) | `Source/RESHOOT/Core`, `Content/RESHOOT/Core`, `L_Stage`, builds quotidiens |
| Réalisateur | Enzo | `Source/RESHOOT/Director`, `Content/RESHOOT/Director` (directives, audimat, final, casting) |
| Médiéval | Ileana | `Source/RESHOOT/Medieval`, `Content/RESHOOT/Medieval` |
| Spatial | Jérémy + Claude, Codex, Argon (voir découpage ci-dessous) | `Source/RESHOOT/Space`, `Content/RESHOOT/Space` |
| Film noir + audio | Jules (@julesgilli, semi-présent) | `Source/RESHOOT/Noir`, `Content/RESHOOT/Noir`, `Content/RESHOOT/Audio` |
| Interface | premier dev libre au jour 3 | `Source/RESHOOT/UI`, `Content/RESHOOT/UI` |

Passation du réalisateur à Enzo : `docs/HANDOFF-Director.md`.

### Découpage du spatial (aucun fichier en commun)

| Lot | Qui | Fichiers (et rien d'autre) | Brief |
| --- | --- | --- | --- |
| Fusil laser | Claude (Claude Code) | `Space/SpaceLaser.h/.cpp`, `Content/RESHOOT/Space/Weapons/` | — |
| Drone | Codex | `Space/SpaceDrone.cpp`, `Content/RESHOOT/Space/Enemies/` | `docs/briefs/codex-space-drone.md` |
| Gravité + habillage | Argon (Antigravity) | `Space/SpaceGravityEvent.cpp`, `Content/RESHOOT/Space/Maps/L_Space`, `Content/RESHOOT/Space/Environment/` | `docs/briefs/argon-space-gravity.md` |
| `DA_Universe_Space`, intégration | Jérémy + Claude | `Content/RESHOOT/Space/DA_Universe_Space` | — |

Une seule IA à la fois pilote l'éditeur via le MCP Unreal : Claude par défaut. Les autres écrivent du `.cpp` et décrivent ce qu'il faut créer dans l'éditeur.

## Jeudi — fin de journée : un joueur tire sur un bot dans une boîte grise

Socle
- [x] Projet UE 5.9 C++ `RESHOOT`, `.gitignore`, premier commit
- [x] Tous les headers de `docs/ARCHITECTURE.md` créés avec corps vides, compilés, éditeur redémarré une fois (compilé par Build.bat le 12/11)
- [ ] `UHealthComponent`, `AReshootWeapon`, `AReshootCharacter`, `AReshootEnemy` fonctionnels (vie, joueur, ennemi testés en PIE sur `L_SocleTest` ; reste une arme concrète pour tirer pour de vrai)
- [x] Second lot de headers : signaux des armes, restrictions des directives, casting, nom du joueur, bilan, valeurs du GDD, headers du spatial (redémarrage de l'éditeur requis)
- [ ] `L_Stage` en boîtes grises
- [ ] Premier build packagé qui se lance

Médiéval
- [ ] Épée (cône), bouclier (réduction de face), arc (projectile chargé) en boîtes grises

Spatial
- [x] Déplacement en apesanteur avec inertie (socle)
- [ ] Laser hitscan, surchauffe, visée (Claude)

Film noir + audio
- [ ] Revolver hitscan avec chargeur
- [ ] Liste des 40 répliques du réalisateur (texte), choix de la voix

Tous
- [ ] Réclamer les crédits (SpaceXSI, Tripo, Hyper3D, fal, Diversion)
- [ ] Première entrée du dev log avec captures

## Vendredi — fin de journée : chaque univers jouable seul en boîtes grises

Réalisateur (Enzo)
- [ ] `UShootDirectorSubsystem` : scènes, prises, vagues, `AWaveSpawner`, sous-niveaux en streaming
- [ ] « CUT! » au milieu de la prise (20 à 40 s) : gel, réplique, directive, reprise
- [ ] Mort du joueur = la prise recommence au point de départ (`ResetForTake`)

Médiéval
- [ ] Figurant en armure (fonce au corps-à-corps), archer fixe
- [ ] `UUniverseDefinition` + sous-niveau `L_Medieval`

Spatial
- [ ] Drone : ligne droite vers le joueur, tir à courte portée, gel au CUT (Codex)
- [ ] Événement de coupure de gravité (Argon)
- [ ] Sous-niveau `L_Space` en boîtes grises, puis habillé (Argon)
- [ ] `DA_Universe_Space` (Jérémy + Claude)

Film noir + audio
- [ ] Gangster à couvert, post-process noir et blanc
- [ ] `UUniverseDefinition` + sous-niveau `L_Noir`
- [ ] Répliques générées, `DT_DirectorLines` remplie

**Coupe du vendredi soir : un univers non jouable est supprimé.**

## Samedi — fin de journée : partie complète du menu à l'écran de fin

Réalisateur (Enzo)
- [ ] Les 8 directives du GDD et `UAudienceRating` (+ 40 prise réussie)
- [ ] Final : bascule d'univers toutes les 20 s
- [ ] Casting : `StartCasting` / `FinishCasting`, nom du joueur
- [ ] Intégration des trois univers dans `UShootScript`

Univers
- [ ] Habillage (assets générés), événement d'univers
- [ ] Équilibrage : une prise dure 60 à 90 s

Interface
- [ ] HUD, menu clap, casting, écran de fin

**Samedi soir : gel des fonctionnalités. Test par une personne extérieure, seule.**

## Dimanche — build publié sur itch.io avant lundi 2h

- [ ] Corrections issues du test
- [ ] Éclairage et rendu, DLSS si carte RTX disponible
- [ ] Sons, musiques, mixage
- [ ] Build de secours en ligne à 20h
- [ ] Page itch.io : description, captures, « Tools you used » (Rodin, Tripo, Meshy, fal, Claude, Codex, Antigravity…)
- [ ] Dev log finalisé
- [ ] Build final

## Demandes au socle

Format : `- [ ] <demandeur> : j'ai besoin de <fonction ou delegate> pour <raison>`
