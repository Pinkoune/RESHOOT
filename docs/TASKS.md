# RESHOOT — tâches

Une tâche = un propriétaire, un dossier, un critère de fin. Remplacez les `@?` par vos prénoms.

## Rôles

| Rôle | Dev | Dossiers |
| --- | --- | --- |
| Socle et réalisateur | @? | `Source/RESHOOT/Core`, `Source/RESHOOT/Director`, `Content/RESHOOT/Core`, `Content/RESHOOT/Director`, `L_Stage` |
| Médiéval | @ileanasaintpaul | `Source/RESHOOT/Medieval`, `Content/RESHOOT/Medieval` |
| Spatial | @? | `Source/RESHOOT/Space`, `Content/RESHOOT/Space` |
| Film noir + audio | @julesgilli (semi-présent) | `Source/RESHOOT/Noir`, `Content/RESHOOT/Noir`, `Content/RESHOOT/Audio` |
| Interface | premier dev libre au jour 3 | `Source/RESHOOT/UI`, `Content/RESHOOT/UI` |

## Jeudi — fin de journée : un joueur tire sur un bot dans une boîte grise

Socle
- [x] Projet UE 5.9 C++ `RESHOOT`, `.gitignore`, premier commit
- [x] Tous les headers de `docs/ARCHITECTURE.md` créés avec corps vides, compilés, éditeur redémarré une fois (compilé par Build.bat le 12/11)
- [ ] `UHealthComponent`, `AReshootWeapon`, `AReshootCharacter`, `AReshootEnemy` fonctionnels (vie, joueur, ennemi testés en PIE sur `L_SocleTest` ; reste une arme concrète pour tirer pour de vrai)
- [ ] `L_Stage` en boîtes grises
- [ ] Premier build packagé qui se lance

Médiéval
- [ ] Épée (cône), bouclier (réduction de face), arc (projectile chargé) en boîtes grises

Spatial
- [ ] Déplacement en apesanteur avec inertie, laser hitscan

Film noir + audio
- [x] Revolver hitscan avec chargeur (testé en PIE sur `L_NoirTest` : 40 dégâts, x2 à la tête, recharge auto)
- [ ] Liste des 40 répliques du réalisateur (texte), choix de la voix — texte fait : `Content/RESHOOT/Audio/DirectorLines.csv` ; voix en attente des crédits fal

Tous
- [ ] Réclamer les crédits (SpaceXSI, Tripo, Hyper3D, fal, Diversion)
- [ ] Première entrée du dev log avec captures

## Vendredi — fin de journée : chaque univers jouable seul en boîtes grises

Socle
- [ ] `UShootDirectorSubsystem` : scènes, prises, vagues, `AWaveSpawner`
- [ ] « CUT! » : gel, réplique, reprise
- [ ] Mort du joueur = la prise recommence

Médiéval
- [ ] Figurant en armure (fonce au corps-à-corps), archer fixe
- [ ] `UUniverseDefinition` + sous-niveau `L_Medieval`

Spatial
- [ ] Drone (ligne droite vers le joueur)
- [ ] `UUniverseDefinition` + sous-niveau `L_Space`

Film noir + audio
- [x] Gangster à couvert, post-process noir et blanc (testés sur `L_NoirTest` ; `M_PP_Noir` reste à appliquer par le réalisateur via `DA_Noir`)
- [x] `UUniverseDefinition` + sous-niveau `L_Noir` (`DA_Noir`, `L_Noir` : 5 caisses + 3 gangsters, sans sol ni lumière, prévu pour être chargé dans `L_Stage` avec un sol à Z=0)
- [ ] Répliques générées, `DT_DirectorLines` remplie — table créée (`Content/RESHOOT/Audio/DT_DirectorLines`, 40 lignes, sous-titres seulement), sons en attente des crédits fal

**Coupe du vendredi soir : un univers non jouable est supprimé.**

## Samedi — fin de journée : partie complète du menu à l'écran de fin

Socle
- [ ] Directives (6 à 8) et `UAudienceRating`
- [ ] Final : bascule d'univers toutes les 20 s
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
- [ ] Jules : `AReshootWeapon::TraceFromAim` trace sur `ECC_Visibility`, que le profil de collision Pawn ignore : les balles traversent les capsules des ennemis (vérifié en PIE). Contourné côté Noir (corps et tête du gangster bloquent Visibility). Médiéval et Spatial auront le même souci : faire bloquer Visibility à la capsule de `AReshootEnemy`, ou tracer sur un autre canal.
- [ ] Jules : `AReshootEnemy` n'a aucun mesh, un figurant est invisible en jeu. Le gangster ajoute ses propres formes (cylindre, sphère, chapeau) ; prévenir Médiéval et Spatial.
