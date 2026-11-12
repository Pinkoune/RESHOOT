#include "Director/ShootDirectorSubsystem.h"
#include "Director/ShootScript.h"
#include "Director/Directive.h"
#include "Director/WaveSpawner.h"
#include "Director/AudienceRating.h"
#include "Director/DirectorVoice.h"
#include "Core/ReshootCharacter.h"
#include "Core/ReshootEnemy.h"
#include "Core/ReshootGameState.h"
#include "Core/HealthComponent.h"
#include "Core/UniverseDefinition.h"
#include "RESHOOT.h"

#include "EngineUtils.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

// Squelette du jeudi : la séquence complète (sous-niveaux, enchaînement des prises) est la tâche du vendredi.

bool UShootDirectorSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UShootDirectorSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PhaseTimer);
		World->GetTimerManager().ClearTimer(FinaleSwitchTimer);
		World->GetTimerManager().ClearTimer(FinaleEndTimer);
	}
	Super::Deinitialize();
}

// ---------------------------------------------------------------------------
// Accès
// ---------------------------------------------------------------------------

void UShootDirectorSubsystem::SetShootScript(UShootScript* InScript)
{
	Script = InScript;
}

UUniverseDefinition* UShootDirectorSubsystem::GetUniverseDefinition(EUniverse Universe) const
{
	return Script ? Script->FindUniverseDefinition(Universe) : nullptr;
}

UAudienceRating* UShootDirectorSubsystem::GetAudienceRating() const
{
	const AReshootGameState* GameState = GetWorld()->GetGameState<AReshootGameState>();
	return GameState ? GameState->GetAudienceRating() : nullptr;
}

UDirectorVoice* UShootDirectorSubsystem::GetDirectorVoice() const
{
	const AReshootGameState* GameState = GetWorld()->GetGameState<AReshootGameState>();
	return GameState ? GameState->GetDirectorVoice() : nullptr;
}

float UShootDirectorSubsystem::PlayLine(FName LineId) const
{
	UDirectorVoice* Voice = GetDirectorVoice();
	return Voice ? Voice->PlayLine(LineId) : 0.f;
}

AWaveSpawner* UShootDirectorSubsystem::FindSpawner()
{
	if (!Spawner.IsValid())
	{
		for (TActorIterator<AWaveSpawner> It(GetWorld()); It; ++It)
		{
			Spawner = *It;
			Spawner->OnWaveCleared.AddUniqueDynamic(this, &UShootDirectorSubsystem::HandleWaveCleared);
			break;
		}
	}
	return Spawner.Get();
}

// ---------------------------------------------------------------------------
// État
// ---------------------------------------------------------------------------

void UShootDirectorSubsystem::SetPhase(EShootPhase NewPhase)
{
	if (Phase == NewPhase)
	{
		return;
	}
	Phase = NewPhase;
	UE_LOG(LogReshoot, Log, TEXT("Director phase -> %s"), *UEnum::GetValueAsString(NewPhase));
	OnPhaseChanged.Broadcast(NewPhase);
}

void UShootDirectorSubsystem::SetDirective(UDirective* NewDirective)
{
	if (CurrentDirective == NewDirective)
	{
		return;
	}
	if (CurrentDirective)
	{
		CurrentDirective->OnEnd(this);
	}
	CurrentDirective = NewDirective;
	if (CurrentDirective)
	{
		CurrentDirective->OnBegin(this);
		PlayLine(CurrentDirective->VoiceLineId);
	}
	OnDirectiveChanged.Broadcast(CurrentDirective);
}

void UShootDirectorSubsystem::SetUniverse(EUniverse NewUniverse)
{
	CurrentUniverse = NewUniverse;

	if (AReshootCharacter* Player = Cast<AReshootCharacter>(UGameplayStatics::GetPlayerPawn(this, 0)))
	{
		Player->SetUniverse(NewUniverse);
	}
	// TODO vendredi : streaming du sous-niveau (UUniverseDefinition::SubLevelName) et post-process.

	OnUniverseChanged.Broadcast(NewUniverse);
}

