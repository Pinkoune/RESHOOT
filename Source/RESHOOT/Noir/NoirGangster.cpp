#include "Noir/NoirGangster.h"
#include "Core/HealthComponent.h"
#include "Core/ReshootCharacter.h"
#include "Noir/NoirRevolver.h"
#include "RESHOOT.h"

#include "AIController.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Navigation/PathFollowingComponent.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	/** Forme simple attachée à la capsule. Bloque seulement le canal Visibility si bShootable. */
	UStaticMeshComponent* MakePart(ANoirGangster* Owner, USceneComponent* Parent, FName Name, UStaticMesh* Mesh,
		const FVector& Location, const FVector& Scale, bool bShootable)
	{
		UStaticMeshComponent* Part = Owner->CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Part->SetupAttachment(Parent);
		Part->SetStaticMesh(Mesh);
		Part->SetRelativeLocation(Location);
		Part->SetRelativeScale3D(Scale);
		Part->SetCanEverAffectNavigation(false);
		if (bShootable)
		{
			Part->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			Part->SetCollisionResponseToAllChannels(ECR_Ignore);
			Part->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
		}
		else
		{
			Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
		return Part;
	}

	/** Distance horizontale au-delà de laquelle le gangster n'est plus « à son couvert ». */
	constexpr float AtCoverTolerance = 120.f;
}

ANoirGangster::ANoirGangster(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Universe = EUniverse::Noir;
	HealthComponent->MaxHealth = 80.f;

	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->MaxWalkSpeed = 450.f;
	Move->bOrientRotationToMovement = true;
	bUseControllerRotationYaw = false;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));

	// Capsule de 176 cm (demi-hauteur 88). Formes de base : 100 cm de diamètre et de haut, pivot au centre.
	USceneComponent* Root = GetCapsuleComponent();
	Body = MakePart(this, Root, TEXT("Body"), CylinderMesh.Object, FVector(0.f, 0.f, -20.f), FVector(0.6f, 0.6f, 1.36f), true);
	Head = MakePart(this, Root, TEXT("Head"), SphereMesh.Object, FVector(0.f, 0.f, 66.f), FVector(0.4f), true);
	Head->ComponentTags.Add(TEXT("head"));
	HatBrim = MakePart(this, Root, TEXT("HatBrim"), CylinderMesh.Object, FVector(0.f, 0.f, 82.f), FVector(0.7f, 0.7f, 0.04f), false);
	HatCrown = MakePart(this, Root, TEXT("HatCrown"), CylinderMesh.Object, FVector(0.f, 0.f, 92.f), FVector(0.38f, 0.38f, 0.2f), false);
}

void ANoirGangster::BeginPlay()
{
	Super::BeginPlay();

	// Rejoint le couvert le plus proche (celui derrière lequel il a été posé, ou un autre s'il vient d'apparaître).
	MoveToCover(false);
	if (CurrentCover.IsValid() && FVector::Dist2D(GetActorLocation(), GetCoverSpot(CurrentCover.Get())) < AtCoverTolerance)
	{
		SetDucked(true);
	}

	const float FirstDelay = BurstInterval * 0.5f + FMath::FRandRange(0.f, FirstShotMaxDelay);
	GetWorldTimerManager().SetTimer(CycleTimer, this, &ANoirGangster::RunCycle, BurstInterval, true, FirstDelay);

	// Spawné pendant un CUT! : les timers démarrent en pause.
	if (IsFrozen())
	{
		GetWorldTimerManager().PauseTimer(CycleTimer);
	}
}

void ANoirGangster::OnFreezeChanged(bool bFreeze)
{
	Super::OnFreezeChanged(bFreeze);

	FTimerManager& Timers = GetWorldTimerManager();
	for (FTimerHandle* Handle : { &CycleTimer, &ShotTimer, &DuckTimer })
	{
		if (bFreeze)
		{
			Timers.PauseTimer(*Handle);
		}
		else
		{
			Timers.UnPauseTimer(*Handle);
		}
	}
}

