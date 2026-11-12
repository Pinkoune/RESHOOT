# Passation du réalisateur — pour Enzo

À partir du commit `1834f72`, `Source/RESHOOT/Director/` est à toi. Le socle (`Core/`) ne bouge plus sans demande dans `docs/TASKS.md`, section « Demandes au socle ».

À lire avant : `AGENTS.md` (règles), `docs/ARCHITECTURE.md` (API), le GDD (`docs/RESHOOT — Game Design Document.md`, section « Le réalisateur »).

## Ce qui existe

| Fichier | État |
| --- | --- |
| `ShootDirectorSubsystem` | Machine à états qui marche de bout en bout en version simple : scène → prises → vagues → « CUT! » en fin de prise → prise suivante → scène suivante → final → Wrap. |
| `ShootScript` | Data Asset complet : scènes, prises, vagues, final, toutes les valeurs du GDD (audimat 10 / 25 / 5 / + 40 / − 60, CUT entre 20 et 40 s, durée du gel, annonce d'événement 3 s avant, réserve de directives, casting). |
| `Directive` | Classe de base : `OnBegin`, `OnEnd`, `OnKill`, `IsKillRespected`, `AllowedUniverses`. **Aucune directive concrète n'existe.** |
| `AudienceRating` | Fini : `Add` (négatif accepté), étoiles {0, 200, 400, 600, 800}. |
| `DirectorVoice` | `PlayLine` joue le son et pousse le sous-titre ; lit `DT_DirectorLines` (pas encore créée, c'est Jules qui fournit les lignes). |
| `WaveSpawner` | Fini : apparition par vague, en cercle ou sur `SpawnPoints`, gel, `KillAll`, `ReplaceAllForUniverse`, `OnWaveCleared`. |

## Ce qui a été testé en jeu (PIE, carte `Content/RESHOOT/Core/Maps/L_SocleTest`)

- Kill → audimat + 10 (avant le second lot ; la formule a changé depuis, voir plus bas).
- Mort du joueur → réplique de reprise → la prise recommence, vie pleine.
- Vague de 3 → `OnWaveCleared` → vague suivante.
- `CallCut` / `CallAction` : tout le monde gèle et dégèle, y compris les figurants posés à la main.

Pas encore testé après le second lot de headers : `NotifyKill` avec directive, la pénalité de mort, le casting.

## Ce qui a changé au second lot (déjà codé dans tes `.cpp`)

- `UDirective::EvaluateKill` (multiplicateur) est remplacé par `IsKillRespected` (booléen). `NotifyKill` donne `BaseKillRating` sans directive, `DirectiveKillRating` si respectée, `IgnoredDirectiveKillRating` sinon, appelle `Directive->OnKill`, met à jour `FShootStats` dans le GameState et diffuse `OnKillScored` (pour le « + 25 » du HUD).
- `NotifyPlayerDeath` retire `DeathPenalty` et compte une prise ratée.
- `StartCasting` / `FinishCasting` / `GetPlayer` existent en version minimale.

## Ce qui reste (par ordre de priorité)

1. **Vendredi — « CUT! » au milieu de la prise.** Aujourd'hui le CUT tombe à la fin de la prise. Le GDD veut : clap → combat → CUT entre `CutDelayMin` et `CutDelayMax` s → directive (celle de `FTakeDef::Directive`, sinon tirée dans `DirectivePool` filtrée par `IsAllowedIn`) → ACTION → fin de prise quand la dernière vague est vidée. `CutTimer` est déjà déclaré. Pense à suspendre `CutTimer` et `EventTimer` pendant le gel.
2. **Vendredi — replacer le joueur.** Au début de chaque prise et à la reprise : chercher le `APlayerStart` dont le tag vaut `FSceneDef::PlayerStartTag`, puis `Player->ResetForTake(Transform)` (téléporte, vie pleine, arme rechargée). Ajouter `TakeClearedBonus` si `bPlayerDiedThisTake` est faux en fin de prise, et `++CompletedTakes`.
3. **Vendredi — sous-niveaux.** Dans `SetUniverse` : décharger le sous-niveau précédent, charger `UUniverseDefinition::SubLevelName` (`UGameplayStatics::LoadStreamLevel`), appliquer `PostProcessMaterial`, lancer `Music`. Les sous-niveaux sont `L_Medieval`, `L_Space`, `L_Noir` dans `L_Stage` (Jérémy).
4. **Samedi — les 8 directives.** Une sous-classe C++ par directive, dans `Director/`. Le tableau de branchement est dans `docs/ARCHITECTURE.md`. Les restrictions passent par `AReshootCharacter::SetWeaponActionAllowed` et `SetWeaponCostMultiplier` ; pense à `ClearWeaponRestrictions` dans `OnEnd` et à la reprise. Nouvelles classes = nouveaux headers = un redémarrage : crée les 8 d'un coup.
5. **Samedi — événement d'univers.** Pendant la prise `EventTakeIndex`, `EventDelay` s après ACTION : `OnUniverseEventAnnounced`, réplique `EventLineId`, puis `StartEvent()` `EventWarningTime` s plus tard. Gel au CUT : `ActiveEvent->SetEventPaused(true)`. L'événement spatial (`ASpaceGravityEvent`) est fait par Argon.
6. **Samedi — final.** Squelette présent (bascule toutes les `FinaleSwitchInterval` s, `ReplaceAllForUniverse`). Reste : réplique `FinaleIntroLineId`, enchaîner les `FinaleWaves`, pas de CUT pendant le final.
7. **Vendredi — voix sans chevauchement.** Le GDD dit qu'une réplique ne coupe jamais une autre : la nouvelle attend, ou elle est abandonnée si elle n'est plus d'actualité. Aujourd'hui, `UDirectorVoice::PlayLine` joue tout de suite (`PlaySound2D`), donc deux répliques se chevauchent.
8. **Vendredi — entrées des figurants.** Le GDD veut qu'ils entrent par les portes, jamais dans le dos du joueur à moins de 10 m. Aujourd'hui, `AWaveSpawner` tire un point au hasard dans `SpawnPoints` ou sur un cercle, sans regarder où est le joueur.
9. **Samedi — casting.** Placer le joueur au `PlayerStart` tagué `CastingPlayerStartTag`. L'UI appelle `FinishCasting()` quand le tutoriel est fini et `AReshootGameState::SetPlayerName`.

## Pièges connus

- Les timers du monde ignorent `CustomTimeDilation` : un ennemi gelé n'arrête ses timers que s'ils passent par `TrackFreezableTimer`. Côté réalisateur, mets toi-même tes timers en pause au CUT.
- `UShootDirectorSubsystem` n'existe qu'en Game/PIE, pas dans l'éditeur.
- Ne pas faire de `Tick` : timers et delegates.
- Tout nouveau `UPROPERTY` ou `UFUNCTION` dans un `.h` = redémarrage. Regroupe.

## Pour tester

`L_SocleTest` a un `AReshootGameMode` en override, un `WaveSpawner` et un chevalier de test. Mets un `UShootScript` sur le GameMode et coche `bAutoStartShoot`. Commande console `DebugUniverse 1|2|3` pour changer d'univers à la main.
