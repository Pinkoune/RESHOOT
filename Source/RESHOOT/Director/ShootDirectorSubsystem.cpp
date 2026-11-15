#include "Director/ShootDirectorSubsystem.h"
#include "Director/ShootScript.h"
#include "Director/Directive.h"
#include "Director/WaveSpawner.h"
#include "Director/AudienceRating.h"
#include "Director/DirectorVoice.h"
#include "Director/CastingTarget.h"
#include "Core/ReshootCharacter.h"
#include "Core/ReshootEnemy.h"
#include "Core/ReshootGameState.h"
#include "Core/HealthComponent.h"
#include "Core/UniverseDefinition.h"
#include "Core/UniverseEvent.h"
#include "Core/SetDressingSweep.h"
#include "RESHOOT.h"

#include "EngineUtils.h"
#include "Engine/World.h"
#include "Engine/LevelStreaming.h"
#include "Engine/PostProcessVolume.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "HAL/IConsoleManager.h"
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

	/** Charge (et affiche) ou décharge un sous-niveau déclaré dans le niveau persistant. Vrai s'il existe. */
	static bool SetSubLevelLoaded(UWorld* World, FName SubLevelName, bool bLoaded)
	{
		bool bFound = false;
		for (ULevelStreaming* Streaming : World->GetStreamingLevels())
		{
			if (IsSubLevel(Streaming, SubLevelName))
			{
				Streaming->SetShouldBeLoaded(bLoaded);
				Streaming->SetShouldBeVisible(bLoaded);
				bFound = true;
			}
		}
		return bFound;
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
		World->GetTimerManager().ClearTimer(CutTimer);
		World->GetTimerManager().ClearTimer(EventTimer);
		World->GetTimerManager().ClearTimer(CastingTimer);
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

AReshootCharacter* UShootDirectorSubsystem::GetPlayer() const
{
	return Cast<AReshootCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
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
	// L'événement d'univers et son compte à rebours se figent aussi.
	FTimerManager& Timers = GetWorld()->GetTimerManager();
	if (bFreeze && Timers.IsTimerActive(EventTimer))
	{
		Timers.PauseTimer(EventTimer);
	}
	else if (!bFreeze && Timers.IsTimerPaused(EventTimer))
	{
		Timers.UnPauseTimer(EventTimer);
	}
	if (ActiveEvent && ActiveEvent->IsEventActive())
	{
		ActiveEvent->SetEventPaused(bFreeze);
	}
}

// ---------------------------------------------------------------------------
// Déroulé
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Casting : fond vert, une glissade et trois cibles, puis la scène 1
// ---------------------------------------------------------------------------

void UShootDirectorSubsystem::StartCasting()
{
	// Une frame plus tard, comme StartShoot : le BeginPlay du joueur passe après celui du GameMode.
	GetWorld()->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		SetPhase(EShootPhase::Casting);
		// Le studio de casting (et son départ, ses cibles) est dans son propre sous-niveau.
		if (Script && !Script->CastingSubLevelName.IsNone())
		{
			if (ReshootDirector::SetSubLevelLoaded(GetWorld(), Script->CastingSubLevelName, true))
			{
				GetWorld()->FlushLevelStreaming();
			}
		}
		bCastingSlid = false;
		bCastingDone = false;
		CastingTargetsHit = 0;
		CastingTargetsTotal = 0;
		for (TActorIterator<ACastingTarget> It(GetWorld()); It; ++It)
		{
			It->ResetTarget();
			++CastingTargetsTotal;
		}

		// Arme de l'univers de casting, sans charger son sous-niveau ni son post-process.
		if (AReshootCharacter* Player = GetPlayer())
		{
			Player->SetUniverse(Script ? Script->CastingUniverse : EUniverse::Noir);
		}
		ResetPlayerToStart(Script ? Script->CastingPlayerStartTag : FName(TEXT("Casting")));
		PlayLine(Script ? Script->CastingIntroLineId : NAME_None);

		GetWorld()->GetTimerManager().SetTimer(CastingTimer, this, &UShootDirectorSubsystem::CheckCastingSlide, 0.1f, true);
		BroadcastCastingProgress();
		UE_LOG(LogReshoot, Log, TEXT("Casting started: %d target(s)."), CastingTargetsTotal);
	}));
}

