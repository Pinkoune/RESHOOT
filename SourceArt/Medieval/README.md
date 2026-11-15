# Sources des assets médiévaux

Fichiers bruts des générateurs (Tripo pour les décors, Rodin pour les armes et le casque), dézippés tels quels.
Rien ici n'est lu par le jeu : on importe dans `Content/RESHOOT/Medieval/Meshes/<Objet>/` (la version réaliste dans `Real/`),
comme décrit dans `docs/RESHOOT — Guide de génération des assets.md`.

| Objet | Carton (`Prop/`) | Réaliste (`Real/`) | Remarque |
| --- | --- | --- | --- |
| `SM_Medieval_CastleWall` | oui | oui | |
| `SM_Medieval_Battlements` | oui | oui | |
| `SM_Medieval_Barrel` | oui | oui | |
| `SM_Medieval_Cart` | oui | oui | |
| `SM_Medieval_Convoy` | oui (cheval de bois) | oui (chariot bâché) | |
| `SM_Medieval_Convoy_Wood` | — | oui | Cheval de bois en version soignée ; les fichiers s'appellent `SM_Medieval_Convoy_Cartoon.*` |
| `SM_Medieval_Banner` | oui | oui | |
| `SM_Medieval_Torch` | oui | oui | |
| `SM_Medieval_Sword` | pas de carton | oui (Rodin) | Rodin : `base.obj` + `texture_*.png`, `shaded.png` = aperçu |
| `SM_Medieval_Shield` | pas de carton | oui (Rodin) | idem |
| `SM_Medieval_Bow` | pas de carton | oui (Rodin) | idem, plus une version `.glb` |
| `SM_Medieval_Helmet` | à générer | oui (Rodin) | idem |

Pas encore générés : ciel et collines (`T_Medieval_SkyPanorama`, `T_Medieval_Hills`), sol et mur (`T_Medieval_Floor`, `T_Medieval_Wall`), rails du convoi en carton (`SM_Medieval_Rails_Prop`).

Studio (`SourceArt/Stage/`) : `SM_Stage_Clapperboard`, `SM_Stage_DirectorChair`, `SM_Stage_Spotlight` sont nouveaux.
`SourceArt/Stage/Variantes/` contient une deuxième génération de six objets déjà importés par Jérémy (perche, câbles, caméra, passerelle, flight case, sacs de sable) : gardés pour comparaison, pas à importer en double.

Armes (épée, bouclier, arc) : pas de version carton. Elles n'apparaissent qu'après le clap de début, quand le réalisateur guide l'acteur, donc dans la scène réaliste.
