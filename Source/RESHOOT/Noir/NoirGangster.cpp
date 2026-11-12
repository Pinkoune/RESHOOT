#include "Noir/NoirGangster.h"
#include "Core/HealthComponent.h"
#include "Core/ReshootCharacter.h"
#include "RESHOOT.h"

#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

ANoirGangster::ANoirGangster(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Universe = EUniverse::Noir;
}

void ANoirGangster::BeginPlay()
{
	Super::BeginPlay();

	const float FirstDelay = FireInterval + FMath::FRandRange(0.f, FirstShotMaxDelay);
	GetWorldTimerManager().SetTimer(FireTimer, this, &ANoirGangster::Fire, FireInterval, true, FirstDelay);

	// Spawné pendant un CUT! : le timer démarre en pause.
	if (IsFrozen())
	{
		GetWorldTimerManager().PauseTimer(FireTimer);
	}
}

void ANoirGangster::OnFreezeChanged(bool bFreeze)
{
	Super::OnFreezeChanged(bFreeze);

	if (bFreeze)
	{
		GetWorldTimerManager().PauseTimer(FireTimer);
	}
	else
	{
		GetWorldTimerManager().UnPauseTimer(FireTimer);
	}
}

void ANoirGangster::OnDeath(AActor* Killer, bool bHeadshot, bool bMelee)
{
	GetWorldTimerManager().ClearTimer(FireTimer);
	Super::OnDeath(Killer, bHeadshot, bMelee);
}

void ANoirGangster::Fire()
{
	if (IsFrozen() || IsDead())
	{
		return;
	}

	AReshootCharacter* Player = GetTargetPlayer();
	UHealthComponent* PlayerHealth = Player ? Player->GetHealth() : nullptr;
	if (!PlayerHealth || PlayerHealth->IsDead())
	{
		return;
	}

	const FVector Start = GetPawnViewLocation();
	const FVector Target = Player->GetActorLocation();
	if (FVector::DistSquared(Start, Target) > FMath::Square(Range))
	{
		return;
	}

	// Ligne de vue : rien entre le gangster et le joueur.
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(NoirGangsterSight), true, this);
	const bool bBlocked = GetWorld()->LineTraceSingleByChannel(Hit, Start, Target, ECC_Visibility, Params) && Hit.GetActor() != Player;
	if (bBlocked)
	{
		return;
	}

	// Se tourne vers le joueur (lacet seulement).
	FRotator Facing = (Target - GetActorLocation()).Rotation();
	Facing.Pitch = 0.f;
	Facing.Roll = 0.f;
	SetActorRotation(Facing);

	if (FireSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, FireSound, Start);
	}

	const bool bHits = FMath::FRand() < HitChance;

	if (bDrawShotTrace)
	{
		const FVector Miss = Target + FMath::VRand() * 150.f;
		DrawDebugLine(GetWorld(), Start, bHits ? Target : Miss, FColor::Red, false, 0.15f, 0, 1.f);
	}

	if (bHits)
	{
		PlayerHealth->ApplyDamage(Damage, this);
	}
}
