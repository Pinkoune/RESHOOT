#include "Director/WaveSpawner.h"
#include "Director/ShootDirectorSubsystem.h"
#include "Core/ReshootEnemy.h"
#include "Core/UniverseDefinition.h"
#include "RESHOOT.h"

#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

AWaveSpawner::AWaveSpawner()
{
	PrimaryActorTick.bCanEverTick = false;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

void AWaveSpawner::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(SpawnTimer);
	Super::EndPlay(EndPlayReason);
}

void AWaveSpawner::SpawnWave(const FWaveDef& Wave)
{
	CurrentWave = Wave;
	PendingSpawns = Wave.Count;
	GetWorldTimerManager().ClearTimer(SpawnTimer);

	if (PendingSpawns <= 0)
	{
		return;
	}
	const float FirstDelay = FMath::Max(Wave.StartDelay, 0.01f);
	GetWorldTimerManager().SetTimer(SpawnTimer, this, &AWaveSpawner::SpawnNext, FMath::Max(Wave.SpawnInterval, 0.01f), true, FirstDelay);
	if (bFrozen)
	{
		GetWorldTimerManager().PauseTimer(SpawnTimer);
	}
}

void AWaveSpawner::SpawnNext()
{
	if (PendingSpawns <= 0)
	{
		GetWorldTimerManager().ClearTimer(SpawnTimer);
		return;
	}

	const UShootDirectorSubsystem* Director = GetWorld()->GetSubsystem<UShootDirectorSubsystem>();
	const EUniverse Universe = Director ? Director->GetCurrentUniverse() : EUniverse::None;

	TSubclassOf<AReshootEnemy> EnemyClass = CurrentWave.EnemyClassesOverride.Num() > 0
		? CurrentWave.EnemyClassesOverride[FMath::RandRange(0, CurrentWave.EnemyClassesOverride.Num() - 1)]
		: PickEnemyClass(Universe);

	--PendingSpawns;
	if (!SpawnEnemy(EnemyClass, PickSpawnTransform()))
	{
		UE_LOG(LogReshoot, Warning, TEXT("%s: could not spawn enemy (class %s)"), *GetName(), *GetNameSafe(EnemyClass));
	}

	if (PendingSpawns <= 0)
	{
		GetWorldTimerManager().ClearTimer(SpawnTimer);
		if (GetAliveCount() == 0)
		{
			OnWaveCleared.Broadcast(this);
		}
	}
}

