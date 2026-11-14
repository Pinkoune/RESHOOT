# RESHOOT — assets et outils à déclarer à la soumission

Le règlement de la jam demande de lister les assets qu'on n'a pas faits. Ils ne sont pas notés.

## Assets Fab (non créés par l'équipe)

| Dossier dans `Content/` | Contenu | Utilisé pour | Titre exact et auteur sur Fab | Preuve de licence |
| --- | --- | --- | --- | --- |
| `Hallway_sci_fi` | Modules de couloir de vaisseau (murs, sols, colonnes, portes, lampes) | Pont spatial réaliste : sol, murs bas, sas, parements, couverts | À compléter (Jérémy) | Capture de la bibliothèque Fab |
| `Hollywood_Vol3` | Matériel de tournage (poutres, projecteurs, pieds) | Studio de la phase carton : gril, projecteurs | À compléter (Jérémy) | Capture de la bibliothèque Fab |
| `Sci_fi_hallway` | Second kit de couloir (blanc et rouge) | Pas encore utilisé | À compléter (Jérémy) | Capture de la bibliothèque Fab |

## Contenu Epic (fourni avec le moteur)

| Dossier dans `Content/` | Contenu | Utilisé pour | Source |
| --- | --- | --- | --- |
| `Weapons/Rifle` | Fusil (mesh, matériau, textures) | Fusil laser réaliste du spatial | Modèle de projet First Person d'Unreal Engine 5.9 (`Templates/TemplateResources/Standard/Weapons`), licence Unreal Engine |

À vérifier avant la soumission : aucun de ces packs ne porte le tag « NoAI » sur Fab (leur licence interdit alors de les donner en entrée à une IA générative).

## Assets générés pendant la jam (créés par l'équipe)

- Modèles 3D : Tripo (décors, drone, studio), Rodin (fusil laser). Sources dans `SourceArt/`.
- Images : fal (sol, mur). Le ciel et la planète sont maintenant procéduraux (`Tools/build_hangar.py`).

## Outils

Claude Code (Opus 5.5, Fable 5.2), Codex, Antigravity, Tripo, Rodin (Hyper3D), Meshy, fal. Sons et musiques : WeLoveIndies (à confirmer).
