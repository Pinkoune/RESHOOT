#include "Core/SetDressingSweep.h"
#include "Core/ReshootCharacter.h"
#include "Core/ReshootEnemy.h"
#include "RESHOOT.h"

#include "Components/ExponentialHeightFogComponent.h"
#include "Components/LightComponentBase.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/VolumetricCloudComponent.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Materials/MaterialParameterCollection.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "TimerManager.h"

namespace
{
	constexpr float SweepStepInterval = 1.f / 60.f;
	const FName ParamOrigin(TEXT("SweepOrigin"));
	const FName ParamDirection(TEXT("SweepDirection"));
	const FName ParamDistance(TEXT("SweepDistance"));
	const FName ParamEdge(TEXT("EdgeWidth"));

	const FName SwitchTag(TEXT("DressingSwitch"));
	constexpr float FarDistance = 1000000.f; // Au-delà de tout le décor, ciel compris.

	/** Lumières, ciel, brouillard, nuages (et tout acteur tagué DressingSwitch) : bascule d'un coup au passage du rideau. */
	bool IsSwitchActor(const AActor* Actor)
	{
		return Actor->ActorHasTag(SwitchTag)
			|| Actor->FindComponentByClass<ULightComponentBase>()
			|| Actor->FindComponentByClass<USkyAtmosphereComponent>()
			|| Actor->FindComponentByClass<UExponentialHeightFogComponent>()
			|| Actor->FindComponentByClass<UVolumetricCloudComponent>();
	}
}

ASetDressingSweep::ASetDressingSweep()
{
	PrimaryActorTick.bCanEverTick = false;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

void ASetDressingSweep::BeginPlay()
{
	Super::BeginPlay();
	ResetDressing();
}

void ASetDressingSweep::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(SweepTimer);
	Super::EndPlay(EndPlayReason);
}

// ---------------------------------------------------------------------------
// Accès statiques
// ---------------------------------------------------------------------------

ASetDressingSweep* ASetDressingSweep::Get(const UObject* WorldContextObject)
{
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	if (!World)
	{
		return nullptr;
	}
	for (TActorIterator<ASetDressingSweep> It(const_cast<UWorld*>(World)); It; ++It)
	{
		return *It;
	}
	return nullptr;
}

bool ASetDressingSweep::IsLocationRealInWorld(const UObject* WorldContextObject, const FVector& Location)
{
	const ASetDressingSweep* Sweep = Get(WorldContextObject);
	return Sweep && Sweep->IsLocationReal(Location);
}

// ---------------------------------------------------------------------------
// Géométrie du balayage
// ---------------------------------------------------------------------------

float ASetDressingSweep::ProjectLocation(const FVector& Location) const
{
	return FVector::DotProduct(Location - GetActorLocation(), Direction);
}

float ASetDressingSweep::GetStartDistance() const
{
	return -SweepLength * 0.5f - EdgeWidth;
}

float ASetDressingSweep::GetEndDistance() const
{
	return SweepLength * 0.5f + EdgeWidth;
}

bool ASetDressingSweep::IsLocationReal(const FVector& Location) const
{
	if (!bReal)
	{
		return false;
	}
	return !bSweeping || ProjectLocation(Location) < CurrentDistance;
}

// ---------------------------------------------------------------------------
// API
// ---------------------------------------------------------------------------

void ASetDressingSweep::ResetDressing()
{
	GetWorldTimerManager().ClearTimer(SweepTimer);
	Direction = SweepDirection.GetSafeNormal(KINDA_SMALL_NUMBER, FVector::ForwardVector);
	bReal = false;
	bSweeping = false;
	SetSparksActive(false);

	GatherDressingActors();
	for (const TWeakObjectPtr<AActor>& Actor : PropActors)
	{
		SetActorDressingVisible(Actor.Get(), true);
	}
	for (const TWeakObjectPtr<AActor>& Actor : RealActors)
	{
		SetActorDressingVisible(Actor.Get(), false);
	}
	for (const TWeakObjectPtr<APostProcessVolume>& Volume : PropVolumes)
	{
		if (Volume.IsValid()) { Volume->BlendWeight = 1.f; }
	}
	for (const TWeakObjectPtr<APostProcessVolume>& Volume : RealVolumes)
	{
		if (Volume.IsValid()) { Volume->BlendWeight = 0.f; }
	}

	ApplyDistance(-FarDistance);
	UpdateCharacters(true);
	OnDressingReset.Broadcast();
}

