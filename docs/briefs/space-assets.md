# Assets 3D du spatial — première série (10)

Destination : `Content/RESHOOT/Space/Weapons/`, `Enemies/`, `Environment/`.

Export : FBX ou GLB, pivot au sol au centre (au bout de la crosse pour le fusil), échelle en mètres.

## Consigne de style commune

À coller **à la fin de chaque prompt**, identique pour les trois univers.

> Stylized low-poly game asset, clean readable silhouette, chunky simple shapes, bold flat colors with subtle hand-painted gradients, slightly exaggerated proportions, looks like a well-made movie prop, PBR, no text, no logos, single object centered, neutral background.

Palette du spatial, à ajouter après la consigne :

> Color palette: off-white panels, dark gunmetal grey, accents of electric cyan and neon blue, small warning orange details.

Rappels du GDD :

- on doit voir que c'est un **tournage** : accessoire de film, décor en contreplaqué ;
- le style doit être **le même pour les trois univers** ;
- un seul générateur principal pour tout le jeu. Je conseille de faire les trois premiers assets avec les trois outils, puis de garder celui qui respecte le mieux la consigne.

## Par ordre de priorité

| # | Asset | Pourquoi | Taille réelle | Budget |
| --- | --- | --- | --- | --- |
| 1 | Fusil laser | Toujours à l'écran | 0,8 m | 5–10k tris |
| 2 | Drone figurant | Le seul ennemi du spatial | 0,9 m | 5–8k tris |
| 3 | Caisse de cargo | Couvert principal, posée 20 fois | 1,2 × 1,2 × 1 m | 1–2k tris |
| 4 | Console de commande | Second couvert | 1,5 × 0,8 × 1,1 m | 3–5k tris |
| 5 | Panneau de coursive (décor de film) | Murs du décor, dos visible | 4 × 0,3 × 4 m | 2–4k tris |
| 6 | Hublot sur fond étoilé imprimé | Signature visuelle du décor | 2 × 2 m | 1–2k tris |
| 7 | Néon tubulaire bleu | Éclairage froid, posé partout | 2 m | < 1k tris |
| 8 | Lampe d'alarme rouge | Événement de gravité | 0,4 m | < 1k tris |
| 9 | Pile de bidons / réservoirs | Variété de couverts | 1 × 1 × 1,5 m | 2–3k tris |
| 10 | Projecteur de plateau sur pied | Envers du décor, commun aux trois univers | 2,5 m | 3–5k tris |

## Prompts prêts à coller

Pour chaque asset : le prompt, puis la consigne de style, puis la palette.

**1. Fusil laser**

> A retro-futuristic sci-fi laser rifle held in first person, chunky boxy body, glowing cyan energy cell on the side, cooling fins along the barrel, short stock, visible screws like a movie prop, the barrel ends in a ring-shaped emitter.

**2. Drone figurant**

> A small hovering sci-fi combat drone, round core body with one big glowing cyan eye-lens at the front, two short stubby side thrusters, a tiny antenna on top, a cheap-looking prop with a visible seam line and a painted serial number plate, menacing but a bit goofy.

**3. Caisse de cargo**

> A sci-fi cargo crate, sturdy rectangular box with reinforced corners, recessed side handles, hazard stripes on one edge, a small glowing status light, stackable, slightly worn edges.

**4. Console de commande**

> A sci-fi control console desk, slanted top panel with chunky buttons, levers and two small glowing screens, solid base you can take cover behind, cables coming out of the back.

**5. Panneau de coursive**

> A sci-fi corridor wall panel built as a film set flat: the front side is a clean spaceship wall with vents and light strips, the back side is raw plywood with wooden braces and sandbags at the foot, the panel is freestanding.

**6. Hublot étoilé**

> A round spaceship porthole window set in a thick metal frame with bolts, behind the glass a flat printed starfield backdrop with a blue planet, clearly a painted canvas prop.

**7. Néon tubulaire**

> A long sci-fi neon light tube fixture, glowing electric blue tube held by two metal brackets, a short power cable, simple and modular.

**8. Lampe d'alarme**

> A sci-fi rotating alarm beacon light, red dome on a short metal base with a protective cage, mounted on a wall bracket.

**9. Bidons / réservoirs**

> A cluster of three sci-fi fuel canisters strapped together on a small pallet, rounded cylinders with valves and pressure gauges, hazard labels without text.

**10. Projecteur de plateau**

> A professional film studio spotlight on a tall tripod stand, black metal housing with barn doors, a thick cable running to the floor, realistic movie set equipment.

Pour le projecteur (n° 10), remplacer la palette du spatial par : *Color palette: matte black, aluminium grey.* C'est le même objet dans les trois univers.

## Ce que je ferai à la réception

1. Import dans les dossiers ci-dessus via le MCP Unreal.
2. Collisions simples : boîte pour les caisses et les consoles, aucune pour les néons et le hublot.
3. Fusil sur `BP_SpaceLaser` ; drone sur `BP_SpaceDrone`, un Blueprint qui ne fait qu'assigner le mesh au composant `Body`.
4. Matériaux émissifs pour les parties cyan, utilisés aussi par le rayon du laser.
