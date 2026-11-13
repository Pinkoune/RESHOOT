# RESHOOT — Guide de génération des assets

Nov 12, 2026 · @Jérémy Barcelo

## Méthode

Chaque asset suit les mêmes sept étapes ; faites-en un seul de bout en bout avant de lancer une série.

1. **Prendre une ligne** dans les tableaux ci-dessous et écrire son prénom dans la colonne « Qui ».
2. **Ouvrir l'outil indiqué** (Tripo, Rodin ou Meshy) en mode texte vers 3D.
3. **Coller la description de la ligne, puis le bloc de style voulu (carton ou réaliste)** à la suite. Ne rien traduire : les prompts sont en anglais exprès.
4. **Générer, puis juger sur trois points** : la silhouette est lisible, l'objet est entier (pas de trou, pas de morceau flottant), il n'y a ni texte ni logo. Sinon, relancer une fois ; au deuxième échec, simplifier le prompt.
5. **Exporter** en FBX ou GLB avec textures PBR, au nombre de triangles indiqué.
6. **Importer dans Unreal** dans le dossier de la section, avec le nom de la ligne, et régler la taille indiquée.
7. **Cocher « Fait »**, puis pousser sur le dépôt.

Règles :

- **Un asset = une personne.** Ne générez pas un objet déjà pris par quelqu'un.
- **Dix minutes par objet au plus.** Un objet moyen suffit ; on améliore dimanche si le temps le permet.
- **Noter dans le dev log** un prompt qui a bien marché ou un raté drôle, avec une capture : cela sert au prix Glass Box.
- **Lister les outils utilisés.** À la soumission, il faudra déclarer Rodin, Tripo et Meshy dans « Tools you used ».

## Outils, réglages et dossiers

Tripo fait le gros du volume, Rodin fait les armes tenues en main, Meshy sert de secours.

| Type d'objet | Outil | Triangles à l'export | Pourquoi |
| --- | --- | --- | --- |
| Armes en vue subjective | Rodin (hyper3d.ai) | 20 000 à 30 000 | Elles occupent l'écran ; utiliser Rodin ouvre droit à la cagnotte Rodin |
| Accessoires de figurants | Tripo (tripo3d.ai) | 5 000 à 8 000 | Petits, vus de loin |
| Décors et couverts | Tripo | 5 000 à 15 000 | Tripo a le plus de crédits (3 000) |
| Objet raté deux fois ailleurs | Meshy (meshy.ai) | Selon le type | Secours (1 000 crédits) |

Crédits et dates limites :

| Outil | Crédits | À savoir |
| --- | --- | --- |
| Tripo | 3 000 | Accès Pro jusqu'au 17/11 |
| Rodin | 208 | Peu de crédits : uniquement les armes. Code MEGAJAM2026 à saisir avant le 16/11 |
| Meshy | 1 000 | Carte bancaire demandée, renouvellement automatique à annuler |

Réglages communs, quand l'outil les propose :

- **Format :** GLB de préférence (un seul fichier, textures et matériau inclus), FBX sinon. Éviter l'OBJ : il arrive en six fichiers séparés et le matériau est à rebrancher à la main.
- **Textures :** PBR (couleur, normale, rugosité), 2K pour les armes, 1K pour le reste.
- **Maillage :** triangles, pas besoin de quads. Activer la réduction de polygones à l'export plutôt que dans Unreal.
- **Pose :** objet droit, posé au sol, centré.

Dossiers et noms dans Unreal :

| Contenu | Dossier | Nom |
| --- | --- | --- |
| Médiéval | `Content/RESHOOT/Medieval/Meshes/` | `SM_Medieval_<Objet>` |
| Spatial | `Content/RESHOOT/Space/Meshes/` | `SM_Space_<Objet>` |
| Film noir | `Content/RESHOOT/Noir/Meshes/` | `SM_Noir_<Objet>` |
| Commun et envers du décor | `Content/RESHOOT/Core/Meshes/` | `SM_Stage_<Objet>` |