void ASetDressingSweep::UpgradeDressing()
{
	if (bReal)
	{
		return;
	}
	Direction = SweepDirection.GetSafeNormal(KINDA_SMALL_NUMBER, FVector::ForwardVector);
	GatherDressingActors();
	bReal = true;
	bSweeping = true;
	SweepStartTime = GetWorld()->GetTimeSeconds();

	// Les objets réalistes masqués par le matériau deviennent visibles : le rideau les révèle.
	for (const TWeakObjectPtr<AActor>& Actor : RealActors)
	{
		if (Actor.IsValid() && !IsSwitchActor(Actor.Get()))
		{
			SetActorDressingVisible(Actor.Get(), true);
		}
	}

	if (SweepSound)
	{
		UGameplayStatics::PlaySound2D(this, SweepSound);
	}
	SetSparksActive(true);
	ApplyDistance(GetStartDistance());
	UE_LOG(LogReshoot, Log, TEXT("Set dressing: upgrade sweep (%.1fs, %d prop / %d real actors)"), Duration, PropActors.Num(), RealActors.Num());

	GetWorldTimerManager().SetTimer(SweepTimer, this, &ASetDressingSweep::StepSweep, SweepStepInterval, true);
}

// ---------------------------------------------------------------------------
// Balayage
// ---------------------------------------------------------------------------

void ASetDressingSweep::StepSweep()
{
	const float Alpha = FMath::Clamp((GetWorld()->GetTimeSeconds() - SweepStartTime) / Duration, 0.f, 1.f);
	// Légère accélération puis décélération : le rideau « démarre » et « freine ».
	const float Eased = FMath::InterpEaseInOut(0.f, 1.f, Alpha, 1.6f);
	ApplyDistance(FMath::Lerp(GetStartDistance(), GetEndDistance(), Eased));

	// Lumières, ciel, brouillard : basculent au passage du rideau.
	for (const TWeakObjectPtr<AActor>& Actor : PropActors)
	{
		if (Actor.IsValid() && IsSwitchActor(Actor.Get()) && ProjectLocation(Actor->GetActorLocation()) < CurrentDistance)
		{
			SetActorDressingVisible(Actor.Get(), false);
		}
	}
	for (const TWeakObjectPtr<AActor>& Actor : RealActors)
	{
		if (Actor.IsValid() && IsSwitchActor(Actor.Get()) && ProjectLocation(Actor->GetActorLocation()) < CurrentDistance)
		{
			SetActorDressingVisible(Actor.Get(), true);
		}
	}
	// Post-process : fondu sur toute la durée.
	for (const TWeakObjectPtr<APostProcessVolume>& Volume : PropVolumes)
	{
		if (Volume.IsValid()) { Volume->BlendWeight = 1.f - Eased; }
	}
	for (const TWeakObjectPtr<APostProcessVolume>& Volume : RealVolumes)
	{
		if (Volume.IsValid()) { Volume->BlendWeight = Eased; }
	}

	UpdateCharacters(false);

	if (Alpha >= 1.f)
	{
		FinishSweep();
	}
}

void ASetDressingSweep::FinishSweep()
{
	GetWorldTimerManager().ClearTimer(SweepTimer);
	bSweeping = false;
	SetSparksActive(false);
	ApplyDistance(FarDistance);

	// Plus rien de carton à l'écran : on cache tout, rendu compris.
	for (const TWeakObjectPtr<AActor>& Actor : PropActors)
	{
		SetActorDressingVisible(Actor.Get(), false);
	}
	for (const TWeakObjectPtr<AActor>& Actor : RealActors)
	{
		SetActorDressingVisible(Actor.Get(), true);
	}
	UpdateCharacters(true);
	OnDressingUpgraded.Broadcast();
}