void UShootDirectorSubsystem::CheckCastingSlide()
{
	const AReshootCharacter* Player = GetPlayer();
	if (Phase == EShootPhase::Casting && !bCastingSlid && Player && Player->IsSliding())
	{
		bCastingSlid = true;
		BroadcastCastingProgress();
		TryFinishCasting();
	}
}

void UShootDirectorSubsystem::NotifyCastingTargetHit(ACastingTarget* Target)
{
	if (Phase != EShootPhase::Casting || bCastingDone)
	{
		return;
	}
	++CastingTargetsHit;
	BroadcastCastingProgress();
	TryFinishCasting();
}

void UShootDirectorSubsystem::BroadcastCastingProgress()
{
	OnCastingProgress.Broadcast(bCastingSlid, CastingTargetsHit, CastingTargetsTotal);
}

void UShootDirectorSubsystem::TryFinishCasting()
{
	if (bCastingDone || !bCastingSlid || CastingTargetsHit < CastingTargetsTotal)
	{
		return;
	}
	bCastingDone = true;
	GetWorld()->GetTimerManager().ClearTimer(CastingTimer);
	UE_LOG(LogReshoot, Log, TEXT("Casting done."));
	const float LineDuration = PlayLine(Script ? Script->CastingDoneLineId : NAME_None);
	const float Delay = FMath::Max(LineDuration, Script ? Script->CastingOutroDelay : 2.f);
	GetWorld()->GetTimerManager().SetTimer(PhaseTimer, this, &UShootDirectorSubsystem::FinishCasting, FMath::Max(Delay, 0.1f), false);
}

void UShootDirectorSubsystem::FinishCasting()
{
	if (Phase == EShootPhase::Casting)
	{
		GetWorld()->GetTimerManager().ClearTimer(CastingTimer);
		StartShoot();
	}
}

// ---------------------------------------------------------------------------
// Départ du joueur
// ---------------------------------------------------------------------------

void UShootDirectorSubsystem::ResetPlayerToStart(FName PreferredTag)
{
	AReshootCharacter* Player = GetPlayer();
	if (!Player)
	{
		return;
	}
	// Ordre : tag demandé, tag de la scène, nom de l'univers (« Noir »...), tag du plateau, tout départ hors casting.
	TArray<FName> Tags;
	Tags.Add(PreferredTag);
	if (Phase != EShootPhase::Casting && Phase != EShootPhase::Finale && Script && Script->Scenes.IsValidIndex(SceneIndex))
	{
		Tags.Add(Script->Scenes[SceneIndex].PlayerStartTag);
	}
	Tags.Add(FName(*StaticEnum<EUniverse>()->GetNameStringByValue(static_cast<int64>(CurrentUniverse))));
	Tags.Add(Script ? Script->StagePlayerStartTag : FName(TEXT("Stage")));
	const FName CastingTag = Script ? Script->CastingPlayerStartTag : FName(TEXT("Casting"));

	const APlayerStart* Start = nullptr;
	for (const FName& Tag : Tags)
	{
		if (Tag.IsNone())
		{
			continue;
		}
		for (TActorIterator<APlayerStart> It(GetWorld()); It && !Start; ++It)
		{
			if (It->PlayerStartTag == Tag)
			{
				Start = *It;
			}
		}
		if (Start)
		{
			break;
		}
	}
	for (TActorIterator<APlayerStart> It(GetWorld()); It && !Start; ++It)
	{
		if (It->PlayerStartTag != CastingTag)
		{
			Start = *It;
		}
	}
	const FTransform StartTransform = Start
		? FTransform(FRotator(0.f, Start->GetActorRotation().Yaw, 0.f), Start->GetActorLocation())
		: Player->GetActorTransform();
	// Téléporte, vie pleine, arme rechargée (socle).
	Player->ResetForTake(StartTransform);
	Player->ClearWeaponRestrictions();
}

