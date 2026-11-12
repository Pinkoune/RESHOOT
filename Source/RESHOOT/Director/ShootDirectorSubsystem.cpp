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
#include "Engine/LevelStreaming.h"
#include "Engine/PostProcessVolume.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/GameModeBase.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "Misc/PackageName.h"
#include "TimerManager.h"

namespace ReshootDirector
{
	/** Tag du volume de post-process géré par le réalisateur (créé à la volée, sans borne). */
	static const FName UniversePostProcessTag(TEXT("ReshootUniversePostProcess"));

	/** Vrai si ce niveau en streaming est le sous-niveau SubLevelName (« L_Noir »), préfixe PIE ignoré. */
	static bool IsSubLevel(const ULevelStreaming* Streaming, FName SubLevelName)
	{
		if (!Streaming || SubLevelName.IsNone())
		{
			return false;
		}
		const FString ShortName = UWorld::RemovePIEPrefix(FPackageName::GetShortName(Streaming->GetWorldAssetPackageName()));
		return ShortName.Equals(SubLevelName.ToString(), ESearchCase::IgnoreCase);
	}

	/** Une réplique au hasard (NAME_None si la liste est vide). */
	static FName PickLine(const TArray<FName>& LineIds)
	{
		return LineIds.Num() > 0 ? LineIds[FMath::RandRange(0, LineIds.Num() - 1)] : NAME_None;
	}

	/** Tous les sous-niveaux d'univers cités par le script (scènes et final). */
	static TSet<FName> CollectUniverseSubLevels(const UShootScript* Script)
	{
		TSet<FName> Names;
		if (!Script)
		{
			return Names;
		}
		for (const FSceneDef& Scene : Script->Scenes)
		{
			if (Scene.UniverseDefinition && !Scene.UniverseDefinition->SubLevelName.IsNone())
			{
				Names.Add(Scene.UniverseDefinition->SubLevelName);
			}
		}
		for (const UUniverseDefinition* Definition : Script->FinaleUniverses)
		{
			if (Definition && !Definition->SubLevelName.IsNone())
			{
				Names.Add(Definition->SubLevelName);
			}
		}
		return Names;
	}
}

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

float UShootDirectorSubsystem::SetDirective(UDirective* NewDirective)
{
	if (CurrentDirective == NewDirective)
	{
		return 0.f;
	}
	if (CurrentDirective)
	{
		CurrentDirective->OnEnd(this);
	}
	CurrentDirective = NewDirective;
	float LineDuration = 0.f;
	if (CurrentDirective)
	{
		CurrentDirective->OnBegin(this);
		LineDuration = PlayLine(CurrentDirective->VoiceLineId);
		UE_LOG(LogReshoot, Log, TEXT("Directive -> %s"), *GetNameSafe(CurrentDirective));
	}
	OnDirectiveChanged.Broadcast(CurrentDirective);
	return LineDuration;
}

