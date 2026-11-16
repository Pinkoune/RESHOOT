# Props du film noir : modèles « réalistes » (GLB)

Remplacent les objets du Noir construits en cubes et cylindres par `Tools/build_noir_set.py`.
Les bibliothèques en ligne de l'addon Blender (Poly Haven, Sketchfab) étaient désactivées : tout est
**modélisé en Python bpy** (Blender 5.2), texturé avec des photos Poly Haven déjà téléchargées pour le Noir.

## Fichiers

| Fichier | Objet | Dimensions L × P × H (cm) | Triangles | Source |
| --- | --- | --- | --- | --- |
| `SM_Noir_MarketStall_Real.glb` | Étal de marché : table sur tréteaux, auvent en toile rayée festonnée sur 4 poteaux, cageots de pommes, oranges et choux, ardoises de prix, balance suspendue, cageots au sol | 308 × 137 × 258 | 9 488 | Modélisé |
| `SM_Noir_PhoneBooth_Real.glb` | Cabine téléphonique en bois : porte pliante vitrée à petits carreaux, côtés vitrés, fond plein, enseigne TELEPHONE sur les 4 faces, téléphone mural à pièces (cadran, combiné), tablette avec annuaire, siège, plafonnier | 107 × 107 × 243 | 7 634 | Modélisé |
| `SM_Noir_Newsstand_Real.glb` | Kiosque à journaux en planches : comptoir en saillie avec piles de journaux et presse-papiers, auvent rayé festonné, revues pincées sur un fil et accrochées aux montants, enseigne NEWS, bandeau DAILY PAPERS · MAGAZINES, affiches sur les côtés, étagères intérieures, tabouret | 236 × 160 × 275 | 7 764 | Modélisé |
| `SM_Noir_Sawhorse_Real.glb` | Chevalet de police : planche à rayures diagonales noir/blanc, plaque POLICE des deux côtés, pieds en A peints en blanc, lanterne à pétrole rouge posée dessus | 120 × 64 × 118 (planche à 95) | 2 404 | Modélisé |
| `SM_Noir_Dumpster_Real.glb` | Benne à ordures en tôle : avant incliné, nervures, fourreaux de levage, roulettes pivotantes, deux couvercles dont un entrouvert (16°) sur des ordures, papiers froissés et un carton | 209 × 123 × 162 (caisse à 125) | 3 316 | Modélisé |
| `SM_Noir_Mailbox_Real.glb` | Boîte aux lettres US bombée sur pieds en cornière : trappe PULL, porte de relevage avec serrure et plaque d'horaires, U.S. MAIL sur l'avant et les flancs | 69 × 66 × 125 | 5 574 | Modélisé |

`SM_Noir_FireAlarmBox_Real` n'a pas été fait : la boîte aux lettres l'a remplacée.

## Conventions

- Échelle réelle en mètres. Pivot en bas au centre (pieds à z = 0). Un seul mesh par fichier.
- **Face avant vers +Y de Blender.** L'export glTF est en Y-up : +Y Blender devient −Z glTF. Vérifiez le sens après
  l'import dans Unreal et tournez de 90° ou 180° en lacet au besoin. Avant = porte de la cabine, comptoir du kiosque,
  côté client de l'étal (l'auvent descend vers l'avant), avant incliné de la benne, trappe de la boîte aux lettres.
- Plusieurs slots de matériau par mesh, nommés `M_Noir_<Objet>_<Matière>`. Les textures (JPEG 1k ou 512) sont dans
  le GLB. On peut remplacer chaque slot par les `MI_Noir_*Tex_Real` existants : les UV sont des projections
  boîte à l'échelle réelle (1 répétition ≈ 1 m pour le bois, 0,5 m pour la toile).
- Vitres de la cabine : `M_Noir_Booth_Glass`, alphaMode BLEND (alpha 0,28).
- Pas de collision : utiliser une collision simple (boîte) ou la complexe comme simple dans Unreal.

## Textures

| Texture | Source | Licence |
| --- | --- | --- |
| Weathered Planks, Wood Planks Grey, Green Metal Rust, Rusty Metal 02 (diffuse 1k) | Poly Haven | CC0 |
| Bois de cageot, bois peint blanc/noir écaillé, rayures du chevalet, peinture olive de la boîte aux lettres | Dérivées des photos Poly Haven ci-dessus (numpy) | CC0 |
| Toile rayée, toile unie, une de journal, tranche de pile, couvertures de revues, affiche EXTRA | Générées (numpy), sans texte lisible | Création de l'équipe |
| Lettrage (TELEPHONE, NEWS, POLICE, U.S. MAIL…) | Géométrie extrudée, police Arial Bold de Windows convertie en maillage | — |