// ---------------------------------------------------------------------------
// Événement d'univers : annoncé EventDelay s après « Action! » de la prise EventTakeIndex, déclenché 3 s plus tard
// ---------------------------------------------------------------------------

void UShootDirectorSubsystem::ScheduleUniverseEvent()
{
	if (!Script || !Script->Scenes.IsValidIndex(SceneIndex))
	{
		return;
	}
	const FSceneDef& Scene = Script->Scenes[SceneIndex];
	if (Scene.EventTakeIndex != TakeIndex || !Scene.UniverseDefinition || !Scene.UniverseDefinition->EventClass)
	{
		return;
	}
	GetWorld()->GetTimerManager().SetTimer(EventTimer, this, &UShootDirectorSubsystem::AnnounceUniverseEvent, FMath::Max(Scene.EventDelay, 0.1f), false);
}

void UShootDirectorSubsystem::AnnounceUniverseEvent()
{
	const UUniverseDefinition* Definition = GetUniverseDefinition(CurrentUniverse);
	if (!Definition || !Definition->EventClass || Phase != EShootPhase::TakeRunning)
	{
		return;
	}
	// L'événement posé dans le sous-niveau (convoi et ses rails, gravité...), sinon on le fait apparaître.
	ActiveEvent = nullptr;
	for (TActorIterator<AUniverseEvent> It(GetWorld()); It; ++It)
	{
		if (It->IsA(Definition->EventClass))
		{
			ActiveEvent = *It;
			break;
		}
	}
	if (!ActiveEvent)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		ActiveEvent = GetWorld()->SpawnActor<AUniverseEvent>(Definition->EventClass, FTransform::Identity, Params);
	}
	if (!ActiveEvent)
	{
		return;
	}
	UE_LOG(LogReshoot, Log, TEXT("Universe event announced: %s"), *GetNameSafe(ActiveEvent));
	OnUniverseEventAnnounced.Broadcast(ActiveEvent);
	PlayLine(Definition->EventLineId);
	GetWorld()->GetTimerManager().SetTimer(EventTimer, this, &UShootDirectorSubsystem::TriggerUniverseEvent, FMath::Max(Script ? Script->EventWarningTime : 3.f, 0.1f), false);
}

void UShootDirectorSubsystem::TriggerUniverseEvent()
{
	if (ActiveEvent && !ActiveEvent->IsEventActive())
	{
		UE_LOG(LogReshoot, Log, TEXT("Universe event started: %s"), *GetNameSafe(ActiveEvent));
		ActiveEvent->StartEvent();
	}
}

void UShootDirectorSubsystem::StopUniverseEvent()
{
	GetWorld()->GetTimerManager().ClearTimer(EventTimer);
	if (ActiveEvent && ActiveEvent->IsEventActive())
	{
		ActiveEvent->SetEventPaused(false);
		ActiveEvent->StopEvent();
	}
	ActiveEvent = nullptr;
}

// ---------------------------------------------------------------------------
// Répétition, puis mise en scène : décor carton à la 1re prise, rideau de lumière, décor réaliste
// ---------------------------------------------------------------------------

void UShootDirectorSubsystem::TakeControlOfStaging()
{
	// Le rideau (ASetDressingSweep, socle) suit le déroulé tout seul tant que ce réglage vaut 1 ;
	// le réalisateur l'appelle lui-même, au bon moment (fin de la 1re prise, pas au début de la 2e).
	if (IConsoleVariable* AutoFollow = IConsoleManager::Get().FindConsoleVariable(TEXT("reshoot.Dressing.AutoFromDirector")))
	{
		AutoFollow->Set(0, ECVF_SetByCode);
	}
}

