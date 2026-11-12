# RESHOOT — architecture

Seul le responsable du socle modifie ce fichier et `Source/RESHOOT/Core/`.
Objectif du jour 1 matin : **créer tous les headers ci-dessous, même avec des corps vides**, puis un seul redémarrage de l'éditeur. Ensuite, presque tout le travail se fait dans les `.cpp` et passe par Live Coding.

Deuxième (et dernier prévu) lot de headers le 12/11 au soir : signaux des armes, restrictions des directives, casting, nom du joueur, bilan de fin, réglages du GDD, et les headers complets du lot spatial (`SpaceLaser`, `SpaceDrone`, `SpaceGravityEvent`).

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
    UPROPERTY(BlueprintReadOnly) bool bPlayerSliding = false;   // « Cascade »
    UPROPERTY(BlueprintReadOnly) bool bPlayerInAir = false;     // « Cascade »
    UPROPERTY(BlueprintReadOnly) int32 WeaponMode = 0;          // 0 = épée, 1 = arc
};

UENUM(BlueprintType)  // diffusé par les armes
enum class EWeaponEvent : uint8 { Fired, MeleeSwing, HitConfirmed, Reloaded, SecondaryStarted, SecondaryStopped, ModeChanged, Overheated, CooledDown };

UENUM(BlueprintType)  // ce qu'une directive peut interdire
enum class EWeaponAction : uint8 { Primary, Secondary, Reload, SwitchMode };

USTRUCT(BlueprintType)  // bilan pour l'écran de fin
struct FShootStats { int32 Kills, DirectiveKills, Headshots, FailedTakes, CompletedTakes; };
```

## Socle — `Core/`

Includes depuis la racine du module : `#include "Core/HealthComponent.h"`. Catégorie de log `LogReshoot` dans `RESHOOT.h`.

| Classe | Rôle | API publique |
| --- | --- | --- |
| `UHealthComponent` | Vie et dégâts, pour le joueur et les ennemis | `float ApplyDamage(float Damage, AActor* DamageInstigator, bool bHeadshot = false, bool bMelee = false)` (retourne les dégâts appliqués), `Heal(float)`, `ResetHealth()`, `GetHealth()`, `GetMaxHealth()`, `GetHealthPct()`, `IsDead()`, `SetInvulnerable(bool)`, `SetFrontalBlock(bool, float Multiplier)` (bouclier) ; delegates `OnHealthChanged(Comp, NewHealth, Delta)`, `OnDeath(Comp, Killer, bHeadshot, bMelee)` ; réglages `MaxHealth`, `HeadshotMultiplier`, `FrontalBlockHalfAngle` |
| `AReshootWeapon` | Classe de base des armes (abstraite) | virtuelles `StartPrimary()`, `StopPrimary()`, `StartSecondary()`, `StopSecondary()` (diffusent `SecondaryStarted/Stopped`), `Reload()`, `SetEnabled(bool)`, `GetAmmo()`, `GetMaxAmmo()` (-1 = infini), `SelectMode(int32)`, `GetMode()`, `GetModeCount()`, `Refill()`, `GetGaugePct()` (-1 = pas de jauge), `PreventsSprint()` ; `CycleMode(Dir)`, `IsEnabled()`, `GetOwnerCharacter()`, `GetCostMultiplier()`, `IsActionAllowed(EWeaponAction)`, `DisplayName` ; delegate `OnWeaponEvent(Weapon, EWeaponEvent)` ; aides protégées `GetAim()`, `TraceFromAim(Range, Hit)`, `IsHeadshot(Hit)`, `DealDamage(Target, Damage, bHeadshot, bMelee)` (diffuse `HitConfirmed`), `BroadcastWeaponEvent(EWeaponEvent)` |
| `AReshootCharacter` | Joueur (vue FPS) | `SetUniverse(EUniverse)` : change d'arme et de mode de déplacement ; `GetHealth()` (retourne le `UHealthComponent*`) ; `EquipWeapon(Class)`, `SetFrozen(bool)`, `GetCurrentWeapon()`, `GetUniverse()`, `GetFirstPersonCamera()`, `IsSprinting()`, `IsSliding()`, `IsMoving()`, `IsInAir()`, `ResetForTake(Transform)` ; restrictions `SetWeaponActionAllowed(EWeaponAction, bool)`, `IsWeaponActionAllowed()`, `SetWeaponCostMultiplier(float)`, `ClearWeaponRestrictions()` ; visée `SetAimFOV(float)`, `ResetAimFOV()` ; apesanteur `SetSpaceGravity(bool)` ; delegates `OnWeaponChanged`, `OnUniverseChanged`, `OnPlayerWeaponEvent(Weapon, EWeaponEvent)` (relais de l'arme courante) ; commande console `DebugUniverse 0..3` |
| `AReshootEnemy` | Classe de base des ennemis (abstraite, `ACharacter`) | `Universe`, `HealthComponent`, `GetTargetPlayer()`, `Freeze(bool)`, `IsFrozen()`, `IsDead()`, delegate `OnEnemyDied` ; virtuelles protégées `OnFreezeChanged(bool)`, `OnDeath(Killer, bHeadshot, bMelee)`, `BuildKillInfo(...)` ; protégée `TrackFreezableTimer(Handle)` (le timer est mis en pause au gel) ; à la mort, notifie le `UShootDirectorSubsystem` avec un `FKillInfo` |
| `UUniverseDefinition` (`UPrimaryDataAsset`) | Décrit un univers | `Universe`, `DisplayName`, `WeaponClass`, `EnemyClasses`, `SubLevelName`, `MovementMode`, `GravityScale`, `BrakingDeceleration`, `PostProcessMaterial`, `EventClass`, `EventLineId`, `Music`, `PosterTitleWords` |
| `AUniverseEvent` | Classe de base des événements d'univers | `StartEvent()`, `StopEvent()` (auto après `Duration`, 0 = jamais), `IsEventActive()`, `SetEventPaused(bool)` (gel au CUT), delegate `OnEventStateChanged(Event, bActive)` |
| `AReshootGameMode` | Pion par défaut `AReshootCharacter`, GameState `AReshootGameState` | `ShootScript` (transmis au réalisateur au BeginPlay), `bAutoStartShoot` (test), `bSkipCasting` |
| `AReshootGameState` | Porte les composants et l'état partagés | `GetAudienceRating()`, `GetDirectorVoice()`, `SetPlayerName(FString)`, `GetPlayerName()`, `GetShootStats()` ; `GetMutableShootStats()` (C++, réalisateur seulement) |