void UShootDirectorSubsystem::SetUniverse(EUniverse NewUniverse)
{
	CurrentUniverse = NewUniverse;

	if (AReshootCharacter* Player = Cast<AReshootCharacter>(UGameplayStatics::GetPlayerPawn(this, 0)))
	{
		Player->SetUniverse(NewUniverse);
	}

	UWorld* World = GetWorld();
	const UUniverseDefinition* Definition = GetUniverseDefinition(NewUniverse);
	const FName TargetSubLevel = Definition ? Definition->SubLevelName : NAME_None;

	// Sous-niveaux : on affiche celui de l'univers, on décharge ceux des autres univers du script.
	// Les sous-niveaux doivent être déclarés dans le niveau persistant (fenêtre Levels de L_Stage).
	const TSet<FName> UniverseSubLevels = ReshootDirector::CollectUniverseSubLevels(Script);
	bool bFoundTarget = TargetSubLevel.IsNone();
	for (ULevelStreaming* Streaming : World->GetStreamingLevels())
	{
		if (ReshootDirector::IsSubLevel(Streaming, TargetSubLevel))
		{
			Streaming->SetShouldBeLoaded(true);
			Streaming->SetShouldBeVisible(true);
			bFoundTarget = true;
			continue;
		}
		for (const FName& Other : UniverseSubLevels)
		{
			if (ReshootDirector::IsSubLevel(Streaming, Other))
			{
				Streaming->SetShouldBeLoaded(false);
				Streaming->SetShouldBeVisible(false);
				break;
			}
		}
	}
	if (!bFoundTarget)
	{
		UE_LOG(LogReshoot, Warning, TEXT("SetUniverse: sub-level '%s' is not in the persistent level's streaming levels."), *TargetSubLevel.ToString());
	}
	// Chargement immédiat (petit à-coup, masqué par l'intro ou le CUT!) : les figurants du sous-niveau
	// existent tout de suite, donc FreezeAll les atteint.
	World->FlushLevelStreaming();

	// Post-process de l'univers sur un volume sans borne, créé au premier besoin.
	UMaterialInterface* PostProcess = Definition ? Definition->PostProcessMaterial.Get() : nullptr;
	APostProcessVolume* Volume = nullptr;
	for (TActorIterator<APostProcessVolume> It(World); It; ++It)
	{
		if (It->ActorHasTag(ReshootDirector::UniversePostProcessTag))
		{
			Volume = *It;
			break;
		}
	}
	if (!Volume && PostProcess)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Volume = World->SpawnActor<APostProcessVolume>(APostProcessVolume::StaticClass(), FTransform::Identity, Params);
		if (Volume)
		{
			Volume->Tags.Add(ReshootDirector::UniversePostProcessTag);
			Volume->bUnbound = true;
			Volume->Priority = 10.f;
		}
	}
	if (Volume)
	{
		Volume->Settings.WeightedBlendables.Array.Reset();
		if (PostProcess)
		{
			Volume->Settings.WeightedBlendables.Array.Add(FWeightedBlendable(1.f, PostProcess));
		}
	}

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
	// Une frame plus tard : avec bAutoStartShoot, le BeginPlay du joueur passe après celui du GameMode
	// et remettrait son univers de départ (None) par-dessus celui de la première scène.
	GetWorld()->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		BeginScene(0);
	}));
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
	// Personne ne bouge pendant l'annonce du réalisateur ; BeginTake dégèle au « Action! ».
	FreezeAll(true);
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

	// Chaque prise (ou reprise après une mort) démarre sans directive : elle arrive au « CUT! » en pleine prise.
	SetDirective(nullptr);
	StartTakeAction();
}

void UShootDirectorSubsystem::StartTakeAction()
{
	if (!Script || !Script->Scenes.IsValidIndex(SceneIndex) || !Script->Scenes[SceneIndex].Takes.IsValidIndex(TakeIndex))
	{
		return;
	}
	const FTakeDef& Take = Script->Scenes[SceneIndex].Takes[TakeIndex];

	FreezeAll(false);
	PlayLine(Take.ActionLineId);
	SetPhase(EShootPhase::TakeRunning);
	OnAction.Broadcast();
	ScheduleMidTakeCut();
	StartNextWave();
}

// ---------------------------------------------------------------------------
// « CUT! » en pleine prise : gel, commentaire, directive, « Action! »
// ---------------------------------------------------------------------------

void UShootDirectorSubsystem::ScheduleMidTakeCut()
{
	// PhaseTimer est libre pendant la prise : il sera remplacé si la prise se termine ou si le joueur meurt avant.
	const float MinDelay = Script ? Script->MidTakeCutMinDelay : 20.f;
	const float MaxDelay = Script ? FMath::Max(Script->MidTakeCutMaxDelay, MinDelay) : 40.f;
	const float Delay = FMath::Max(FMath::FRandRange(MinDelay, MaxDelay), 0.1f);
	GetWorld()->GetTimerManager().SetTimer(PhaseTimer, this, &UShootDirectorSubsystem::MidTakeCut, Delay, false);
}