void UShootDirectorSubsystem::SetStagingState(bool bReal)
{
	bStagingReal = bReal;
	ASetDressingSweep* Sweep = ASetDressingSweep::Get(this);
	if (!Sweep)
	{
		return;
	}
	if (bReal)
	{
		Sweep->UpgradeDressing();	// sans effet s'il est déjà réaliste
	}
	else if (Sweep->IsDressingReal() || Sweep->IsSweeping())
	{
		Sweep->ResetDressing();
	}
}

float UShootDirectorSubsystem::ImproveStaging()
{
	UE_LOG(LogReshoot, Log, TEXT("Staging improved: cardboard -> real."));
	PlayLine(Script ? Script->StagingLineId : NAME_None);
	OnStagingImproved.Broadcast(CurrentUniverse);
	SetStagingState(true);
	const ASetDressingSweep* Sweep = ASetDressingSweep::Get(this);
	if (!Sweep)
	{
		UE_LOG(LogReshoot, Warning, TEXT("ImproveStaging: no ASetDressingSweep in the level, nothing to sweep."));
	}
	return Sweep ? Sweep->Duration : 0.f;
}

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

	StopUniverseEvent();
	SetPhase(EShootPhase::SceneIntro);
	SetUniverse(Scene.UniverseDefinition ? Scene.UniverseDefinition->Universe : EUniverse::None);
	// Chaque scène commence en répétition, dans le décor en carton.
	TakeControlOfStaging();
	SetStagingState(false);
	ResetPlayerToStart(Scene.PlayerStartTag);
	// Le joueur a quitté le studio de casting : on le décharge.
	if (!Script->CastingSubLevelName.IsNone())
	{
		ReshootDirector::SetSubLevelLoaded(GetWorld(), Script->CastingSubLevelName, false);
	}
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
	bMidTakeCutDone = false;
	StopUniverseEvent();

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
	ScheduleUniverseEvent();
	StartNextWave();
}

// ---------------------------------------------------------------------------
// « CUT! » en pleine prise : gel, commentaire, directive, « Action! »
// ---------------------------------------------------------------------------

void UShootDirectorSubsystem::ScheduleMidTakeCut()
{
	// PhaseTimer est libre pendant la prise : il sera remplacé si la prise se termine ou si le joueur meurt avant.
	const float MinDelay = Script ? Script->CutDelayMin : 20.f;
	const float MaxDelay = Script ? FMath::Max(Script->CutDelayMax, MinDelay) : 40.f;
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
	if (!PendingDirective && Script->DirectivePool.Num() > 0)
	{
		TArray<UDirective*> Choices;
		for (UDirective* Candidate : Script->DirectivePool)
		{
			if (Candidate && Candidate != LastDirective && Candidate->IsAllowedIn(CurrentUniverse))
			{
				Choices.Add(Candidate);
			}
		}
		if (Choices.Num() == 0 && LastDirective && LastDirective->IsAllowedIn(CurrentUniverse))
		{
			Choices.Add(LastDirective);
		}
		PendingDirective = Choices.Num() > 0 ? Choices[FMath::RandRange(0, Choices.Num() - 1)] : nullptr;
	}

	bMidTakeCutDone = true;
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
	GetWorld()->GetTimerManager().SetTimer(PhaseTimer, this, &UShootDirectorSubsystem::ResumeAfterMidTakeCut, FMath::Max(Hold, 0.1f), false);
}