void ANoirGangster::OnDeath(AActor* Killer, bool bHeadshot, bool bMelee)
{
	FTimerManager& Timers = GetWorldTimerManager();
	Timers.ClearTimer(CycleTimer);
	Timers.ClearTimer(ShotTimer);
	Timers.ClearTimer(DuckTimer);
	CurrentCover.Reset();

	// Le corps ne doit plus arrêter les balles pendant qu'il disparaît.
	Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Head->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Super::OnDeath(Killer, bHeadshot, bMelee);
}

void ANoirGangster::RunCycle()
{
	if (IsFrozen() || IsDead() || ShotsLeft > 0)
	{
		return;
	}

	const AReshootCharacter* Player = GetTargetPlayer();
	const UHealthComponent* PlayerHealth = Player ? Player->GetHealth() : nullptr;
	if (!PlayerHealth || PlayerHealth->IsDead())
	{
		return;
	}

	// En train de courir vers un couvert : on attend d'arriver.
	if (GetVelocity().SizeSquared2D() > FMath::Square(50.f))
	{
		return;
	}

	const float PlayerDistance = FVector::Dist(GetActorLocation(), Player->GetActorLocation());

	// Joueur trop proche : on change de couvert.
	if (PlayerDistance < FleeDistance && MoveToCover(true))
	{
		return;
	}

	// Le joueur a contourné le couvert : on fait le tour pour rester caché. Sans couvert : on en cherche un.
	if (CurrentCover.IsValid())
	{
		if (FVector::Dist2D(GetActorLocation(), GetCoverSpot(CurrentCover.Get())) > AtCoverTolerance && MoveToCover(false))
		{
			return;
		}
	}
	else if (MoveToCover(false))
	{
		return;
	}

	if (PlayerDistance > Range)
	{
		return;
	}

	// Se relève et tire une rafale.
	SetDucked(false);
	ShotsLeft = BurstCount;
	GetWorldTimerManager().SetTimer(ShotTimer, this, &ANoirGangster::FireOneShot, ShotSpacing, true, FMath::Max(PeekDelay, 0.01f));
}

void ANoirGangster::FireOneShot()
{
	if (IsFrozen() || IsDead())
	{
		return;
	}

	--ShotsLeft;
	if (ShotsLeft <= 0)
	{
		ShotsLeft = 0;
		GetWorldTimerManager().ClearTimer(ShotTimer);
		GetWorldTimerManager().SetTimer(DuckTimer, this, &ANoirGangster::FinishBurst, FMath::Max(StayExposedTime, 0.01f), false);
	}

	AReshootCharacter* Player = GetTargetPlayer();
	UHealthComponent* PlayerHealth = Player ? Player->GetHealth() : nullptr;
	if (!PlayerHealth || PlayerHealth->IsDead() || !CanSeePlayer())
	{
		return;
	}

	const FVector Start = GetPawnViewLocation();
	const FVector Target = Player->GetActorLocation();

	// Se tourne vers le joueur (lacet seulement).
	FRotator Facing = (Target - GetActorLocation()).Rotation();
	Facing.Pitch = 0.f;
	Facing.Roll = 0.f;
	SetActorRotation(Facing);

	ANoirRevolver::SpawnMuzzleFlash(this, Start + Facing.Vector() * 50.f, MuzzleFlashIntensity, 0.06f);
	if (FireSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, FireSound, Start);
	}

	const float Chance = Player->IsMoving() ? MovingHitChance : HitChance;
	const bool bHits = FMath::FRand() < Chance;

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

void ANoirGangster::FinishBurst()
{
	// Ne se baisse que s'il est bien derrière son couvert (sinon il s'enfoncerait dans le sol pour rien).
	if (CurrentCover.IsValid() && FVector::Dist2D(GetActorLocation(), GetCoverSpot(CurrentCover.Get())) < AtCoverTolerance)
	{
		SetDucked(true);
	}
}

