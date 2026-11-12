# RESHOOT — règles pour tous les agents IA

Ce fichier est lu par toutes les IA de tous les devs (Claude Code, Codex, Antigravity).
Lis aussi `docs/ARCHITECTURE.md` et `docs/TASKS.md` avant d'écrire du code.

## Le jeu en cinq lignes

- FPS solo de 10 à 15 minutes, Unreal Engine 5.9, C++.
- Le joueur est l'acteur principal d'un film. Les ennemis sont des figurants (bots).
- Un réalisateur en voix off crie « CUT! », fige l'action, impose une directive, puis relance.
- Trois univers sur un même plateau : médiéval, spatial, film noir. Final qui alterne les trois.
- Jam de 4 jours : on préfère simple et fini à ambitieux et cassé.

## Règles absolues

1. **Tu ne touches qu'aux dossiers de ta tâche** (voir `docs/TASKS.md`). Jamais ceux d'un autre.
2. **Tu ne modifies pas `Source/RESHOOT/Core/`** sauf si ta tâche est le socle. Si une interface te manque, écris la demande dans `docs/TASKS.md`, section « Demandes au socle ».
3. **Tu ne lances jamais** `Build.bat`, `UnrealBuildTool`, `RunUAT`, ni l'éditeur. **Tu ne fermes jamais l'éditeur.** La compilation est faite par l'humain avec Live Coding (Ctrl+Alt+F11).
4. **Tu ne crées ni ne modifies de fichiers `.uasset` / `.umap`** à la main. Ce sont des binaires. Passe par Unreal MCP si l'humain te l'a branché, sinon décris ce qu'il doit créer dans l'éditeur.
5. **Pas de nouveaux plugins ni de dépendances** sans accord humain.

## Live Coding : comment écrire du code qui ne force pas de redémarrage

Live Coding recompile sans redémarrer l'éditeur **tant que seuls des corps de fonctions changent** (fichiers `.cpp`).

- Privilégie les modifications dans les `.cpp`.
- Un changement de `.h` (nouvelle classe, `UPROPERTY`, `UFUNCTION`, signature) demande en général un redémarrage.
- Si tu dois toucher un `.h` :
  1. Regroupe **tous** tes changements de headers en une seule fois.
  2. Termine ta réponse par la ligne `RESTART REQUIRED: <liste des .h modifiés>`.
  3. N'enchaîne pas d'autres changements de headers avant que l'humain confirme.
- Sinon, termine par `LIVE CODING OK` pour que l'humain fasse Ctrl+Alt+F11.
- Les valeurs de réglage (dégâts, vitesses, durées) vont dans des `UPROPERTY(EditAnywhere)` ou des Data Assets, pas en dur : on les ajuste dans l'éditeur sans recompiler.

## Conventions de code

- Module : `RESHOOT` (macro d'export `RESHOOT_API`, includes depuis la racine du module : `#include "Core/HealthComponent.h"`). Préfixes Unreal standards (`A`, `U`, `F`, `E`, `I`).
- Noms de classes en anglais, préfixés par le domaine : `AReshootEnemy`, `AMedievalKnight`, `USpaceDroneMovement`.
- Un fichier `.h` + un `.cpp` par classe, dans le dossier de son domaine.
- Logique en C++. Les Blueprints servent seulement à assigner des assets et régler des valeurs sur une classe C++.
- Pas de `Tick` si un timer ou un delegate suffit.
- Log avec la catégorie `LogReshoot` (déclarée dans `RESHOOT.h` : `#include "RESHOOT.h"`).
- Pas de refactoring non demandé, pas de fichiers hors du périmètre de la tâche.

## Arborescence

```
Source/RESHOOT/
  Core/       socle : vie, armes, ennemi de base, joueur, types communs
  Director/   gestionnaire de tournage, directives, audimat, répliques
  Medieval/   épée, bouclier, arc, figurants en armure
  Space/      apesanteur, laser, drones
  Noir/       revolver, gangsters, filtre noir et blanc
  UI/         menu, HUD, casting, écran de fin
Content/RESHOOT/
  Core/ Director/ Medieval/ Space/ Noir/ UI/ Audio/
docs/
  ARCHITECTURE.md  TASKS.md  DEVLOG.md
```

## Fin de tâche

1. Vérifie que tu n'as touché que ton périmètre (`git status`).
2. Ajoute une entrée dans `docs/DEVLOG.md` (modèle en tête du fichier).
3. Coche la tâche dans `docs/TASKS.md`.
4. Termine par `LIVE CODING OK` ou `RESTART REQUIRED: …`.