void UShootDirectorSubsystem::ResumeAfterMidTakeCut()
{
	CallAction();
	// Si le CUT! est tombé sur un plateau vide (dernière vague éliminée avant lui), la vague suivante repart.
	AWaveSpawner* WaveSpawner = FindSpawner();
	if (WaveSpawner && WaveSpawner->GetAliveCount() == 0 && !WaveSpawner->IsSpawning())
	{
		StartNextWave();
	}
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
		// GDD : une prise est toujours interrompue une fois. Si le joueur a tout éliminé avant le CUT!, le CUT!
		// tombe maintenant, puis la dernière vague est rejouée sous la directive.
		if (!bMidTakeCutDone && Take.Waves.Num() > 0)
		{
			WaveIndex = Take.Waves.Num() - 2;
			MidTakeCut();
			return;
		}
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
		Rating->Add(Script ? Script->TakeClearedBonus : 40.f);
	}
	if (AReshootGameState* GameState = GetWorld()->GetGameState<AReshootGameState>())
	{
		++GameState->GetMutableShootStats().CompletedTakes;
	}
	StopUniverseEvent();
	const FSceneDef* Scene = Script && Script->Scenes.IsValidIndex(SceneIndex) ? &Script->Scenes[SceneIndex] : nullptr;
	if (Scene && Scene->Takes.IsValidIndex(TakeIndex))
	{
		PlayLine(Scene->Takes[TakeIndex].CutLineId);
	}
	SetPhase(EShootPhase::Cut);
	OnCut.Broadcast();
	FreezeAll(true);

	float CutDuration = Script ? Script->CutDuration : 2.f;
	// Fin de la répétition (1re prise d'une scène qui en a d'autres) : « On améliore la mise en scène ! »
	if (Scene && TakeIndex == 0 && Scene->Takes.Num() > 1 && !bStagingReal)
	{
		// Le gel dure le temps du balayage, plus une demi-seconde pour admirer le décor.
		const float SweepDuration = ImproveStaging();
		CutDuration = FMath::Max(CutDuration, SweepDuration + 0.5f);
	}
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
	bool bRespected = false;
	float Points = Script ? Script->BaseKillRating : 10.f;
	if (CurrentDirective)
	{
		bRespected = CurrentDirective->IsKillRespected(Kill);
		Points = bRespected
			? (Script ? Script->DirectiveKillRating : 25.f)
			: (Script ? Script->IgnoredDirectiveKillRating : 5.f);
		CurrentDirective->OnKill(this, Kill);
	}
	if (UAudienceRating* Rating = GetAudienceRating())
	{
		Rating->Add(Points);
	}
	if (AReshootGameState* GameState = GetWorld()->GetGameState<AReshootGameState>())
	{
		FShootStats& Stats = GameState->GetMutableShootStats();
		++Stats.Kills;
		Stats.DirectiveKills += bRespected ? 1 : 0;
		Stats.Headshots += Kill.bHeadshot ? 1 : 0;
	}
	OnKillScored.Broadcast(Kill, Points, bRespected);
}

void UShootDirectorSubsystem::NotifyPlayerDeath()
{
	UE_LOG(LogReshoot, Log, TEXT("Player died: retake."));
	bPlayerDiedThisTake = true;
	if (UAudienceRating* Rating = GetAudienceRating())
	{
		Rating->Add(-(Script ? Script->DeathPenalty : 60.f));
	}
	if (AReshootGameState* GameState = GetWorld()->GetGameState<AReshootGameState>())
	{
		++GameState->GetMutableShootStats().FailedTakes;
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
	if (!bInFinale)
	{
		StopUniverseEvent();
	}
	// Retour au départ de la scène (tag de scène, d'univers ou du plateau), vie et arme pleines.
	ResetPlayerToStart(NAME_None);
	bPlayerDiedThisTake = false;
	// La prise repart du clap : BeginTake retire la directive, un nouveau « CUT! » en imposera une.
	if (!bInFinale && TakeIndex != INDEX_NONE)
	{
		BeginTake(TakeIndex);
	}
}

void UShootDirectorSubsystem::BeginFinale()
{
	StopUniverseEvent();
	SetPhase(EShootPhase::Finale);
	SetDirective(nullptr);
	// Le final se joue dans le « vrai » film : décors réalistes.
	TakeControlOfStaging();
	SetStagingState(true);
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
	StopUniverseEvent();
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
