# RESHOOT — assets et outils à déclarer à la soumission

Le règlement de la jam demande de lister les assets qu'on n'a pas faits. Ils ne sont pas notés.

## Assets Fab (non créés par l'équipe)

| Dossier dans `Content/` | Contenu | Utilisé pour | Titre exact et auteur sur Fab | Preuve de licence |
| --- | --- | --- | --- | --- |
| `Hallway_sci_fi` | Modules de couloir de vaisseau (murs, sols, colonnes, portes, lampes) | Pont spatial réaliste : sol, murs bas, sas, parements, couverts | À compléter (Jérémy) | Capture de la bibliothèque Fab |
| `Hollywood_Vol3` | Matériel de tournage (poutres, projecteurs, pieds) | Studio de la phase carton : gril, projecteurs | À compléter (Jérémy) | Capture de la bibliothèque Fab |
| `Fab/Laserrifle` | Fusil laser (mesh, matériaux, 7 textures) | Fusil laser réaliste du spatial | À compléter (Jérémy) | Capture de la bibliothèque Fab |
| `Sci_fi_hallway` | Second kit de couloir (blanc et rouge) | Pas encore utilisé | À compléter (Jérémy) | Capture de la bibliothèque Fab |
| `Light_Foliage` (MYTHRA TECH, https://www.fab.com/listings/30e6c98c-6634-4b7e-b142-2c3cdfde6e1e) | Herbes, fleurs, buissons, rochers | Prairie réaliste du médiéval (herbe 3D, buissons, rochers) | Licence Personal gratuite (Professional payante) ; **marqué « entraînement IA interdit » sur Fab : à vérifier** (Ileana) | Page Fab |
| `Fab/FREE_Farm_Paths_and_Stone_Walls_Kit` (Lauraa, https://www.fab.com/listings/ccd7c525-fa69-4585-8799-74e6f4785c87) | Chemins (terre, pavés, gravier, dalles), murets de pierre | Chemins de terre du médiéval réaliste | Gratuit, importé en FBX (textures absentes : matériau de terre maison) (Ileana) | Page Fab |
| `GrassyMeadowGround4` (Mosaic Environments, https://www.fab.com/listings/5743b825-ba16-4048-be20-37188bbad04f) | Sol de prairie photographié (textures 512 à 4K) | Sol de la prairie réaliste du médiéval | Licence Personal gratuite (Ileana) | Page Fab |
| `MuddyGroundPack` (AL-HADIDI, https://www.fab.com/listings/93d594e8-99f2-4ec6-8980-6135929338d8) | Terre boueuse avec cailloux et herbe | Chemins de terre du médiéval réaliste | Licence Personal gratuite (Ileana) | Page Fab |
| `FC_BirchTreeSample` (FreshCan, https://www.fab.com/listings/fd8126f6-4dea-4023-9213-2af433743995) | Bouleaux, jeunes arbres, tapis d'herbe | Forêt autour de la prairie médiévale | Gratuit (Personal et Professional) ; **marqué « entraînement IA interdit » sur Fab : à vérifier** (Ileana) | Page Fab |

À vérifier avant la soumission : aucun de ces packs ne porte le tag « NoAI » sur Fab (leur licence interdit alors de les donner en entrée à une IA générative).

## Assets Poly Haven (CC0, non créés par l'équipe)

Licence CC0 : aucune attribution obligatoire, on les cite quand même. Téléchargés via Blender (MCP for Blender), exportés en GLB, importés dans `Content/RESHOOT/Noir/`. Habillage posé par `Tools/build_noir_set.py`.

| Asset Poly Haven | Auteur | Dans le jeu |
| --- | --- | --- |
| Street Lamp 01 | Josh Dean | `SM_Noir_StreetLamp_Real` (lampadaires ; en carton peint en phase carton) |
| Street Lamp 02 | Josh Dean | `SM_Noir_WallLamp_Real` (appliques) |
| Covered Car | MP | `SM_Noir_CoveredCar_Real` (voitures bâchées ; en carton peint en phase carton) |
| Metal Trash Can | GurJas Studios | `SM_Noir_TrashCan_Real` |
| Fire Hydrant | Gonçalo Felício | `SM_Noir_Hydrant_Real` |
| Barrel_01 | Jorge Camacho | `SM_Noir_Barrel_Real` |
| Trashbag | Benny Weimer | `SM_Noir_TrashBag_Real` |
| Wooden Crate 02 | James Ray Cock, Jurita Burger | `SM_Noir_WoodCrate_Real` (couverts des gangsters) |
| Rollershutter Window 01 | MP | `SM_Noir_Shutter_Real` (rideaux de fer) |
| Cardboard Box 01 | Rahul Chaudhary | `SM_Noir_CardboardBox_Prop` (couverts en phase carton) |
| Service Pistol | Mateusz Sadek | `SM_Noir_Pistol_Real` (arme du film noir ; `SM_Noir_Pistol_Prop` en carton peint) |
| Modular Fire Escape | Juniix | `SM_Noir_FE_*_Real` (escaliers de secours) |
| Exterior Aircon Unit | Monsta3D | `SM_Noir_AirCon_Real` (climatiseurs) |
| Utility Box 02 | James Ray Cock | `SM_Noir_UtilityBox_Real` (armoires électriques) |
| Barrel Stove | MP | `SM_Noir_BarrelStove_Real` (braseros) |
| Concrete Pavement 03 (texture) | Charlotte Baglioni | `T_Noir_Sidewalk_D` (trottoirs, corniches) |
| Red Bricks 02 (texture) | Rob Tuytel | `T_Noir_Brick_*` (façades) |
| Asphalt 02 (texture) | Rob Tuytel | `T_Noir_Asphalt_*` (sol de la ruelle) |

## Musiques (licence CC-BY 4.0, citation obligatoire)

| Morceau | Auteur | Licence | Dans le jeu |
| --- | --- | --- | --- |
| « Hard Boiled » | Kevin MacLeod (incompetech.com) | Creative Commons : By Attribution 4.0 (http://creativecommons.org/licenses/by/4.0/) | `SW_Noir_Music_HardBoiled`, boucle du film noir (`DA_Noir`) |
| « Master of the Feast » | Kevin MacLeod (incompetech.com) | Creative Commons : By Attribution 4.0 (http://creativecommons.org/licenses/by/4.0/) | `SW_Medieval_Music_MasterOfTheFeast`, boucle du médiéval (`DA_Medieval`) |
| « Space Fighter Loop » | Kevin MacLeod (incompetech.com) | Creative Commons : By Attribution 4.0 (http://creativecommons.org/licenses/by/4.0/) | `SW_Space_Music_SpaceFighterLoop`, boucle du spatial (`DA_Universe_Space`) |

Texte à reprendre tel quel sur la page itch.io : "Hard Boiled", "Master of the Feast", "Space Fighter Loop" Kevin MacLeod (incompetech.com) Licensed under Creative Commons: By Attribution 4.0 License http://creativecommons.org/licenses/by/4.0/

## Police

| Police | Auteur | Licence | Où |
| --- | --- | --- | --- |
| Bebas Neue | Dharma Type (Ryoichi Tsunekawa), The Bebas Neue Project Authors | SIL Open Font License 1.1 (texte joint : `SourceArt/UI/Fonts/OFL.txt`) | Titres du HUD (`FF_BebasNeue`, `AReshootHUD`) |

Texte à reprendre sur la page itch.io : Bebas Neue by Dharma Type, licensed under the SIL Open Font License 1.1.

## Assets générés pendant la jam (créés par l'équipe)

- Modèles 3D : Tripo (décors, drone, studio, décors médiévaux), Rodin (fusil laser ; épée, bouclier, arc, casque médiévaux). Sources dans `SourceArt/`.
- Images médiévales : montagnes et ciel (`SourceArt/Medieval/Real/Backdrops`).
- Images : fal (sol, mur). Le ciel et la planète sont maintenant procéduraux (`Tools/build_hangar.py`).
- Voix du réalisateur : 49 répliques générées avec ElevenLabs Eleven v4 (voix « George ») via fal. Textes et indications de jeu dans `SourceArt/Audio/DirectorVoice/prompts.csv`.

## Outils

Claude Code (Opus 5.5, Fable 5.2), Codex, Antigravity, Tripo, Rodin (Hyper3D), Meshy, fal, Blender (MCP for Blender), Poly Haven. Musiques des trois univers : Kevin MacLeod (CC-BY 4.0, voir plus haut). Autres sons et musiques : WeLoveIndies (à confirmer).
- Paragon: Greystone et Paragon: Sparrow — Epic Games, Fab (gratuit, licence Fab Standard). Le nom « Paragon » ne peut pas servir à présenter le jeu. Seuls le personnage, ses matériaux et 5 animations sont gardés.
