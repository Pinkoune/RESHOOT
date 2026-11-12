# RESHOOT — architecture

Seul le responsable du socle modifie ce fichier et `Source/RESHOOT/Core/`.
Objectif du jour 1 matin : **créer tous les headers ci-dessous, même avec des corps vides**, puis un seul redémarrage de l'éditeur. Ensuite, presque tout le travail se fait dans les `.cpp` et passe par Live Coding.

## Types communs — `Core/ReshootTypes.h`

```cpp
UENUM(BlueprintType)
enum class EUniverse : uint8 { None, Medieval, Space, Noir };

UENUM(BlueprintType)
enum class EShootPhase : uint8 { Menu, Casting, SceneIntro, TakeRunning, Cut, SceneTransition, Finale, Wrap };

USTRUCT(BlueprintType)
struct FKillInfo
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) EUniverse Universe = EUniverse::None;
    UPROPERTY(BlueprintReadOnly) bool bMelee = false;
    UPROPERTY(BlueprintReadOnly) bool bHeadshot = false;
    UPROPERTY(BlueprintReadOnly) bool bPlayerMoving = false;
    UPROPERTY(BlueprintReadOnly) float PlayerHealthPct = 1.f;
};
```

## Socle — `Core/`

Includes depuis la racine du module : `#include "Core/HealthComponent.h"`. Catégorie de log `LogReshoot` dans `RESHOOT.h`.

| Classe | Rôle | API publique |
| --- | --- | --- |
| `UHealthComponent` | Vie et dégâts, pour le joueur et les ennemis | `float ApplyDamage(float Damage, AActor* DamageInstigator, bool bHeadshot = false, bool bMelee = false)` (retourne les dégâts appliqués), `Heal(float)`, `ResetHealth()`, `GetHealth()`, `GetMaxHealth()`, `GetHealthPct()`, `IsDead()`, `SetInvulnerable(bool)`, `SetFrontalBlock(bool, float Multiplier)` (bouclier) ; delegates `OnHealthChanged(Comp, NewHealth, Delta)`, `OnDeath(Comp, Killer, bHeadshot, bMelee)` ; réglages `MaxHealth`, `HeadshotMultiplier`, `FrontalBlockHalfAngle` |
| `AReshootWeapon` | Classe de base des armes (abstraite) | virtuelles `StartPrimary()`, `StopPrimary()`, `StartSecondary()`, `StopSecondary()`, `Reload()`, `SetEnabled(bool)`, `GetAmmo()`, `GetMaxAmmo()` (-1 = infini) ; `IsEnabled()`, `GetOwnerCharacter()`, `DisplayName` ; aides protégées `GetAim()`, `TraceFromAim(Range, Hit)`, `IsHeadshot(Hit)`, `DealDamage(Target, Damage, bHeadshot, bMelee)` |
| `AReshootCharacter` | Joueur (vue FPS) | `SetUniverse(EUniverse)` : change d'arme et de mode de déplacement ; `GetHealth()` (retourne le `UHealthComponent*`) ; `EquipWeapon(Class)`, `SetFrozen(bool)`, `GetCurrentWeapon()`, `GetUniverse()`, `GetFirstPersonCamera()`, `IsSprinting()`, `IsSliding()`, `IsMoving()` ; delegates `OnWeaponChanged`, `OnUniverseChanged` ; commande console `DebugUniverse 0..3` |
| `AReshootEnemy` | Classe de base des ennemis (abstraite, `ACharacter`) | `Universe`, `HealthComponent`, `GetTargetPlayer()`, `Freeze(bool)`, `IsFrozen()`, `IsDead()`, delegate `OnEnemyDied` ; virtuelles protégées `OnFreezeChanged(bool)`, `OnDeath(Killer, bHeadshot, bMelee)`, `BuildKillInfo(...)` ; à la mort, notifie le `UShootDirectorSubsystem` avec un `FKillInfo` |
| `UUniverseDefinition` (`UPrimaryDataAsset`) | Décrit un univers | `Universe`, `DisplayName`, `WeaponClass`, `EnemyClasses`, `SubLevelName`, `MovementMode`, `GravityScale`, `BrakingDeceleration`, `PostProcessMaterial`, `EventClass` |
| `AUniverseEvent` | Classe de base des événements d'univers | `StartEvent()`, `StopEvent()`, `IsEventActive()` |
| `AReshootGameMode` | Pion par défaut `AReshootCharacter`, GameState `AReshootGameState` | `ShootScript` (transmis au réalisateur au BeginPlay), `bAutoStartShoot` (test) |
| `AReshootGameState` | Porte les composants partagés | `GetAudienceRating()`, `GetDirectorVoice()` |