void UShootDirectorSubsystem::MidTakeCut()
{
	if (Phase != EShootPhase::TakeRunning || !Script
		|| !Script->Scenes.IsValidIndex(SceneIndex) || !Script->Scenes[SceneIndex].Takes.IsValidIndex(TakeIndex))
	{
		return;
	}

	// Directive imposée par la prise, sinon tirée au hasard (en évitant de répéter la précédente).
	PendingDirective = Script->Scenes[SceneIndex].Takes[TakeIndex].Directive;
	if (!PendingDirective && Script->RandomDirectives.Num() > 0)
	{
		TArray<UDirective*> Choices;
		for (UDirective* Candidate : Script->RandomDirectives)
		{
			if (Candidate && Candidate != LastDirective)
			{
				Choices.Add(Candidate);
			}
		}
		if (Choices.Num() == 0 && LastDirective)
		{
			Choices.Add(LastDirective);
		}
		PendingDirective = Choices.Num() > 0 ? Choices[FMath::RandRange(0, Choices.Num() - 1)] : nullptr;
	}

	SetPhase(EShootPhase::Cut);
	OnCut.Broadcast();
	FreezeAll(true);

	const TArray<FName>& Comments = Script->MidTakeCutLineIds;
	const float CommentDuration = Comments.Num() > 0 ? PlayLine(Comments[FMath::RandRange(0, Comments.Num() - 1)]) : 0.f;
	GetWorld()->GetTimerManager().SetTimer(PhaseTimer, this, &UShootDirectorSubsystem::AnnounceMidTakeDirective, FMath::Max(CommentDuration, 0.1f), false);
}

void UShootDirectorSubsystem::AnnounceMidTakeDirective()
{
	if (Phase != EShootPhase::Cut)
	{
		return;
	}
	if (PendingDirective)
	{
		LastDirective = PendingDirective;
	}
	const float AnnounceDuration = SetDirective(PendingDirective);
	PendingDirective = nullptr;

	// Sans réplique (ou sans directive), le gel dure au moins CutDuration.
	const float Hold = AnnounceDuration > 0.f ? AnnounceDuration : (Script ? Script->CutDuration : 2.f);
	GetWorld()->GetTimerManager().SetTimer(PhaseTimer, this, &UShootDirectorSubsystem::CallAction, FMath::Max(Hold, 0.1f), false);
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
	else if (Phase == EShootPhase::Finale && Script && Script->FinaleWaves.Num() > 0)
	{
		// Le final ne s'arrête qu'au bout de FinaleDuration : ses vagues tournent en boucle.
		WaveIndex = (WaveIndex + 1) % Script->FinaleWaves.Num();
		ClearedSpawner->SpawnWave(Script->FinaleWaves[WaveIndex]);
	}
}

void UShootDirectorSubsystem::EndTake()
{
	// Fin de prise : bonus (une mort relance la prise, donc on arrive ici sans être mort), commentaire, prise suivante.
	if (UAudienceRating* Rating = GetAudienceRating())
	{
		Rating->Add(Script ? Script->TakeClearedRating : 40.f);
	}
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
	if (UAudienceRating* Rating = GetAudienceRating())
	{
		Rating->Add(-(Script ? Script->PlayerDeathPenalty : 60.f));
	}
	const float Delay = FMath::Max(PlayLine(Script ? ReshootDirector::PickLine(Script->RetakeLineIds) : NAME_None), Script ? Script->RetakeDelay : 2.f);
	GetWorld()->GetTimerManager().SetTimer(PhaseTimer, this, &UShootDirectorSubsystem::RestartTake, FMath::Max(Delay, 0.1f), false);
}