void UShootDirectorSubsystem::FreezeAll(bool bFreeze)
{
	if (AReshootCharacter* Player = Cast<AReshootCharacter>(UGameplayStatics::GetPlayerPawn(this, 0)))
	{
		Player->SetFrozen(bFreeze);
		if (UHealthComponent* Health = Player->GetHealth())
		{
			Health->SetInvulnerable(bFreeze);
		}
	}
	// Le spawner suspend ses apparitions et gèlera les figurants qu'il crée pendant le gel.
	if (AWaveSpawner* WaveSpawner = FindSpawner())
	{
		WaveSpawner->FreezeAll(bFreeze);
	}
	// Tous les figurants du monde, y compris ceux posés à la main dans le niveau.
	for (TActorIterator<AReshootEnemy> It(GetWorld()); It; ++It)
	{
		if (!It->IsDead())
		{
			It->Freeze(bFreeze);
		}
	}
}

// ---------------------------------------------------------------------------
// Déroulé
// ---------------------------------------------------------------------------

void UShootDirectorSubsystem::StartShoot()
{
	if (!Script || Script->Scenes.Num() == 0)
	{
		UE_LOG(LogReshoot, Warning, TEXT("StartShoot: no ShootScript (or no scene) assigned on the GameMode."));
		return;
	}
	if (UAudienceRating* Rating = GetAudienceRating())
	{
		Rating->ResetRating();
	}
	BeginScene(0);
}

void UShootDirectorSubsystem::BeginScene(int32 NewSceneIndex)
{
	if (!Script || !Script->Scenes.IsValidIndex(NewSceneIndex))
	{
		BeginFinale();
		return;
	}
	SceneIndex = NewSceneIndex;
	const FSceneDef& Scene = Script->Scenes[SceneIndex];

	SetPhase(EShootPhase::SceneIntro);
	SetUniverse(Scene.UniverseDefinition ? Scene.UniverseDefinition->Universe : EUniverse::None);
	const float Delay = PlayLine(Scene.IntroLineId);

	GetWorld()->GetTimerManager().SetTimer(PhaseTimer, FTimerDelegate::CreateUObject(this, &UShootDirectorSubsystem::BeginTake, 0), FMath::Max(Delay, 0.1f), false);
}

void UShootDirectorSubsystem::BeginTake(int32 NewTakeIndex)
{
	if (!Script || !Script->Scenes.IsValidIndex(SceneIndex))
	{
		return;
	}
	const FSceneDef& Scene = Script->Scenes[SceneIndex];
	if (!Scene.Takes.IsValidIndex(NewTakeIndex))
	{
		SetPhase(EShootPhase::SceneTransition);
		BeginScene(SceneIndex + 1);
		return;
	}

	TakeIndex = NewTakeIndex;
	WaveIndex = INDEX_NONE;
	const FTakeDef& Take = Scene.Takes[TakeIndex];

	SetDirective(Take.Directive);
	PlayLine(Take.ActionLineId);
	SetPhase(EShootPhase::TakeRunning);
	OnAction.Broadcast();
	StartNextWave();
}

void UShootDirectorSubsystem::StartNextWave()
{
	if (!Script || !Script->Scenes.IsValidIndex(SceneIndex) || !Script->Scenes[SceneIndex].Takes.IsValidIndex(TakeIndex))
	{
		return;
	}
	const FTakeDef& Take = Script->Scenes[SceneIndex].Takes[TakeIndex];
	++WaveIndex;
	if (!Take.Waves.IsValidIndex(WaveIndex))
	{
		EndTake();
		return;
	}
	if (AWaveSpawner* WaveSpawner = FindSpawner())
	{
		WaveSpawner->SpawnWave(Take.Waves[WaveIndex]);
	}
	else
	{
		UE_LOG(LogReshoot, Warning, TEXT("No AWaveSpawner in the level."));
	}
}

void UShootDirectorSubsystem::HandleWaveCleared(AWaveSpawner* ClearedSpawner)
{
	if (Phase == EShootPhase::TakeRunning)
	{
		StartNextWave();
	}
}

void UShootDirectorSubsystem::EndTake()
{
	// Fin de prise = « CUT! » puis prise suivante, avec sa directive.
	const FSceneDef* Scene = Script && Script->Scenes.IsValidIndex(SceneIndex) ? &Script->Scenes[SceneIndex] : nullptr;
	if (Scene && Scene->Takes.IsValidIndex(TakeIndex))
	{
		PlayLine(Scene->Takes[TakeIndex].CutLineId);
	}
	SetPhase(EShootPhase::Cut);
	OnCut.Broadcast();
	FreezeAll(true);

	const float CutDuration = Script ? Script->CutDuration : 2.f;
	const int32 NextTake = TakeIndex + 1;
	GetWorld()->GetTimerManager().SetTimer(PhaseTimer, FTimerDelegate::CreateWeakLambda(this, [this, NextTake]()
	{
		FreezeAll(false);
		BeginTake(NextTake);
	}), FMath::Max(CutDuration, 0.1f), false);
}