Règles :

- Une arme ne connaît pas le réalisateur. Elle inflige des dégâts via `UHealthComponent::ApplyDamage`, rien d'autre.
- Un ennemi ne gère ni score ni directive. Il meurt et notifie.
- `Freeze(true)` doit stopper mouvement, tirs et timers d'un ennemi (utilisé par « CUT! »).

## Déplacement

Le déplacement est codé uniquement dans `AReshootCharacter` (Core). Aucun univers n'écrit de code de déplacement.

- Touches identiques partout : ZQSD, Espace, Ctrl, Maj (sprint), clic gauche, clic droit, R.
- `Medieval` et `Noir` : `MOVE_Walking`, mêmes valeurs. Sprint, saut, accroupi,
  glissade (accroupi pendant le sprint : friction réduite + impulsion),
  saut en sortie de glissade qui conserve l'élan, contrôle aérien élevé,
  pas de dégâts de chute. Escalade des rebords : optionnelle, pas avant samedi.
- `Space` : `MOVE_Flying`, mêmes vitesses max. ZQSD suit la direction de la caméra,
  Espace monte, Ctrl descend, sprint = boost, freinage faible pour l'inertie.
- Les seuls paramètres par univers sont dans `UUniverseDefinition` :
  `MovementMode`, `GravityScale`, `BrakingDeceleration`. Sans définition, `Space` = vol, gravité 0,
  `DefaultSpaceBraking` ; les autres = marche, `DefaultWalkingBraking`.
- Toutes les valeurs (vitesses, glissade, apesanteur) sont des `UPROPERTY(EditAnywhere)` de `AReshootCharacter`.
- Entrées : Enhanced Input. Si `DefaultMappingContext` n'est pas assigné sur le Blueprint du joueur,
  le contexte et les actions sont créés en C++ au lancement (ZQSD + WASD, souris, Espace, Ctrl, Maj, clics, R).

## Réalisateur — `Director/`