AReshootEnemy* AWaveSpawner::SpawnEnemy(TSubclassOf<AReshootEnemy> EnemyClass, const FTransform& Transform)
{
	if (!EnemyClass)
	{
		return nullptr;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	AReshootEnemy* Enemy = GetWorld()->SpawnActor<AReshootEnemy>(EnemyClass, Transform, Params);
	if (Enemy)
	{
		Enemy->OnEnemyDied.AddDynamic(this, &AWaveSpawner::HandleEnemyDied);
		AliveEnemies.Add(Enemy);
		if (bFrozen)
		{
			Enemy->Freeze(true);
		}
	}
	return Enemy;
}

TSubclassOf<AReshootEnemy> AWaveSpawner::PickEnemyClass(EUniverse Universe) const
{
	const UShootDirectorSubsystem* Director = GetWorld()->GetSubsystem<UShootDirectorSubsystem>();
	const UUniverseDefinition* Definition = Director ? Director->GetUniverseDefinition(Universe) : nullptr;
	if (!Definition || Definition->EnemyClasses.Num() == 0)
	{
		return nullptr;
	}
	return Definition->EnemyClasses[FMath::RandRange(0, Definition->EnemyClasses.Num() - 1)];
}

FTransform AWaveSpawner::PickSpawnTransform() const
{
	// Règle du GDD : jamais dans le dos du joueur à moins de MinDistanceBehindPlayer.
	const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	const auto IsAllowed = [this, Player](const FVector& Location)
	{
		if (!Player)
		{
			return true;
		}
		const FVector ToSpawn = (Location - Player->GetActorLocation()).GetSafeNormal2D();
		const FVector Facing = Player->GetControlRotation().Vector().GetSafeNormal2D();
		const bool bBehind = FVector::DotProduct(Facing, ToSpawn) < 0.f;
		return !bBehind || FVector::Dist2D(Location, Player->GetActorLocation()) >= MinDistanceBehindPlayer;
	};

	TArray<AActor*> ValidPoints;
	TArray<AActor*> AllowedPoints;
	for (AActor* Point : SpawnPoints)
	{
		if (Point)
		{
			ValidPoints.Add(Point);
			if (IsAllowed(Point->GetActorLocation()))
			{
				AllowedPoints.Add(Point);
			}
		}
	}
	if (AllowedPoints.Num() > 0)
	{
		return AllowedPoints[FMath::RandRange(0, AllowedPoints.Num() - 1)]->GetActorTransform();
	}
	if (ValidPoints.Num() > 0)
	{
		// Tous interdits : le plus éloigné du joueur.
		const FVector PlayerLocation = Player ? Player->GetActorLocation() : GetActorLocation();
		ValidPoints.Sort([&PlayerLocation](const AActor& A, const AActor& B)
		{
			return FVector::DistSquared2D(A.GetActorLocation(), PlayerLocation) > FVector::DistSquared2D(B.GetActorLocation(), PlayerLocation);
		});
		return ValidPoints[0]->GetActorTransform();
	}

	// Sans points : autour du spawner, quelques essais pour respecter la règle.
	FVector Location = GetActorLocation();
	for (int32 Attempt = 0; Attempt < 8; ++Attempt)
	{
		const FVector2D Offset = FMath::RandPointInCircle(SpawnRadius);
		Location = GetActorLocation() + FVector(Offset.X, Offset.Y, 0.f);
		if (IsAllowed(Location))
		{
			break;
		}
	}
	return FTransform(GetActorRotation(), Location);
}

void AWaveSpawner::HandleEnemyDied(AReshootEnemy* Enemy)
{
	AliveEnemies.RemoveAll([Enemy](const TWeakObjectPtr<AReshootEnemy>& Ptr) { return !Ptr.IsValid() || Ptr.Get() == Enemy; });
	if (PendingSpawns <= 0 && GetAliveCount() == 0)
	{
		OnWaveCleared.Broadcast(this);
	}
}

int32 AWaveSpawner::GetAliveCount() const
{
	int32 Count = 0;
	for (const TWeakObjectPtr<AReshootEnemy>& Enemy : AliveEnemies)
	{
		if (Enemy.IsValid() && !Enemy->IsDead())
		{
			++Count;
		}
	}
	return Count;
}

void AWaveSpawner::KillAll()
{
	GetWorldTimerManager().ClearTimer(SpawnTimer);
	PendingSpawns = 0;
	for (const TWeakObjectPtr<AReshootEnemy>& Enemy : AliveEnemies)
	{
		if (Enemy.IsValid())
		{
			Enemy->Destroy();
		}
	}
	AliveEnemies.Reset();
}

void AWaveSpawner::ReplaceAllForUniverse(EUniverse Universe)
{
	TArray<FTransform> Transforms;
	for (const TWeakObjectPtr<AReshootEnemy>& Enemy : AliveEnemies)
	{
		if (Enemy.IsValid() && !Enemy->IsDead())
		{
			Transforms.Add(Enemy->GetActorTransform());
			Enemy->Destroy();
		}
	}
	AliveEnemies.Reset();

	for (const FTransform& Transform : Transforms)
	{
		SpawnEnemy(PickEnemyClass(Universe), Transform);
	}
}

void AWaveSpawner::FreezeAll(bool bFreeze)
{
	bFrozen = bFreeze;
	if (bFreeze)
	{
		GetWorldTimerManager().PauseTimer(SpawnTimer);
	}
	else
	{
		GetWorldTimerManager().UnPauseTimer(SpawnTimer);
	}
	for (const TWeakObjectPtr<AReshootEnemy>& Enemy : AliveEnemies)
	{
		if (Enemy.IsValid())
		{
			Enemy->Freeze(bFreeze);
		}
	}
}