void UShootDirectorSubsystem::CallCut(UDirective* NewDirective)
{
	SetPhase(EShootPhase::Cut);
	OnCut.Broadcast();
	FreezeAll(true);
	SetDirective(NewDirective);

	const float CutDuration = Script ? Script->CutDuration : 2.f;
	GetWorld()->GetTimerManager().SetTimer(PhaseTimer, this, &UShootDirectorSubsystem::CallAction, FMath::Max(CutDuration, 0.1f), false);
}

void UShootDirectorSubsystem::CallAction()
{
	FreezeAll(false);
	SetPhase(EShootPhase::TakeRunning);
	OnAction.Broadcast();
}

void UShootDirectorSubsystem::NotifyKill(const FKillInfo& Kill)
{
	const float Base = Script ? Script->BaseKillRating : 10.f;
	const float Multiplier = CurrentDirective ? CurrentDirective->EvaluateKill(Kill) : 1.f;
	if (UAudienceRating* Rating = GetAudienceRating())
	{
		Rating->Add(Base * Multiplier);
	}
}

void UShootDirectorSubsystem::NotifyPlayerDeath()
{
	UE_LOG(LogReshoot, Log, TEXT("Player died: retake."));
	const float Delay = FMath::Max(PlayLine(Script ? Script->RetakeLineId : NAME_None), Script ? Script->RetakeDelay : 2.f);
	GetWorld()->GetTimerManager().SetTimer(PhaseTimer, this, &UShootDirectorSubsystem::RestartTake, FMath::Max(Delay, 0.1f), false);
}

void UShootDirectorSubsystem::RestartTake()
{
	if (AWaveSpawner* WaveSpawner = FindSpawner())
	{
		WaveSpawner->KillAll();
	}
	if (AReshootCharacter* Player = Cast<AReshootCharacter>(UGameplayStatics::GetPlayerPawn(this, 0)))
	{
		if (UHealthComponent* Health = Player->GetHealth())
		{
			Health->ResetHealth();
		}
		// TODO vendredi : replacer le joueur au point de départ de la prise.
	}
	if (TakeIndex != INDEX_NONE)
	{
		BeginTake(TakeIndex);
	}
}

void UShootDirectorSubsystem::BeginFinale()
{
	SetPhase(EShootPhase::Finale);
	FinaleUniverseIndex = 0;
	FinaleSwitchUniverse();

	if (Script && Script->FinaleWaves.Num() > 0)
	{
		if (AWaveSpawner* WaveSpawner = FindSpawner())
		{
			WaveSpawner->SpawnWave(Script->FinaleWaves[0]);
		}
	}

	const float Interval = Script ? Script->FinaleSwitchInterval : 20.f;
	const float Duration = Script ? Script->FinaleDuration : 60.f;
	GetWorld()->GetTimerManager().SetTimer(FinaleSwitchTimer, this, &UShootDirectorSubsystem::FinaleSwitchUniverse, Interval, true);
	GetWorld()->GetTimerManager().SetTimer(FinaleEndTimer, this, &UShootDirectorSubsystem::Wrap, FMath::Max(Duration, 0.1f), false);
}

void UShootDirectorSubsystem::FinaleSwitchUniverse()
{
	if (!Script || Script->FinaleUniverses.Num() == 0)
	{
		return;
	}
	const UUniverseDefinition* Definition = Script->FinaleUniverses[FinaleUniverseIndex % Script->FinaleUniverses.Num()];
	++FinaleUniverseIndex;
	if (!Definition)
	{
		return;
	}
	SetUniverse(Definition->Universe);
	if (AWaveSpawner* WaveSpawner = FindSpawner())
	{
		WaveSpawner->ReplaceAllForUniverse(Definition->Universe);
	}
}

void UShootDirectorSubsystem::Wrap()
{
	GetWorld()->GetTimerManager().ClearTimer(FinaleSwitchTimer);
	GetWorld()->GetTimerManager().ClearTimer(PhaseTimer);
	if (AWaveSpawner* WaveSpawner = FindSpawner())
	{
		WaveSpawner->KillAll();
	}
	SetDirective(nullptr);
	PlayLine(Script ? Script->WrapLineId : NAME_None);
	FreezeAll(true);
	SetPhase(EShootPhase::Wrap);
}
