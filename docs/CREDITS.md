# RESHOOT — assets et outils à déclarer à la soumission

Le règlement de la jam demande de lister les assets qu'on n'a pas faits. Ils ne sont pas notés.

## Assets Fab (non créés par l'équipe)

| Dossier dans `Content/` | Contenu | Utilisé pour | Titre exact et auteur sur Fab | Preuve de licence |
| --- | --- | --- | --- | --- |
| `Hallway_sci_fi` | Modules de couloir de vaisseau (murs, sols, colonnes, portes, lampes) | Pont spatial réaliste : sol, murs bas, sas, parements, couverts | À compléter (Jérémy) | Capture de la bibliothèque Fab |
| `Hollywood_Vol3` | Matériel de tournage (poutres, projecteurs, pieds) | Studio de la phase carton : gril, projecteurs | À compléter (Jérémy) | Capture de la bibliothèque Fab |
| `Fab/Laserrifle` | Fusil laser (mesh, matériaux, 7 textures) | Fusil laser réaliste du spatial | À compléter (Jérémy) | Capture de la bibliothèque Fab |
| `Sci_fi_hallway` | Second kit de couloir (blanc et rouge) | Pas encore utilisé | À compléter (Jérémy) | Capture de la bibliothèque Fab |

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
| Red Bricks 02 (texture) | Rob Tuytel | `T_Noir_Brick_*` (façades) |
| Asphalt 02 (texture) | Rob Tuytel | `T_Noir_Asphalt_*` (sol de la ruelle) |

## Assets générés pendant la jam (créés par l'équipe)

- Modèles 3D : Tripo (décors, drone, studio), Rodin (fusil laser). Sources dans `SourceArt/`.
- Images : fal (sol, mur). Le ciel et la planète sont maintenant procéduraux (`Tools/build_hangar.py`).

## Outils

Claude Code (Opus 5.5, Fable 5.2), Codex, Antigravity, Tripo, Rodin (Hyper3D), Meshy, fal, Blender (MCP for Blender), Poly Haven. Sons et musiques : WeLoveIndies (à confirmer).