Les textures et matériaux importés restent dans le même dossier que le modèle.

Les décors à deux versions prennent un suffixe, dans le même dossier : `SM_Space_Console_Prop` pour le carton, `SM_Space_Console_Real` pour le réaliste.

## Deux styles : carton, puis réaliste

Chaque scène commence dans un décor en carton. Puis le réalisateur crie « On améliore la mise en scène ! », et une transition remplace chaque objet par sa version réaliste, à la même place. Les objets de décor existent donc en deux versions.

|  | Version carton | Version réaliste |
| --- | --- | --- |
| Quand on la voit | Première prise de chaque scène | Après la transition, jusqu'à la fin de la scène |
| Rendu visé | Bricolé, visiblement faux, drôle | Crédible et soigné : c'est ce que les juges retiennent |
| Suffixe du nom | `_Prop` | `_Real` |
| Triangles | 3 000 à 5 000 | Valeurs des tableaux |

**Règle d'or : les deux versions d'un objet ont la même taille, le même pivot et la même emprise au sol.** La transition les échange sur place ; si elles diffèrent, le décor « saute ». Les murs et couverts du gameplay sont portés par le blockout gris, commun aux deux versions.

**Tout change à la transition, studio compris.** Décors, armes et accessoires existent en deux versions. Le studio (objets communs et envers du décor) n'existe que pendant la phase carton : il disparaît au passage du rideau, et le joueur se retrouve plongé dans l'univers, jusqu'à l'horizon. Les objets du studio n'ont donc qu'une version : du vrai matériel de tournage, en bloc réaliste pour contraster avec le carton, rangé avec la phase carton.

**Bloc réaliste** — à coller après la description :

```text
Realistic game asset, physically based materials, accurate real-world proportions,
detailed but clean, subtle wear and dirt, neutral lighting.
Single object, centered, no background, no text, no logos, no characters.
```

Negative prompt, si l'outil en demande un : `text, letters, logo, watermark, human, hand, multiple objects, base, pedestal, cartoon, toy`

**Bloc carton** — à coller après la même description :

```text
Cheap handmade theater prop version of this object, built from cardboard, plywood and foam,
visible tape, staples and rough brush strokes of flat paint, slightly wonky shapes,
obviously fake and funny. Single object, centered, no background, no text, no logos, no characters.
```

Negative prompt : `text, letters, logo, watermark, human, hand, multiple objects, base, pedestal, photorealistic`

## Priorité 1 : armes, accessoires, objets communs

Ces 11 objets sont à générer vendredi : sans eux, le joueur n'a rien en main et les figurants se ressemblent tous.

Les figurants utilisent le mannequin d'Unreal ; on ne génère aucun personnage, seulement l'accessoire qu'il porte.

Armes et accessoires existent en deux versions, comme les décors : même prompt, suivi une fois du bloc carton et une fois du bloc réaliste, noms en `_Prop` et `_Real`, même taille et même pivot. Cocher « Fait » quand les deux sont importées. Les objets communs font partie du studio : une seule version, qui disparaît à la transition.

**Armes** — outil : Rodin. Dossier de l'univers.

| Objet | Nom | Prompt | Taille dans Unreal | Triangles | Qui | Fait |
| --- | --- | --- | --- | --- | --- | --- |
| Épée | `SM_Medieval_Sword` | `Medieval knight longsword, straight double-edged steel blade, simple crossguard, leather-wrapped grip, round pommel.` | 100 cm de long | 20 000 |  |  |
| Bouclier | `SM_Medieval_Shield` | `Medieval heater shield, painted wood with red and gold heraldic stripes and no emblem, metal rim, leather straps on the back.` | 80 cm de haut | 20 000 |  |  |
| Arc | `SM_Medieval_Bow` | `Medieval wooden longbow, thick curved yew wood, leather grip in the middle, no string, no arrow.` | 140 cm de haut | 20 000 |  |  |
| Fusil laser | `SM_Space_LaserRifle` | `Retro sci-fi laser rifle, compact bullpup shape, white and orange plastic panels, glowing blue emitter at the muzzle, cooling fins on the barrel, chunky trigger guard.` | 70 cm de long | 30 000 |  |  |
| Revolver | `SM_Noir_Revolver` | `1940s snub-nose revolver, six-shot cylinder, dark blued steel, checkered wooden grip, film noir detective style.` | 20 cm de long | 20 000 |  |  |