void ASetDressingSweep::ApplyDistance(float Distance)
{
	CurrentDistance = Distance;

	if (Collection)
	{
		const FVector Origin = GetActorLocation();
		UKismetMaterialLibrary::SetVectorParameterValue(this, Collection, ParamOrigin, FLinearColor(Origin.X, Origin.Y, Origin.Z, 0.f));
		UKismetMaterialLibrary::SetVectorParameterValue(this, Collection, ParamDirection, FLinearColor(Direction.X, Direction.Y, Direction.Z, 0.f));
		UKismetMaterialLibrary::SetScalarParameterValue(this, Collection, ParamDistance, Distance);
		UKismetMaterialLibrary::SetScalarParameterValue(this, Collection, ParamEdge, EdgeWidth);
	}

	// Étincelles réparties sur la largeur du plateau, sur la ligne du rideau.
	if (SparkComponents.Num() > 0)
	{
		const FVector Side = FVector::CrossProduct(FVector::UpVector, Direction).GetSafeNormal();
		const FVector LineCenter = GetActorLocation() + Direction * Distance;
		const int32 PerRow = FMath::Max(SparkCount, 1);
		for (int32 Index = 0; Index < SparkComponents.Num(); ++Index)
		{
			if (UNiagaraComponent* Spark = SparkComponents[Index])
			{
				const int32 Column = Index % PerRow;
				const int32 Row = Index / PerRow;
				const float T = PerRow > 1 ? static_cast<float>(Column) / (PerRow - 1) - 0.5f : 0.f;
				const float Height = SparkHeights.IsValidIndex(Row) ? SparkHeights[Row] : 0.f;
				Spark->SetWorldLocation(LineCenter + Side * (T * SweepWidth) + FVector::UpVector * Height);
			}
		}
	}
}

void ASetDressingSweep::SetSparksActive(bool bActive)
{
	if (bActive && EdgeSparks && SparkComponents.Num() == 0)
	{
		const int32 Rows = FMath::Max(SparkHeights.Num(), 1);
		for (int32 Index = 0; Index < SparkCount * Rows; ++Index)
		{
			UNiagaraComponent* Spark = UNiagaraFunctionLibrary::SpawnSystemAttached(EdgeSparks, GetRootComponent(), NAME_None,
				FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::KeepRelativeOffset, false, false);
			if (Spark)
			{
				Spark->SetUsingAbsoluteLocation(true);
				SparkComponents.Add(Spark);
			}
		}
	}
	for (UNiagaraComponent* Spark : SparkComponents)
	{
		if (!Spark)
		{
			continue;
		}
		if (bActive)
		{
			Spark->Activate(true);
		}
		else
		{
			Spark->Deactivate();
		}
	}
}

// ---------------------------------------------------------------------------
// Acteurs tagués, joueur, figurants
// ---------------------------------------------------------------------------

void ASetDressingSweep::GatherDressingActors()
{
	PropActors.Reset();
	RealActors.Reset();
	PropVolumes.Reset();
	RealVolumes.Reset();
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		AActor* Actor = *It;
		const bool bProp = Actor->ActorHasTag(PropTag);
		const bool bRealTagged = Actor->ActorHasTag(RealTag);
		if (!bProp && !bRealTagged)
		{
			continue;
		}
		if (APostProcessVolume* Volume = Cast<APostProcessVolume>(Actor))
		{
			(bProp ? PropVolumes : RealVolumes).Add(Volume);
			continue;
		}
		(bProp ? PropActors : RealActors).Add(Actor);
	}
}

void ASetDressingSweep::SetActorDressingVisible(AActor* Actor, bool bVisible)
{
	if (!Actor)
	{
		return;
	}
	Actor->SetActorHiddenInGame(!bVisible);
	TInlineComponentArray<ULightComponentBase*> Lights(Actor);
	for (ULightComponentBase* Light : Lights)
	{
		Light->SetVisibility(bVisible);
		if (USkyLightComponent* Sky = Cast<USkyLightComponent>(Light))
		{
			if (bVisible)
			{
				Sky->RecaptureSky();
			}
		}
	}
	TInlineComponentArray<USceneComponent*> Others(Actor);
	for (USceneComponent* Component : Others)
	{
		if (Component->IsA<USkyAtmosphereComponent>() || Component->IsA<UExponentialHeightFogComponent>() || Component->IsA<UVolumetricCloudComponent>())
		{
			Component->SetVisibility(bVisible);
		}
	}
}

void ASetDressingSweep::UpdateCharacters(bool bForceAll)
{
	if (AReshootCharacter* Player = Cast<AReshootCharacter>(UGameplayStatics::GetPlayerPawn(this, 0)))
	{
		const bool bPlayerReal = bForceAll ? bReal : IsLocationReal(Player->GetActorLocation());
		if (Player->IsDressingReal() != bPlayerReal)
		{
			Player->SetDressingReal(bPlayerReal);
		}
	}
	for (TActorIterator<AReshootEnemy> It(GetWorld()); It; ++It)
	{
		if (It->IsDead())
		{
			continue;
		}
		const bool bEnemyReal = bForceAll ? bReal : IsLocationReal(It->GetActorLocation());
		if (It->IsDressingReal() != bEnemyReal)
		{
			It->SetDressingReal(bEnemyReal);
		}
	}
}
