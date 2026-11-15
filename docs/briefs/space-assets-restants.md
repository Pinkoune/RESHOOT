# Spatial — ce qu'il reste à générer (14/11)

**Tout est fait et branché** (drone, sol, mur, ciel, planète, coque, studio). Reste seulement, côté studio : projecteur, chaise de réalisateur, clap. Le suivi est maintenant dans le guide des assets.

Fait : les 8 objets du spatial existent en `_Prop` et en `_Real`, importés, posés dans `L_Space`, branchés sur `M_SetDressing`. Le fusil a ses deux versions.

Ce qui manque, par ordre d'importance pour l'écran :

| # | Quoi | Où trouver le prompt | Outil | Pourquoi |
| --- | --- | --- | --- | --- |
| 1 | Drone `SM_Space_Drone_Prop` / `_Real` | Guide des assets, accessoires (`SM_Space_Drone`) + bloc carton / réaliste | Tripo | Le seul ennemi est encore une sphère grise |
| 2 | Sol du hangar spatial (texture qui se répète) | Ci-dessous | fal (image) | Après le rideau, on voit encore le sol gris du blockout |
| 3 | Mur de coque (texture qui se répète) | Ci-dessous | fal (image) | Idem pour les murs |
| 4 | Ciel : champ d'étoiles et nébuleuse (panorama) | Ci-dessous | fal (image) | Remplace le ciel étoilé procédural actuel |
| 5 | Grande planète + coque du vaisseau (plans peints) | Ci-dessous | fal (image) | Profondeur au-dessus des murs |
| 6 | Studio : projecteur, caméra, perche, passerelle, câbles, sacs de sable, flight case, chaise | Guide des assets, objets communs et envers du décor (`SM_Stage_*`) | Tripo | Remplacent les formes grises du studio carton (une seule version, réaliste) |

## Textures (fal, image carrée 2048 × 2048)

Ajouter à chaque prompt : `seamless tileable texture, top-down orthographic, even flat lighting, no shadows, no perspective, no text, no logo`.

- **Sol** — `T_Space_Floor` : `sci-fi spaceship hangar floor, large square white and light grey metal plates, thin dark seams, small bolts at the corners, subtle scuffs and orange hazard markings on a few plates`
- **Mur** — `T_Space_Wall` : `sci-fi spaceship hull interior wall, white composite panels with recessed grey vents and thin orange stripes, modular grid`

Je les branche sur un matériau réaliste qui se répète, sur le sol et les murs ; le blockout passe invisible en jeu.

## Images peintes (fal, panorama 21:9, 4K si possible)

Ajouter : `matte painting, cinematic, wide panorama, no characters, no text, no logo`.

- **Ciel** — `T_Space_SkyPanorama` : `deep space starfield with a soft blue and violet nebula, dense small stars, a few bright stars with gentle glow, dark black background`
- **Planète** — `T_Space_Planet` : `huge blue ocean planet with swirling white clouds seen from orbit, thin glowing atmosphere rim, black space background` (fond noir pur, il sera rendu transparent)
- **Coque** — `T_Space_HullBackdrop` : `exterior of a giant white spaceship hull stretching into the distance, panels, antennas and blue running lights, black space background`

## Dépôt des fichiers

Comme pour les objets : un zip par asset dans `Content/RESHOOT/Space/Meshes/` (ou les images en PNG dans `Content/RESHOOT/Space/Textures/`). Je dézippe dans `SourceArt/`, j'importe et je pose.