L'arc est demandé sans corde : les générateurs ratent les objets très fins. La corde et la flèche seront ajoutées dans Unreal.

**Accessoires de figurants et ennemis** — outil : Tripo. Dossier de l'univers.

| Objet | Nom | Prompt | Taille dans Unreal | Triangles | Qui | Fait |
| --- | --- | --- | --- | --- | --- | --- |
| Casque de chevalier | `SM_Medieval_Helmet` | `Medieval knight helmet, rounded steel great helm with eye slit and breathing holes, slightly dented, hollow inside, wearable.` | 30 cm de haut | 8 000 |  |  |
| Chapeau de gangster | `SM_Noir_Fedora` | `1940s fedora hat, dark grey felt, black ribbon band, pinched crown, film noir gangster style, hollow inside, wearable.` | 15 cm de haut, 35 cm de large | 5 000 |  |  |
| Drone | `SM_Space_Drone` | `Small hovering sci-fi security drone, round white body, single large red camera eye, two short side thrusters, small gun barrel underneath, visible screws and seams.` | 60 cm de large | 15 000 |  |  |

**Objets communs** — outil : Tripo. Dossier `Content/RESHOOT/Core/Meshes/`.

| Objet | Nom | Prompt | Taille dans Unreal | Triangles | Qui | Fait |
| --- | --- | --- | --- | --- | --- | --- |
| Clap | `SM_Stage_Clapperboard` | `Film clapperboard, black slate with white diagonal striped hinged clapstick on top, blank slate with empty white lines, wooden frame.` | 30 cm de large (une version géante sera mise à l'échelle pour le menu) | 5 000 |  |  |
| Projecteur sur pied | `SM_Stage_Spotlight` | `Film studio spotlight on a tripod stand, black metal fresnel light with four barn doors, adjustable yoke, thick three-legged stand.` | 200 cm de haut | 10 000 |  |  |
| Chaise de réalisateur | `SM_Stage_DirectorChair` | `Classic folding director's chair, thick wooden X-frame, black canvas seat and backrest, blank backrest.` | 115 cm de haut | 8 000 |  |  |

Si Claude Code a déjà écrit des prompts pour le fusil laser et le drone dans `space-assets.md` (fichier pas encore poussé sur le dépôt), choisissez une seule des deux versions et supprimez l'autre.

## Priorité 2 : décors en deux versions

Chaque objet se génère deux fois avec la même description : une fois suivie du bloc carton, une fois suivie du bloc réaliste. Samedi, avec Tripo, dans le dossier de l'univers.

- **Commencer par la version réaliste**, la poser dans Unreal, puis générer la version carton et la mettre exactement à la même taille.
- **La version carton peut être grossière** : un raté drôle est un bon résultat.
- **Plus de crédits ?** La version carton devient la version réaliste habillée d'une instance de matériau « carton peint ». Moins drôle, mais même taille garantie et aucune génération.

**Médiéval** — `Content/RESHOOT/Medieval/Meshes/`

| Objet | Nom (+ `_Prop` / `_Real`) | Description | Taille dans Unreal | Rôle | Qui | Carton | Réaliste |
| --- | --- | --- | --- | --- | --- | --- | --- |
| Mur de château | `SM_Medieval_CastleWall` | `Medieval castle stone wall section, grey stone blocks, straight modular piece, flat back.` | 400 × 300 cm | Mur, à dupliquer |  |  |  |
| Créneaux | `SM_Medieval_Battlements` | `Row of medieval castle battlements, grey stone, straight modular segment.` | 400 × 100 cm | Se pose sur le mur |  |  |  |
| Tonneau | `SM_Medieval_Barrel` | `Medieval wooden barrel, oak staves with iron hoops, closed lid, standing upright.` | 100 cm de haut | Couvert bas |  |  |  |
| Charrette | `SM_Medieval_Cart` | `Medieval wooden hand cart, two large spoked wheels, open flat bed with low side boards, two pulling handles, empty.` | 250 × 130 cm | Couvert |  |  |  |
| Convoi | `SM_Medieval_Convoy` | Carton : `Life-size wooden horse on a wheeled platform, carved plywood horse with a simple saddle, four small metal wheels.` Réaliste : `Covered medieval supply wagon, wooden body, canvas cover on hoops, four spoked wheels, no horse.` | 250 × 200 cm | Événement du convoi |  |  |  |
| Bannière | `SM_Medieval_Banner` | `Medieval hanging wall banner, long red fabric with gold border and pointed bottom, hanging from a horizontal wooden pole, no emblem.` | 250 cm de haut | Décoration |  |  |  |
| Torche murale | `SM_Medieval_Torch` | `Medieval wall torch, thick wooden shaft wrapped in cloth at the top, black iron wall bracket, unlit, no flame.` | 70 cm de haut | Décoration ; flamme ajoutée dans Unreal |  |  |  |

Les rails du convoi n'existent qu'en carton : `SM_Medieval_Rails_Prop`, prompt `Straight section of film dolly track, two parallel steel tubes joined by flat metal crossbars, lying flat, modular segment.`, 300 × 80 cm. En version réaliste, un chemin de terre peint au sol les remplace, sans génération.

**Spatial** — `Content/RESHOOT/Space/Meshes/`

| Objet | Nom (+ `_Prop` / `_Real`) | Description | Taille dans Unreal | Rôle | Qui | Carton | Réaliste |
| --- | --- | --- | --- | --- | --- | --- | --- |
| Panneau de coursive | `SM_Space_CorridorPanel` | `Sci-fi spaceship corridor wall section, white panels, grey vents and orange stripes, straight modular piece, flat back.` | 400 × 260 cm | Mur, à dupliquer | Jérémy | Fait |  |
| Hublot | `SM_Space_Porthole` | `Large round spaceship porthole window, thick white metal ring frame with bolts, dark glass with stars behind, wall mounted.` | 150 cm de diamètre | Se pose sur le panneau |  |  |  |
| Caisse de cargo | `SM_Space_CargoCrate` | `Sci-fi cargo crate, white and orange hard plastic box with reinforced corners, recessed handles and a hazard stripe, closed.` | 120 cm de côté | Couvert, flotte en apesanteur |  |  |  |
| Console | `SM_Space_Console` | `Sci-fi control console, slanted desk with buttons, switches, levers and dark blank screens, white casing, freestanding.` | 150 × 110 cm | Couvert bas |  |  |  |
| Néon | `SM_Space_NeonTube` | `Long sci-fi light tube fixture, glowing blue tube held by two white metal end caps, wall mounted.` | 150 cm de long | Éclairage |  |  |  |
| Lampe d'alarme | `SM_Space_AlarmLight` | `Red emergency beacon light, red dome on a black metal base with protective wire cage, wall mounted.` | 30 cm de haut | S'allume à la coupure de gravité |  |  |  |
| Bidon | `SM_Space_Barrel` | `Sci-fi fuel canister, yellow metal drum with black hazard stripes, two handles on top, round cap.` | 90 cm de haut | Couvert, flotte en apesanteur |  |  |  |

Le panneau déjà généré, avec son cadre en bois, est la version carton : le renommer `SM_Space_CorridorPanel_Prop`. Commencer ensuite par les caisses et les bidons : ce sont eux qui rendent l'apesanteur visible.

**Film noir** — `Content/RESHOOT/Noir/Meshes/`. L'univers passe en noir et blanc : les formes et les contrastes comptent plus que les couleurs.

| Objet | Nom (+ `_Prop` / `_Real`) | Description | Taille dans Unreal | Rôle | Qui | Carton | Réaliste |
| --- | --- | --- | --- | --- | --- | --- | --- |
| Façade | `SM_Noir_Facade` | `1940s city brick building facade section with a door, two windows and a fire escape, straight modular piece, flat back.` | 500 × 400 cm | Mur, à dupliquer |  |  |  |
| Lampadaire | `SM_Noir_StreetLamp` | `1940s cast iron street lamp, thick fluted black post with a single glass lantern on top, round base.` | 350 cm de haut | Éclairage principal |  |  |  |
| Voiture d'époque | `SM_Noir_Car` | `1940s black sedan car, rounded fenders, long hood, chrome grille and bumpers, whitewall tires, closed doors, no license plate.` | 480 × 160 cm | Couvert principal |  |  |  |
| Poubelle | `SM_Noir_TrashCan` | `1940s metal trash can, dented galvanized steel cylinder with ribbed sides and a lid.` | 90 cm de haut | Couvert bas |  |  |  |
| Kiosque à journaux | `SM_Noir_Newsstand` | `1940s street newsstand, small wooden kiosk with an open front counter, striped awning, shelves with blank newspapers.` | 250 × 250 cm | Couvert haut |  |  |  |
| Bouche d'incendie | `SM_Noir_Hydrant` | `Old cast iron fire hydrant, rounded cap, two side outlets, chipped paint.` | 80 cm de haut | Décoration |  |  |  |
| Enseigne lumineuse | `SM_Noir_NeonSign` | `Vintage neon sign shaped like a cocktail glass, glowing glass tube on a dark metal backing plate with a mounting bracket.` | 120 cm de haut | Seule lumière colorée avant le filtre |  |  |  |
| Cabine téléphonique | `SM_Noir_PhoneBooth` | `1940s wooden telephone booth, tall narrow cabin with glass panel folding door, small roof, payphone inside.` | 220 cm de haut | Couvert haut |  |  |  |

**Arrière-plans de la version réaliste.** Quand le studio disparaît, il faut voir l'univers à la place des murs du hangar. Pas besoin de modèles 3D détaillés : à distance, un ciel et deux ou trois grandes images peintes sur des plans suffisent. Images à générer avec fal, en panorama large, sans personnage ni texte.

| Univers | Ciel | Éléments lointains | Qui | Fait |
| --- | --- | --- | --- | --- |
| Médiéval | Ciel de fin d'après-midi, nuages dorés | Collines et forêts, remparts et tours d'un château au loin |  |  |
| Spatial | Champ d'étoiles et nébuleuse | Grande planète en arrière-plan, coque du vaisseau qui se prolonge |  |  |
| Film noir | Ciel de nuit couvert, pluie | Silhouettes d'immeubles, fenêtres éclairées, enseignes lointaines |  |  |

Ces arrière-plans font partie de la version réaliste : invisibles pendant la répétition, ils apparaissent derrière le rideau.

## Priorité 3 : envers du décor

Six objets communs aux trois univers, en une seule version, visibles seulement pendant la phase carton, à générer dimanche seulement si tout le reste est fait, avec Tripo, dans `Content/RESHOOT/Core/Meshes/`. Ils servent le prix Render Royalty : ce sont eux qui montrent qu'on est sur un plateau.

| Objet | Nom | Prompt | Taille dans Unreal | Triangles | Où le placer | Qui | Fait |
| --- | --- | --- | --- | --- | --- | --- | --- |
| Caméra de cinéma | `SM_Stage_Camera` | `Vintage film movie camera on a heavy tripod, large black camera body with two film reels on top, matte box lens hood, crank handle.` | 180 cm de haut | 15 000 | Aux bords du plateau, tournée vers la zone de jeu |  |  |
| Perche micro | `SM_Stage_BoomMic` | `Boom microphone on a thick wheeled stand, long angled metal arm ending in a large fluffy grey windscreen microphone.` | 300 cm de haut | 8 000 | Au-dessus de la zone de jeu, visible dans le champ |  |  |
| Câbles enroulés | `SM_Stage_Cables` | `Pile of thick black electrical cables coiled in loops on the floor, with two large stage plugs.` | 80 cm de large | 6 000 | Au sol, hors des chemins de course |  |  |
| Sacs de sable | `SM_Stage_Sandbags` | `Stack of three film set sandbags, heavy black canvas bags with carrying handles, piled on top of each other.` | 60 cm de large | 4 000 | Au pied des panneaux de décor et des projecteurs |  |  |
| Passerelle | `SM_Stage_Catwalk` | `Straight section of industrial metal catwalk, grated steel floor with tubular safety railings on both sides, modular segment.` | 400 cm de long, 100 cm de large | 10 000 | En hauteur, le long des murs du hangar |  |  |
| Caisse de matériel | `SM_Stage_FlightCase` | `Black road flight case on four caster wheels, aluminum edges, ball corners, butterfly latches and recessed handles, closed.` | 120 cm de large, 80 cm de haut | 5 000 | En coulisses, peut servir de couvert |  |  |

## Import dans Unreal et contrôle

Un asset n'est « fait » que lorsqu'il est dans le jeu, à la bonne taille, avec une collision.

1. **Glisser le fichier** (GLB de préférence) dans le dossier indiqué du Content Browser.
2. **Options d'import :** garder les textures, ne pas importer d'animation, une seule mesh combinée.
3. **Renommer** selon la colonne « Nom », avec `_Prop` ou `_Real` pour les décors.
4. **Matériau d'un décor à deux versions :** ne pas garder le matériau généré. Créer une instance du matériau maître `M_SetDressing` et y brancher couleur, normale et rugosité. C'est lui qui fait la transition : un décor sans lui ne change pas au passage du balayage.
5. **Poser l'objet à côté du mannequin** et le mettre à la taille indiquée. Si l'échelle est très loin de 1, corriger l'échelle d'import dans les réglages de l'asset.
6. **Vérifier le pivot :** en bas et au centre pour un décor, à la poignée pour une arme. Les deux versions d'un objet ont le même pivot.
7. **Collision :** aucune sur les décors fixes, le blockout gris s'en charge. Collision convexe automatique pour ce qui bouge (caisses et bidons flottants, convoi) et pour les objets à une version qui servent de couvert.
8. **Superposer les deux versions** et alterner leur visibilité : rien ne doit sauter. Puis regarder l'objet sous l'éclairage du plateau et ajuster la rugosité si besoin.

Pièges fréquents :

| Problème | Solution |
| --- | --- |
| Objet minuscule ou géant | Échelle d'import : les GLB sont souvent en mètres, Unreal travaille en centimètres |
| Objet couché sur le côté | Rotation d'import de 90° sur l'axe X |
| Matériau gris | Les textures n'ont pas suivi : les importer à la main et les brancher dans le matériau |
| Parties fines cassées (corde, pieds, câbles) | Regénérer avec « thick » dans le prompt, ou supprimer la partie du prompt |
| Texte illisible sur l'objet | Regénérer en ajoutant « blank, no text » |
| Trop de triangles | Réduire à l'export de l'outil, pas dans Unreal |

Pour les armes, c'est le développeur de l'univers qui règle la position en main ; la personne qui génère s'arrête à l'import.

Les fichiers `.uasset` sont gros : poussez-les par petits lots, et prévenez l'équipe avant d'ajouter plus de dix objets d'un coup.
