# RESHOOT — tâches

Une tâche = un propriétaire, un dossier, un critère de fin.

## Rôles

| Rôle | Dev | Dossiers |
| --- | --- | --- |
| Socle, plateau, builds | Jérémy (+ Claude) | `Source/RESHOOT/Core`, `Content/RESHOOT/Core`, `L_Stage`, builds quotidiens |
| Réalisateur | Enzo | `Source/RESHOOT/Director`, `Content/RESHOOT/Director` (directives, audimat, final, casting) |
| Médiéval | Ileana | `Source/RESHOOT/Medieval`, `Content/RESHOOT/Medieval` |
| Spatial | Jérémy + Claude (Codex et Argon en renfort, voir ci-dessous) | `Source/RESHOOT/Space`, `Content/RESHOOT/Space` |
| Film noir + audio | Jules (@julesgilli, semi-présent) | `Source/RESHOOT/Noir`, `Content/RESHOOT/Noir`, `Content/RESHOOT/Audio` |
| Interface | Ileana (en plus du Médiéval) | `Source/RESHOOT/UI`, `Content/RESHOOT/UI` |

Passation du réalisateur à Enzo : `docs/HANDOFF-Director.md`.

### Découpage du spatial (aucun fichier en commun)

| Lot | Qui | Fichiers (et rien d'autre) | Brief |
| --- | --- | --- | --- |
| Fusil laser | Claude (Claude Code) | `Space/SpaceLaser.h/.cpp`, `Content/RESHOOT/Space/Weapons/` | — |
| Drone | ~~Codex~~ fait par Claude le 12/11 | `Space/SpaceDrone.cpp`, `Content/RESHOOT/Space/Enemies/` | `docs/briefs/codex-space-drone.md` (sans objet) |
| Gravité (fait par Claude le 12/11) + habillage avec les assets générés (Argon) | `Space/SpaceGravityEvent.cpp`, `Content/RESHOOT/Space/Maps/L_Space`, `Content/RESHOOT/Space/Environment/` | `docs/briefs/argon-space-gravity.md` |
| `DA_Universe_Space`, intégration | Jérémy + Claude | `Content/RESHOOT/Space/DA_Universe_Space` | — |

Une seule IA à la fois pilote l'éditeur via le MCP Unreal : Claude par défaut. Les autres écrivent du `.cpp` et décrivent ce qu'il faut créer dans l'éditeur.

## Jeudi — fin de journée : un joueur tire sur un bot dans une boîte grise

Socle
- [x] Projet UE 5.9 C++ `RESHOOT`, `.gitignore`, premier commit
- [x] Tous les headers de `docs/ARCHITECTURE.md` créés avec corps vides, compilés, éditeur redémarré une fois (compilé par Build.bat le 12/11)
- [ ] `UHealthComponent`, `AReshootWeapon`, `AReshootCharacter`, `AReshootEnemy` fonctionnels (vie, joueur, ennemi testés en PIE sur `L_SocleTest` ; reste une arme concrète pour tirer pour de vrai)
- [x] Second lot de headers : signaux des armes, restrictions des directives, casting, nom du joueur, bilan, valeurs du GDD, headers du spatial (redémarrage de l'éditeur requis)
- [x] `L_Stage` en boîtes grises (version d'Enzo gardée à la fusion : 60 × 60 m, 6 portes avec apparitions, règle des 10 m, sous-niveaux des univers ; GameMode `BP_StageGameMode`, script `SS_Main`. La version de Jérémy — mezzanines, rampes, couverts — est dans l'historique de `origin/space`)
- [ ] Premier build packagé qui se lance

Médiéval
- [x] Épée (cône), bouclier (réduction de face), arc (projectile chargé) en boîtes grises (`AMedievalSword` à deux modes : 1 = épée, 2 = arc, molette = bascule ; testé en PIE sur `L_MedievalTest`)

Spatial
- [x] Déplacement en apesanteur avec inertie (socle)
- [x] Laser hitscan, surchauffe, visée (testé en PIE)

Film noir + audio
- [x] Revolver hitscan avec chargeur (testé en PIE sur `L_NoirTest` : 40 dégâts, x2 à la tête, recharge auto)
- [ ] Liste des 40 répliques du réalisateur (texte), choix de la voix — texte fait : `Content/RESHOOT/Audio/DirectorLines.csv` ; voix en attente des crédits fal

Tous
- [ ] Réclamer les crédits (SpaceXSI, Tripo, Hyper3D, fal, Diversion)
- [ ] Première entrée du dev log avec captures

## Vendredi — fin de journée : chaque univers jouable seul en boîtes grises

Réalisateur (Enzo)
- [x] `UShootDirectorSubsystem` : scènes, prises, vagues, `AWaveSpawner`, sous-niveaux en streaming
- [x] « CUT! » au milieu de la prise (20 à 40 s) : gel, réplique, directive, reprise (testé dans `L_Stage`)
- [x] Mort du joueur = la prise recommence au point de départ (`ResetForTake`)

Médiéval
- [x] Figurant en armure (fonce au corps-à-corps), archer fixe (`AMedievalKnight`, `AMedievalArcher`, flèche `AMedievalArrow` ; testé en PIE sur `L_MedievalTest`)
- [x] `UUniverseDefinition` + sous-niveau `L_Medieval` (`DA_Medieval` ; `L_Medieval` : rempart en panneau peint, chemin de ronde à 4 m accessible par deux longues rampes, deux tours, 3 archers, tonneaux, charrettes et bottes de paille ; sans sol ni lumière, testé dans `L_MedievalTest`)

Spatial
- [x] Drone : ligne droite vers le joueur, tir à courte portée, gel au CUT (testé en PIE)
- [x] Événement de coupure de gravité (testé en PIE)
- [x] Sous-niveau `L_Space` en boîtes grises
- [ ] `L_Space` habillé avec les assets générés (Argon, `docs/briefs/space-assets.md`)
- [x] `DA_Universe_Space`, `BP_SpaceDrone`, script de test `SS_SpaceTest`

Film noir + audio
- [x] Gangster à couvert, post-process noir et blanc (testés sur `L_NoirTest` ; `M_PP_Noir` reste à appliquer par le réalisateur via `DA_Noir`)
- [x] `UUniverseDefinition` + sous-niveau `L_Noir` (`DA_Noir`, `L_Noir` : 5 caisses + 3 gangsters, sans sol ni lumière, prévu pour être chargé dans `L_Stage` avec un sol à Z=0)
- [ ] Répliques générées, `DT_DirectorLines` remplie — table créée (`Content/RESHOOT/Audio/DT_DirectorLines`, 40 lignes, sous-titres seulement), sons en attente des crédits fal

**Coupe du vendredi soir : un univers non jouable est supprimé.**

## Samedi — fin de journée : partie complète du menu à l'écran de fin

Réalisateur (Enzo)
- [x] Les 8 directives du GDD et `UAudienceRating` (+ 40 prise réussie) (8 directives dans `Content/RESHOOT/Director/Directives`, testées en PIE)
- [x] Final : bascule d'univers toutes les 20 s (Médiéval ⇄ Noir testé dans `L_Stage`, vagues en boucle)
- [x] Casting : studio de cinéma dans le sous-niveau `L_Casting` (chargé et déchargé par le réalisateur ; estrade, toboggan, tapis rouge, cyclo vert, 3 `ACastingTarget`, PlayerStart `Casting`), glissade + 3 cibles puis scène 1 ; `OnCastingProgress` pour l'UI. Nom du joueur : côté interface (`AReshootGameState::SetPlayerName`)
- [x] Intégration des trois univers dans `UShootScript` (`SS_Main` : Médiéval, Spatial, Noir, final sur les trois ; testé dans `L_Stage`)

Univers
- [ ] Habillage (assets générés), événement d'univers — Noir : événement fait (`ANoirBlackoutEvent`, coupure de courant 10 s, branché dans `DA_Noir`), habillage à faire ; Médiéval : convoi de chevaux de bois fait (`AMedievalHorseConvoyEvent` dans `L_Medieval`, branché dans `DA_Medieval`), habillage à faire
- [ ] Équilibrage : une prise dure 60 à 90 s

Interface
- [ ] HUD, menu clap, casting, écran de fin

**Samedi soir : gel des fonctionnalités. Test par une personne extérieure, seule.**

## Dimanche — build publié sur itch.io avant lundi 2h

- [ ] Corrections issues du test
- [ ] Éclairage et rendu, DLSS si carte RTX disponible
- [ ] Sons, musiques, mixage — lecture des musiques faite côté réalisateur (`UUniverseDefinition::Music` de chaque univers : fondu enchaîné, coupée au CUT!, change au final, volume et fondus dans `SS_Main`) ; reste à fournir les morceaux (Jules)
- [ ] Build de secours en ligne à 20h
- [ ] Page itch.io : description, captures, « Tools you used » (Rodin, Tripo, Meshy, fal, Claude, Codex, Antigravity…)
- [ ] Dev log finalisé
- [ ] Build final

## Décor carton -> réaliste (13/11)

- [x] Socle : `ASetDressingSweep`, `MPC_SetDressing`, `M_SetDressing`, armes et accessoires en deux versions, test dans `L_Space` (Jérémy + Claude)
- [x] Réalisateur : `ResetDressing()` au début de chaque scène, `UpgradeDressing()` à la fin de la prise 1 et au final (Enzo, fait le 15/11, testé en PIE ; suivi automatique du rideau coupé par le réalisateur)
- [ ] Chaque univers : versions `_Prop` et `_Real` de chaque objet, de l'arme et de l'accessoire du figurant, rangées et taguées selon `docs/ARCHITECTURE.md` (Ileana, Jules, spatial)
- [x] Spatial : versions carton et réaliste des 7 objets, du fusil et du drone ; sol, murs, ciel, planète et coque en version réaliste ; studio réel (caméras, perche, passerelles, câbles, sacs de sable, flight cases)
- [x] `L_Stage` : blockout invisible en jeu, plafond de collision, studio sombre (gril, projecteurs) commun aux univers
- [x] Rendu : exposition figée dans les deux phases
- [ ] Médiéval et film noir : habillage réaliste et carton sur le blockout (sans lui, leur phase réaliste montre du vide), sur le modèle de `Tools/build_space_set.py`
- [ ] Compléter `docs/CREDITS.md` (titres Fab exacts) avant la soumission
- [ ] Étincelles `NS_SetDressingSparks` : réglage (couleur orange, plus courtes)
- [x] Spatial : ciel et planète procéduraux, fusil laser réaliste animé, effets de tir du fusil et des drones (`USpaceLaserFXComponent`, `Tools/build_laser_fx.py`)
- [ ] Spatial : étincelles d'impact laser (le modèle Niagara est désactivé, trop gros), sons définitifs du mode réaliste (Jules ; des sons provisoires sont branchés). Carton fait (sons à la bouche de Jérémy)
- [x] Spatial : fusil laser en carton (Tripo), posé comme `PropMesh` de `BP_SpaceLaser`

## Demandes au socle

Format : `- [ ] <demandeur> : j'ai besoin de <fonction ou delegate> pour <raison>`
- [x] Jules : `AReshootWeapon::TraceFromAim` trace sur `ECC_Visibility`, que le profil de collision Pawn ignore : les balles traversent les capsules des ennemis (vérifié en PIE). Contourné côté Noir (corps et tête du gangster bloquent Visibility). Médiéval et Spatial auront le même souci : faire bloquer Visibility à la capsule de `AReshootEnemy`, ou tracer sur un autre canal. — Fait par le socle (tir par type d'objet).
- [ ] Jules : `AReshootEnemy` n'a aucun mesh, un figurant est invisible en jeu. Le gangster ajoute ses propres formes (cylindre, sphère, chapeau) ; prévenir Médiéval et Spatial.
- [x] Jules : `L_Stage` a besoin d'un `NavMeshBoundsVolume` qui couvre le plateau : les gangsters courent d'un couvert à l'autre par navmesh (sans navmesh, ils restent sur place). Les couverts sont les acteurs tagués `NoirCover` (caisses de `L_Noir`). — Fait : `Plateau_NavMesh` couvre les 60 × 60 m.
- [x] Jules : directive « Plan-séquence » (Noir) : `ANoirRevolver::SetReloadBlocked(true)` au début, `RefillMagazine()` à chaque élimination, `SetReloadBlocked(false)` à la fin. L'événement Noir se lance avec `StartEvent()` et s'arrête seul au bout de `Duration` (10 s). — Fait : `UDirectiveOneTake` (« Pas de coupure ! », `DIR_OneTake`).

- [ ] @ileanasaintpaul : j'ai besoin de `AReshootCharacter::SetSprintBlocked(bool)` (ou d'un multiplicateur de vitesse) pour empêcher le sprint bouclier levé. En attendant, `AMedievalSword` rabaisse `MaxWalkSpeed` toutes les 0,05 s.
- [ ] Enzo (fusion de `space`) : deux correctifs minimaux faits hors de mon périmètre pour que le projet compile avec le nouveau socle. À vérifier par leurs propriétaires :
  - Jules : `ANoirBlackoutEvent::Duration` supprimé, il masquait `AUniverseEvent::Duration` (même défaut 10 s). La coupure utilise maintenant celle du socle, et le socle programme aussi son propre arrêt : vérifier qu'il n'y a pas de double arrêt.  — Vérifié par Jules (15/11) : le minuteur propre à la coupure est retiré, seul celui du socle reste ; testé en PIE, la coupure reste active pendant la pause du CUT! et s'arrête après la reprise.
  - Iléana : `AMedievalSword::GetMode()` renommé `GetMedievalMode()` (et ses 3 appels dans `ReshootHUD.cpp`, `GetModeCount` = 3 avec les poings) (il entrait en conflit avec `AReshootWeapon::GetMode()` du socle) ; ajout de `GetMode()` / `GetModeCount()` en override (0 = épée, 1 = arc). Pas de `SelectMode` : les touches 1 / 2 du socle ne changent pas encore de mode.
- [x] @ileanasaintpaul : j'ai besoin que le réalisateur déclenche l'événement d'univers (`UUniverseDefinition::EventClass`) une fois par scène : chercher un acteur de cette classe déjà posé dans le sous-niveau (le convoi médiéval `AMedievalHorseConvoyEvent` est posé dans `L_Medieval` avec ses rails), sinon le faire apparaître, puis appeler `StartEvent()` 3 s après la réplique d'annonce. L'événement s'arrête tout seul (`StopEvent`) et se fige au CUT!.  Penser à remettre `AutoStartInterval` du convoi à 0 dans `L_Medieval` à ce moment-là (il se relance seul toutes les 20 s pour les tests). — Fait : prise 2 de chaque scène, 15 s après « Action! », annonce puis `StartEvent()` 3 s plus tard, figé au CUT!, arrêté en fin de prise. `AutoStartInterval` du convoi remis à 0 dans `L_Medieval`.
- [x] @ileanasaintpaul (Interface) : j'ai besoin que `AReshootGameMode` utilise `AReshootHUD` (`HUDClass = AReshootHUD::StaticClass()` dans son constructeur, `#include "UI/ReshootHUD.h"`) pour afficher le HUD dans le vrai jeu. En attendant, il est branché sur le GameMode de test médiéval. — Pour `L_Stage` : fait via `BP_StageGameMode` (HUDClass). Reste valable pour les autres cartes.
- [ ] Enzo (réalisateur, 14/11) : retouches hors de mon périmètre, à vérifier par leurs propriétaires :
  - Jules : 9 lignes ajoutées à `DT_DirectorLines` (sous-titres seuls) : `Dir_NoShield`, `Dir_NoBow`, `Dir_LaserCost`, `Staging_Improve`, `CastingIntro`, `CastingDone`, `EventSpace`, `Event_Medieval`, `Event_Noir` ; `EventLineId` de `DA_Noir` = `Event_Noir`.
  - Iléana : `EventLineId` de `DA_Medieval` = `Event_Medieval` ; `AutoStartInterval` du convoi à 0 dans `L_Medieval` (c'est le réalisateur qui le lance).
- [x] Enzo → Jérémy : transition carton → réaliste branchée sur `ASetDressingSweep`. Le réalisateur appelle `ResetDressing()` au début de chaque scène, `UpgradeDressing()` à la fin de la 1re prise (« On améliore la mise en scène ! », le gel du CUT! dure le temps du balayage + 0,5 s) et au final, et met `reshoot.Dressing.AutoFromDirector` à 0. Tags du décor : ceux du socle (`DressingProp`, `DressingReal`, `DressingSwitch`). Mon doublon (MPC du réalisateur, tags `SetDressing_*`) est supprimé.
- [ ] Enzo → Iléana (Interface) : pendant le casting (phase `Casting`), le HUD affiche « Scène 1 – Prise 1 », le REC et l'audimat. À masquer ou à remplacer par la fiche d'acteur ; `OnCastingProgress(bSlid, TargetsHit, TargetsTotal)` donne l'avancement du tutoriel.
- [ ] Enzo → Jérémy (socle) : `ReshootCharacter.cpp` retouché (accord d'Enzo) : touche C ajoutée à l'accroupi/glissade, et `CanStartSlide` accepte la vitesse de sprint même Maj relâché. Possible amélioration : un délai de grâce (~0,3 s) après le relâchement de Maj, qui demande une variable dans le header.
- [ ] Enzo → Jules : sous-titres du réalisateur passés en anglais (49 lignes de `DT_DirectorLines` et `DirectorLines.csv`, demande d'Enzo). Le texte français est gardé dans `Content/RESHOOT/Audio/DirectorLines_FR.csv`. Pour les voix (fal) : choisir la langue ; si elles restent en français, les sous-titres anglais servent de traduction.
- [ ] Enzo → équipe : tout le texte visible du jeu est passé en anglais (demande d'Enzo, 15/11). Hors de mon dossier, à vérifier par leurs propriétaires : HUD (`UI/ReshootHUD.cpp` : SCENE / TAKE, RATINGS, HEALTH, CUT!, 1 SWORD 2 BOW 3 FISTS, GUARD, SHIELD UP — Iléana), noms d'armes (`MedievalSword.cpp`, `BP_MedievalSword` : Sword & shield, Bow, Fists ; `SpaceLaser.cpp`, `BP_SpaceLaser` : Laser rifle), noms d'univers (`DA_Medieval` : Medieval, `DA_Universe_Space` : Sci-fi et mots de l'affiche Orbit / Galaxy / Odyssey / Void / Stars), nom de joueur par défaut `Extra` (`ReshootGameState.h`, Jérémy). Tout nouveau texte affiché doit être écrit en anglais.