void ANoirGangster::SetDucked(bool bDuck)
{
	if (bDucked == bDuck)
	{
		return;
	}
	bDucked = bDuck;

	const FVector Offset(0.f, 0.f, bDuck ? -DuckDepth : DuckDepth);
	for (UStaticMeshComponent* Part : { Body.Get(), Head.Get(), HatBrim.Get(), HatCrown.Get() })
	{
		if (Part)
		{
			Part->AddRelativeLocation(Offset);
		}
	}
}

FVector ANoirGangster::GetCoverSpot(const AActor* Cover) const
{
	const FVector CoverLocation = Cover->GetActorLocation();
	const AReshootCharacter* Player = GetTargetPlayer();
	FVector Away = Player ? (CoverLocation - Player->GetActorLocation()) : (GetActorLocation() - CoverLocation);
	Away.Z = 0.f;
	if (!Away.Normalize())
	{
		Away = FVector::ForwardVector;
	}
	FVector Spot = CoverLocation + Away * CoverOffset;
	Spot.Z = GetActorLocation().Z;
	return Spot;
}

bool ANoirGangster::MoveToCover(bool bFlee)
{
	const FVector MyLocation = GetActorLocation();
	const AReshootCharacter* Player = GetTargetPlayer();
	const FVector PlayerLocation = Player ? Player->GetActorLocation() : MyLocation;

	TArray<AActor*> Covers;
	UGameplayStatics::GetAllActorsWithTag(this, CoverTag, Covers);

	AActor* Best = nullptr;
	float BestScore = TNumericLimits<float>::Max();
	for (AActor* Cover : Covers)
	{
		if (!Cover || (bFlee && Cover == CurrentCover.Get()))
		{
			continue;
		}
		const float DistToMe = FVector::Dist2D(MyLocation, Cover->GetActorLocation());
		const float DistToPlayer = FVector::Dist2D(PlayerLocation, Cover->GetActorLocation());
		if (DistToMe > CoverSearchRadius || (bFlee && DistToPlayer < FleeDistance + CoverOffset))
		{
			continue;
		}

		// Un couvert par gangster.
		bool bTaken = false;
		for (TActorIterator<ANoirGangster> It(GetWorld()); It; ++It)
		{
			if (*It != this && !It->IsDead() && It->GetCover() == Cover)
			{
				bTaken = true;
				break;
			}
		}
		if (bTaken)
		{
			continue;
		}

		// Fuite : le plus proche de moi parmi ceux assez loin du joueur. Sinon : le plus proche de moi.
		const float Score = DistToMe;
		if (Score < BestScore)
		{
			BestScore = Score;
			Best = Cover;
		}
	}

	if (!Best)
	{
		return false;
	}

	CurrentCover = Best;
	const FVector Spot = GetCoverSpot(Best);
	if (FVector::Dist2D(MyLocation, Spot) < 30.f)
	{
		return false;
	}

	AAIController* AI = Cast<AAIController>(GetController());
	if (!AI)
	{
		return false;
	}

	EPathFollowingRequestResult::Type Result = AI->MoveToLocation(Spot, 20.f, false, true, false, true, nullptr, true);
	if (Result == EPathFollowingRequestResult::Failed)
	{
		// Pas de navmesh : ligne droite.
		Result = AI->MoveToLocation(Spot, 20.f, false, false, false, true, nullptr, true);
	}
	if (Result != EPathFollowingRequestResult::RequestSuccessful)
	{
		return false;
	}

	SetDucked(false);
	return true;
}

bool ANoirGangster::CanSeePlayer() const
{
	const AReshootCharacter* Player = GetTargetPlayer();
	if (!Player)
	{
		return false;
	}
	const FVector Start = GetPawnViewLocation();
	const FVector Target = Player->GetActorLocation();
	if (FVector::DistSquared(Start, Target) > FMath::Square(Range))
	{
		return false;
	}

	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(NoirGangsterSight), true, this);
	const bool bBlocked = GetWorld()->LineTraceSingleByChannel(Hit, Start, Target, ECC_Visibility, Params) && Hit.GetActor() != Player;
	return !bBlocked;
}
