# Brief Codex — le drone spatial (`ASpaceDrone`)

*À coller tel quel dans Codex.*

---

Tu travailles sur RESHOOT, un FPS solo en Unreal Engine 5.9 en C++ fait pendant une game jam de 4 jours. Le joueur est la vedette d'un film ; les ennemis sont des figurants. Avant d'écrire du code, lis dans l'ordre : `AGENTS.md`, `docs/ARCHITECTURE.md`, puis dans le GDD (`docs/RESHOOT — Game Design Document.md`) les sections « Ennemis » et « Level design ».

## Ta tâche

Implémenter le comportement du drone dans **`Source/RESHOOT/Space/SpaceDrone.cpp`**. Le header `SpaceDrone.h` est déjà écrit : n'y touche pas.

Comportement attendu (GDD) :

- **Déplacement.** Le drone vole en ligne droite vers le joueur, sans navmesh ni calcul de chemin. Il est en `MOVE_Flying` avec une gravité nulle (le constructeur le règle déjà). À chaque `SteerInterval`, un timer appelle `AddMovementInput` vers `GetTargetPlayer()` et oriente le drone vers lui. Il cesse d'avancer sous `StopDistance` : il reste sur place et tire. Vitesse maximale : `FlySpeed`, à appliquer à `MaxFlySpeed` dans `BeginPlay`.
- **Tir.** Quand le joueur est à moins de `FireRange` et visible (trace de visibilité), le drone tire `FireRate` fois par seconde. Il attend `FirstShotDelay` avant le premier tir. Chaque tir est une trace instantanée, avec une dispersion aléatoire de `SpreadDegrees`. S'il touche le joueur, il appelle `GetHealth()->ApplyDamage(DamagePerShot, this)` sur le `UHealthComponent` du joueur. Le tir est rendu visible par `ShotBeam` : un cylindre de 100 unités étiré de la position du drone au point d'impact, affiché environ 0,06 s. Le matériau `ShotMaterial` est appliqué s'il est défini. Le son `FireSound` est joué s'il est défini.
- **Gel au « CUT! ».** Tous tes timers (direction, tir, rayon) sont démarrés avec `GetWorldTimerManager().SetTimer(...)`, puis passés à `TrackFreezableTimer(Handle)`. La classe de base les met en pause au gel. Dans `OnFreezeChanged(true)`, cache le rayon et annule la vitesse. Un drone gelé ne bouge plus et ne tire plus.
- **Gravité.** `SetGravityEnabled(true)` : `GravityScale = 1`, `SetMovementMode(MOVE_Falling)`, plus de direction ni de tir pendant que le drone tombe. `SetGravityEnabled(false)` : `GravityScale = 0`, `MOVE_Flying`, le comportement reprend. C'est l'événement d'Argon qui appelle cette fonction ; tu ne touches pas à son fichier.
- **Mort.** `OnDeath` appelle `Super::OnDeath`, coupe les timers et cache le rayon. Pour une « mort théâtrale », le drone tombe en tournoyant : il passe en gravité, reçoit une petite impulsion et une rotation. `DeathSound` est joué s'il est défini.
- **Aucun `Tick`.** Utilise uniquement des timers.
- Le drone ne s'occupe pas du score : la classe de base prévient déjà le réalisateur à sa mort.

## Classes à utiliser

- `AReshootEnemy` (`Core/ReshootEnemy.h`) : `GetTargetPlayer()`, `IsFrozen()`, `IsDead()`, `TrackFreezableTimer()`, `HealthComponent`, ainsi que `OnFreezeChanged` et `OnDeath` à surcharger.
- `AReshootCharacter` (`Core/ReshootCharacter.h`) : `GetHealth()`, `GetFirstPersonCamera()` (pour viser la tête du joueur plutôt que ses pieds).
- `UHealthComponent` (`Core/HealthComponent.h`) : `ApplyDamage(Damage, Instigator)`.
- Log : `UE_LOG(LogReshoot, ...)` avec `#include "RESHOOT.h"`.
- Les réglages sont déjà des `UPROPERTY(EditAnywhere)` dans le header. Ne mets aucune valeur en dur.

## Périmètre exact

- Tu peux modifier : `Source/RESHOOT/Space/SpaceDrone.cpp`, et c'est tout.
- Tu ne touches pas à : `SpaceDrone.h`, `SpaceLaser.*` (lot de Claude), `SpaceGravityEvent.*` (lot d'Argon), `Core/`, `Director/`, les autres univers, les docs, sauf ton entrée dans `docs/DEVLOG.md` à la fin.
- S'il te manque une fonction du socle, ne la crée pas : écris la demande dans `docs/TASKS.md`, section « Demandes au socle ».

## Interdits (rappel d'AGENTS.md)

- Tu ne lances jamais `Build.bat`, UnrealBuildTool, RunUAT, ni l'éditeur. Tu ne fermes jamais l'éditeur. La compilation est faite par l'humain avec Live Coding.
- Aucun `.uasset` ni `.umap` créé ou modifié à la main. Tu ne pilotes pas l'éditeur par le MCP : une seule IA le fait à la fois, et c'est Claude. Si un asset est nécessaire (Blueprint `BP_SpaceDrone`, matériau), décris-le dans ta réponse : nom, dossier `Content/RESHOOT/Space/Enemies/`, réglages.
- Pas de nouveau plugin ni de nouvelle dépendance de module.
- Pas de refactoring hors de ton fichier.
- Pas de modification de header. Si c'est vraiment indispensable, arrête-toi et demande.

## Critère de fin

1. Ton code se compile par Live Coding : il ne change que le `.cpp`.
2. En PIE, sur `L_SocleTest` avec `DebugUniverse 2`, ou sur `L_Space`, trois drones posés dans le niveau :
   - ils foncent sur le joueur en ligne droite ;
   - ils s'arrêtent à `StopDistance` ;
   - ils tirent à moins de 15 m, avec un rayon visible ;
   - ils retirent 8 PV par tir au but ;
   - ils meurent en 4 tirs de laser.
3. Au « CUT! », les drones s'arrêtent net et ne tirent plus. Ils reprennent à « ACTION! ».
4. Un appel à `SetGravityEnabled(true)` fait tomber les drones au sol. `SetGravityEnabled(false)` les fait revoler.
5. Tu ajoutes une entrée dans `docs/DEVLOG.md` (modèle en tête du fichier) et tu coches « Drone » dans `docs/TASKS.md`.
6. `git status` ne montre que `SpaceDrone.cpp`, `DEVLOG.md` et `TASKS.md`.
7. Ta réponse se termine par `LIVE CODING OK`.