void UShootDirectorSubsystem::RestartTake()
{
	// Pendant le final, pas de prise à rejouer : le joueur repart du départ et le final continue.
	const bool bInFinale = Phase == EShootPhase::Finale;
	if (AWaveSpawner* WaveSpawner = bInFinale ? nullptr : FindSpawner())
	{
		WaveSpawner->KillAll();
	}
	if (AReshootCharacter* Player = Cast<AReshootCharacter>(UGameplayStatics::GetPlayerPawn(this, 0)))
	{
		if (UHealthComponent* Health = Player->GetHealth())
		{
			Health->ResetHealth();
		}

		// Retour au départ : PlayerStart dont le Player Start Tag est le nom de l'univers (« Noir »,
		// « Medieval », « Space »), sinon le PlayerStart où le joueur est apparu.
		AController* Controller = Player->GetController();
		AGameModeBase* GameMode = GetWorld()->GetAuthGameMode();
		const FString UniverseTag = StaticEnum<EUniverse>()->GetNameStringByValue(static_cast<int64>(CurrentUniverse));
		if (AActor* Start = GameMode && Controller ? GameMode->FindPlayerStart(Controller, UniverseTag) : nullptr)
		{
			const FRotator StartYaw(0.f, Start->GetActorRotation().Yaw, 0.f);
			Player->GetCharacterMovement()->StopMovementImmediately();
			Player->SetActorLocationAndRotation(Start->GetActorLocation(), StartYaw, false, nullptr, ETeleportType::TeleportPhysics);
			Controller->SetControlRotation(StartYaw);
		}
	}
	// La prise repart du clap : BeginTake retire la directive, un nouveau « CUT! » en imposera une.
	if (!bInFinale && TakeIndex != INDEX_NONE)
	{
		BeginTake(TakeIndex);
	}
}

void UShootDirectorSubsystem::BeginFinale()
{
	SetPhase(EShootPhase::Finale);
	SetDirective(nullptr);
	// On peut arriver ici depuis une intro de scène (gelée) si la scène n'a aucune prise.
	FreezeAll(false);
	FinaleUniverseIndex = 0;
	FinaleSwitchUniverse();
	PlayLine(Script ? Script->FinaleIntroLineId : NAME_None);

	if (Script && Script->FinaleWaves.Num() > 0)
	{
		if (AWaveSpawner* WaveSpawner = FindSpawner())
		{
			WaveIndex = 0;
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
	const bool bFirstSwitch = FinaleUniverseIndex == 0;
	++FinaleUniverseIndex;
	if (!Definition)
	{
		return;
	}
	SetUniverse(Definition->Universe);
	// La première bascule a lieu à l'entrée du final, annoncée par FinaleIntroLineId.
	if (!bFirstSwitch)
	{
		PlayLine(ReshootDirector::PickLine(Script->FinaleSwitchLineIds));
	}
	if (AWaveSpawner* WaveSpawner = FindSpawner())
	{
		WaveSpawner->ReplaceAllForUniverse(Definition->Universe);
	}
}

void UShootDirectorSubsystem::Wrap()
{
	GetWorld()->GetTimerManager().ClearTimer(FinaleSwitchTimer);
	GetWorld()->GetTimerManager().ClearTimer(PhaseTimer);
	// Phase d'abord : KillAll vide la vague, et une vague vidée pendant le final en relancerait une.
	SetPhase(EShootPhase::Wrap);
	if (AWaveSpawner* WaveSpawner = FindSpawner())
	{
		WaveSpawner->KillAll();
	}
	SetDirective(nullptr);
	if (Script)
	{
		const UAudienceRating* Rating = GetAudienceRating();
		const bool bBadShoot = Rating && Rating->GetStars() <= Script->WrapBadMaxStars;
		PlayLine(bBadShoot ? Script->WrapBadLineId : Script->WrapLineId);
	}
	FreezeAll(true);
}
