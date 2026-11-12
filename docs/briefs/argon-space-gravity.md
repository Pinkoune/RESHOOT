# Brief Argon (Antigravity) — événement de gravité, puis habillage de `L_Space`

*À coller tel quel dans Antigravity.*

---

Tu travailles sur RESHOOT, un FPS solo en Unreal Engine 5.9 / C++ fait pendant une game jam de 4 jours. Le joueur est la vedette d'un film ; le réalisateur fige l'action (« CUT! ») et change les règles. Avant d'écrire du code, lis dans cet ordre : `AGENTS.md`, `docs/ARCHITECTURE.md`, et dans le GDD (`docs/RESHOOT — Game Design Document.md`) les sections « Level design » et « Direction artistique ».

## Partie 1 — l'événement « la gravité se rallume » (`ASpaceGravityEvent`)

Tu implémentes l'événement dans **`Source/RESHOOT/Space/SpaceGravityEvent.cpp`**. Le header `SpaceGravityEvent.h` est déjà écrit : n'y touche pas.

Ce que dit le GDD : « La gravité artificielle se rallume 10 secondes : joueur et drones retombent au sol. » L'événement a lieu une fois par scène. Le réalisateur l'annonce, puis appelle `StartEvent()`.

Ce que fait chaque fonction :

- `StartEvent()`
  - Appelle `Super::StartEvent()` : la base passe `bEventActive` à vrai, arme le timer de `Duration` (10 s) qui appellera `StopEvent`, puis diffuse `OnEventStateChanged`.
  - Appelle ensuite `ApplyGravity(true)`.
  - Joue `GravityOnSound`.
  - Lance le clignotement des lumières d'alarme.
- `StopEvent()`
  - Appelle `ApplyGravity(false)`.
  - Arrête l'alarme et remet les lumières dans leur état d'origine.
  - Joue `GravityOffSound`.
  - Appelle `Super::StopEvent()`.
  - Doit pouvoir être appelé deux fois sans effet de bord.
- `ApplyGravity(bool)`
  - Pour le joueur : `AReshootCharacter::SetSpaceGravity(bGravityOn)`. Récupère le joueur avec `UGameplayStatics::GetPlayerPawn(this, 0)`.
  - Si `bAffectDrones` : pour chaque `ASpaceDrone` vivant (`TActorIterator`), appelle `SetGravityEnabled(bGravityOn)`.
- `BlinkAlarm()`
  - Timer en boucle de `AlarmBlinkInterval`.
  - Allume et éteint chaque `ULightComponent` des acteurs qui portent le tag `AlarmLightTag`.
- `SetEventPaused(bool)`
  - Gel au « CUT! ».
  - Appelle `Super` : il met en pause le timer de durée.
  - Mets aussi en pause le timer d'alarme.
- `EndPlay`
  - Si l'événement est actif, appelle `StopEvent()` pour ne jamais laisser le joueur en gravité.
- Pas de `Tick`, seulement des timers.

### Classes à utiliser

- `AUniverseEvent` (`Core/UniverseEvent.h`) : classe de base, avec `Duration`, `bEventActive` et `DurationTimer`.
- `AReshootCharacter::SetSpaceGravity(bool)` (`Core/ReshootCharacter.h`) : déjà codé dans le socle. Sans effet hors de l'univers spatial.
- `ASpaceDrone::SetGravityEnabled(bool)` (`Space/SpaceDrone.h`) : codé par Codex en parallèle. Tu l'appelles, tu ne modifies pas son fichier.
- Log : `UE_LOG(LogReshoot, ...)` avec `#include "RESHOOT.h"`.

### Test

Pose un `ASpaceGravityEvent` dans le niveau. Lance la PIE, puis tape la commande console `DebugUniverse 2`.

Appelle ensuite `StartEvent` : depuis le Level Blueprint de test, ou par la console avec `ke * StartEvent`.

Ce que tu dois observer :

- le joueur retombe au sol et marche normalement pendant 10 s ;
- il revole ensuite tout seul ;
- les drones tombent, puis revolent ;
- les lumières taguées `GravityAlarm` clignotent pendant l'événement.

## Partie 2 — habillage de `L_Space` (après la partie 1)