| Classe | Rôle | API publique |
| --- | --- | --- |
| `UShootDirectorSubsystem` (`UWorldSubsystem`, Game/PIE seulement) | Machine à états de la partie | `StartShoot()`, `GetPhase()`, `GetCurrentUniverse()`, `NotifyKill(const FKillInfo&)`, `NotifyPlayerDeath()` ; delegates `OnPhaseChanged(EShootPhase)`, `OnUniverseChanged(EUniverse)`, `OnCut()`, `OnAction()`, `OnDirectiveChanged(UDirective*)` ; compléments `SetShootScript()`, `GetShootScript()`, `GetUniverseDefinition(EUniverse)`, `GetCurrentDirective()`, `GetSceneIndex()`, `GetTakeIndex()`, `CallCut(UDirective*)`, `CallAction()`, `SetUniverse(EUniverse)`, `FreezeAll(bool)` |
| `UShootScript` (`UPrimaryDataAsset`) | Déroulé : scènes, prises, vagues | Données : `Scenes` (`FSceneDef` : `UniverseDefinition`, `IntroLineId`, `Takes`), `FTakeDef` (`Waves`, `Directive`, `ActionLineId`, `CutLineId`), `FWaveDef` (`Count`, `EnemyClassesOverride`, `SpawnInterval`, `StartDelay`), `FinaleUniverses`, `FinaleSwitchInterval`, `FinaleDuration`, `FinaleWaves`, `BaseKillRating`, `CutDuration`, `RetakeDelay`, `RetakeLineId`, `WrapLineId` ; `FindUniverseDefinition(EUniverse)` |
| `UDirective` (`UPrimaryDataAsset`, une sous-classe C++ par type) | Une contrainte du réalisateur | `Title`, `VoiceLineId`, virtuelles `OnBegin(UShootDirectorSubsystem*)`, `OnEnd(UShootDirectorSubsystem*)`, `EvaluateKill(const FKillInfo&) -> float` (multiplicateur d'audimat), sans `Tick` |
| `UAudienceRating` (composant de `AReshootGameState`) | Score | `Add(float)`, `ResetRating()`, `GetRating()`, `GetRatingPct()`, `GetStars()`, delegate `OnRatingChanged(NewRating, Delta)` ; réglages `MaxRating`, `StarThresholds` |
| `UDirectorVoice` (composant de `AReshootGameState`) | Répliques | `float PlayLine(FName LineId)` : joue le son et pousse le sous-titre, retourne la durée ; delegate `OnSubtitle(LineId, Subtitle, Duration)` ; `LinesTable` = DataTable `DT_DirectorLines` de lignes `FDirectorLine` (`Sound`, `Subtitle`, `SubtitleDuration`) |
| `AWaveSpawner` (un par niveau) | Fait apparaître les ennemis d'une vague | `SpawnWave(const FWaveDef&)`, `KillAll()`, `ReplaceAllForUniverse(EUniverse)`, `FreezeAll(bool)`, `GetAliveCount()`, `IsSpawning()`, delegate `OnWaveCleared` ; `SpawnPoints` (vide = cercle de `SpawnRadius`) |

Déroulé d'une prise :

1. `SceneIntro` : charge le sous-niveau de l'univers, `PlayLine`, clap.
2. `TakeRunning` : vagues actives, directive éventuelle.
3. `Cut` : `OnCut` → tout le monde `Freeze(true)`, réplique, nouvelle directive, puis `OnAction`.
4. Fin de prise : audimat mis à jour, prise suivante ou `SceneTransition`.
5. `Finale` : `SetUniverse` toutes les 20 s, `ReplaceAllForUniverse`.
6. `Wrap` : écran de fin.

Mort du joueur : `NotifyPlayerDeath()` → réplique « on la refait », la prise recommence.

## Univers — `Medieval/`, `Space/`, `Noir/`

Chaque univers fournit, dans son dossier uniquement :

- une arme dérivée de `AReshootWeapon` ;
- un ou deux ennemis dérivés de `AReshootEnemy` ;
- un événement dérivé de `AUniverseEvent` ;
- un `UUniverseDefinition` et un sous-niveau d'habillage dans `Content/RESHOOT/<Univers>/`.

Simplifications imposées :

- **Medieval** : épée = un coup dans un cône ; bouclier levé = dégâts de face réduits ; arc = projectile chargé.
- **Space** : joueur en `MOVE_Flying` avec inertie ; drones en ligne droite vers le joueur, sans navmesh.
- **Noir** : revolver hitscan avec chargeur ; gangsters à couvert sur navmesh ; post-process noir et blanc.

## Interface — `UI/`

L'interface écoute les delegates, elle n'appelle jamais la logique de jeu sauf `StartShoot()` depuis le menu.

- HUD : vie, arme, directive en cours, jauge d'audimat, sous-titres.
- Menu clap, casting, écran de fin (affiche + étoiles via `UAudienceRating::GetStars()`).

## Le plateau

Un niveau persistant `L_Stage` (géométrie de base, éclairage neutre, points d'apparition) et un sous-niveau par univers (`L_Medieval`, `L_Space`, `L_Noir`) chargé en streaming. Personne d'autre que le responsable du socle ne modifie `L_Stage`.