Règles :

- Une arme ne connaît pas le réalisateur. Elle inflige des dégâts via `UHealthComponent::ApplyDamage`, rien d'autre.
- Un ennemi ne gère ni score ni directive. Il meurt et notifie.
- `Freeze(true)` doit stopper mouvement, tirs et timers d'un ennemi (utilisé par « CUT! »). `CustomTimeDilation` ne ralentit pas les timers du monde : chaque timer d'ennemi passe par `TrackFreezableTimer`.
- Chaque arme appelle `BroadcastWeaponEvent(Fired)` à chaque tir ou coup (`MeleeSwing` en plus pour un coup d'épée), `Reloaded` en fin de rechargement, `ModeChanged`, `Overheated`, `CooledDown`. Le réalisateur et le HUD écoutent `AReshootCharacter::OnPlayerWeaponEvent`.
- Les restrictions des directives sont portées par le joueur, pas par l'arme : elles survivent au changement d'arme du final. Le joueur filtre lui-même les entrées interdites ; l'arme lit `GetCostMultiplier()` pour sa surchauffe.
- Changer d'arme ou de mode : touches 1 et 2, molette. Le mode 0 est toujours permis.

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
| `UShootDirectorSubsystem` (`UWorldSubsystem`, Game/PIE seulement) | Machine à états de la partie | `StartShoot()`, `GetPhase()`, `GetCurrentUniverse()`, `NotifyKill(const FKillInfo&)` (audimat, bilan, `OnKill` de la directive), `NotifyPlayerDeath()` (pénalité, prise ratée) ; delegates `OnPhaseChanged(EShootPhase)`, `OnUniverseChanged(EUniverse)`, `OnCut()`, `OnAction()`, `OnDirectiveChanged(UDirective*)`, `OnKillScored(Kill, Points, bDirectiveRespected)`, `OnUniverseEventAnnounced(Event)` ; compléments `StartCasting()`, `FinishCasting()`, `GetPlayer()`, `SetShootScript()`, `GetShootScript()`, `GetUniverseDefinition(EUniverse)`, `GetCurrentDirective()`, `GetSceneIndex()`, `GetTakeIndex()`, `CallCut(UDirective*)`, `CallAction()`, `SetUniverse(EUniverse)`, `FreezeAll(bool)` |
| `UShootScript` (`UPrimaryDataAsset`) | Déroulé : scènes, prises, vagues | Données : `Scenes` (`FSceneDef` : `UniverseDefinition`, `IntroLineId`, `Takes`, `PlayerStartTag`, `EventTakeIndex`, `EventDelay`), `FTakeDef` (`Waves`, `Directive`, `ActionLineId`, `CutLineId`), `FWaveDef` (`Count`, `EnemyClassesOverride`, `SpawnInterval`, `StartDelay`), `FinaleUniverses`, `FinaleSwitchInterval`, `FinaleDuration`, `FinaleWaves` ; audimat `BaseKillRating` 10, `DirectiveKillRating` 25, `IgnoredDirectiveKillRating` 5, `TakeClearedBonus` 40, `DeathPenalty` 60 ; temps `CutDelayMin` 20, `CutDelayMax` 40, `CutDuration` 3, `RetakeDelay`, `EventWarningTime` 3 ; `DirectivePool` ; casting `CastingPlayerStartTag`, `CastingIntroLineId` ; répliques `RetakeLineId`, `WrapLineId`, `FinaleIntroLineId` ; `FindUniverseDefinition(EUniverse)` |
| `UDirective` (`UPrimaryDataAsset`, une sous-classe C++ par type) | Une contrainte du réalisateur | `Title`, `VoiceLineId`, `AllowedUniverses` (vide = tous), `IsAllowedIn(EUniverse)` ; virtuelles `OnBegin(UShootDirectorSubsystem*)`, `OnEnd(UShootDirectorSubsystem*)`, `OnKill(Director, Kill)`, `IsKillRespected(const FKillInfo&) -> bool` (vrai = + 25, faux = + 5 ; toujours vrai pour une directive restrictive), sans `Tick` |
| `UAudienceRating` (composant de `AReshootGameState`) | Score | `Add(float)` (négatif accepté, borné à 0), `ResetRating()`, `GetRating()`, `GetRatingPct()`, `GetStars()`, delegate `OnRatingChanged(NewRating, Delta)` ; réglages `MaxRating` 1000, `StarThresholds` {0, 200, 400, 600, 800} |
| `UDirectorVoice` (composant de `AReshootGameState`) | Répliques | `float PlayLine(FName LineId)` : joue le son et pousse le sous-titre, retourne la durée ; delegate `OnSubtitle(LineId, Subtitle, Duration)` ; `LinesTable` = DataTable `DT_DirectorLines` de lignes `FDirectorLine` (`Sound`, `Subtitle`, `SubtitleDuration`) |
| `AWaveSpawner` (un par niveau) | Fait apparaître les ennemis d'une vague | `SpawnWave(const FWaveDef&)`, `KillAll()`, `ReplaceAllForUniverse(EUniverse)`, `FreezeAll(bool)`, `GetAliveCount()`, `IsSpawning()`, delegate `OnWaveCleared` ; `SpawnPoints` (vide = cercle de `SpawnRadius`) |

Déroulé d'une prise :

1. `SceneIntro` : charge le sous-niveau de l'univers, `PlayLine`, clap.
2. `TakeRunning` : vagues actives, directive éventuelle.
3. `Cut` : `OnCut` → tout le monde `Freeze(true)`, réplique, nouvelle directive, puis `OnAction`.
4. Fin de prise : audimat mis à jour, prise suivante ou `SceneTransition`.
5. `Finale` : `SetUniverse` toutes les 20 s, `ReplaceAllForUniverse`.
6. `Wrap` : écran de fin.

Mort du joueur : `NotifyPlayerDeath()` → audimat − 60, réplique « on la refait », la prise recommence.

Comment brancher les directives du GDD (code d'Enzo, dans `Director/`) :

| Directive | Branchement |
| --- | --- |
| Drame | `IsKillRespected` : `Kill.PlayerHealthPct < 0.3` |
| Mouvement | `IsKillRespected` : `Kill.bPlayerMoving` |
| Gros plan | `IsKillRespected` : `Kill.bHeadshot` |
| Cascade | `IsKillRespected` : `Kill.bPlayerSliding \|\| Kill.bPlayerInAir` ; `AllowedUniverses` = Médiéval, Noir |
| Sans filet | `OnBegin` : `Player->SetWeaponActionAllowed(Secondary, false)` ; `OnEnd` : `true` |
| Corps à corps | `OnBegin` : `SetWeaponActionAllowed(SwitchMode, false)` (force l'épée) |
| Plan-séquence | `OnBegin` : `SetWeaponActionAllowed(Reload, false)` ; `OnKill` : `Player->GetCurrentWeapon()->Refill()` |
| Économie | `OnBegin` : `Player->SetWeaponCostMultiplier(2)` ; `OnEnd` : `1` |

Pour les directives qui ont une version « ignorée » à l'usage (et pas seulement au kill), écouter `OnPlayerWeaponEvent`.

## Univers — `Medieval/`, `Space/`, `Noir/`

Classes d'univers déjà déclarées (headers stables, à remplir dans les `.cpp`) :

| Classe | Dossier | Rôle |
| --- | --- | --- |
| `AMedievalSword`, `AMedievalKnight` | `Medieval/` | Point de départ, vides |
| `ASpaceLaser` | `Space/` | Hitscan 12 dégâts, 8 tirs/s, surchauffe en 3 s (÷ `GetCostMultiplier()`), refroidit en 1,5 s, zoom clic droit ; `IsOverheated()`, jauge = chaleur |
| `ASpaceDrone` | `Space/` | Vol en ligne droite, `StopDistance`, tir à `FireRange` 15 m, 8 dégâts, 2 tirs/s ; `SetGravityEnabled(bool)` |
| `ASpaceGravityEvent` | `Space/` | `AUniverseEvent` : gravité rallumée `Duration` 10 s pour le joueur (`SetSpaceGravity`) et les drones ; lumières tag `GravityAlarm` |
| `ANoirRevolver`, `ANoirGangster` | `Noir/` | Point de départ, vides |

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