L'univers spatial est un habillage posé sur le plateau commun.

- `L_Space` est un sous-niveau de `L_Stage`, chargé en streaming par le réalisateur.
- Il ne contient **que** du décor, des lumières et des couverts, jamais de gameplay.
- La géométrie de jeu (sol, rampes, portes) est dans `L_Stage`, qui appartient à Jérémy. Tu n'y touches pas.

Ce que dit le GDD :

- **Décor.** Hangar de station, coursives, hublots sur un fond étoilé imprimé.
- **Lumière.** Froide, néons bleus.
- **Couverts.** Caisses et consoles.
- **Envers du décor visible.** Les panneaux montrent leur dos en bois, avec des étais.
- **Centre ouvert en hauteur.** Les drones n'ont pas de navmesh. Tu ne poses rien entre 2 m et 10 m de hauteur au centre du plateau.
- **Lumières d'alarme.** Pose 2 ou 3 lumières rouges avec le tag `GravityAlarm`, éteintes par défaut.

Étapes :

1. **Boîtes grises.** Fais une passe avec des cubes et des cylindres de base, à l'échelle du plateau (environ 50 × 40 m).
2. **Assets générés.** Remplace-les ensuite par les assets générés, qui arrivent dans `Content/RESHOOT/Space/Environment/`. Jérémy les génère d'après `docs/briefs/space-assets.md`.

### Comment tu touches au niveau

Tu ne crées ni ne modifies de `.umap` ou de `.uasset` à la main. Une seule IA à la fois pilote l'éditeur par le MCP Unreal, et par défaut c'est Claude.

Tu as deux possibilités :

- **Jérémy te donne explicitement la main sur le MCP** pendant une session. Tu peux alors construire `L_Space` dans l'éditeur. À la fin, tu rends la main en le disant.
- **Sinon**, tu écris un plan de placement : une liste d'acteurs avec leur mesh, position, rotation, échelle et tags. Tu le mets dans `docs/briefs/l-space-layout.md`. Claude ou Jérémy l'appliqueront.

## Périmètre exact

Tu peux modifier :

- `Source/RESHOOT/Space/SpaceGravityEvent.cpp` ;
- `Content/RESHOOT/Space/Maps/L_Space` et `Content/RESHOOT/Space/Environment/`, via le MCP et seulement quand la main t'est donnée ;
- `docs/briefs/l-space-layout.md`.

Tu ne touches pas à :

- `SpaceGravityEvent.h` ;
- `SpaceDrone.*` (lot de Codex) ;
- `SpaceLaser.*` (lot de Claude) ;
- `Core/`, `Director/` et les autres univers ;
- `L_Stage`.

Seule exception : ton entrée dans `docs/DEVLOG.md` et ta case dans `docs/TASKS.md`.

S'il te manque une fonction du socle, écris la demande dans `docs/TASKS.md`, section « Demandes au socle ».

## Interdits (rappel d'AGENTS.md)

- Tu ne lances jamais `Build.bat`, UnrealBuildTool, RunUAT, ni l'éditeur. Tu ne fermes jamais l'éditeur. L'humain compile avec Live Coding.
- Aucun `.uasset` ni `.umap` à la main.
- Pas de nouveau plugin ni de dépendance.
- Pas de refactoring hors de ton périmètre.
- Pas de modification de header. Si c'est indispensable, arrête-toi et demande.

## Critère de fin

**Partie 1**

1. Tu n'as modifié que le `.cpp` : Live Coding suffit.
2. Le test ci-dessus passe : 10 s de gravité pour le joueur et les drones, l'alarme clignote, l'apesanteur revient toute seule.
3. Au « CUT! », l'événement se met en pause, puis reprend.
4. Entrée dans `docs/DEVLOG.md`, case « Événement de coupure de gravité » cochée dans `docs/TASKS.md`.
5. Ta réponse se termine par `LIVE CODING OK`.

**Partie 2**

1. `L_Space` est jouable en boîtes grises : centre dégagé, couverts, lumière froide, lumières d'alarme taguées.
2. Après réception des assets, il est habillé et une capture est ajoutée dans `docs/devlog-media/`.
3. Case « Sous-niveau `L_Space` » cochée dans `docs/TASKS.md`.
