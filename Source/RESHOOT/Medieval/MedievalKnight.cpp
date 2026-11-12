#include "Medieval/MedievalKnight.h"
#include "Core/HealthComponent.h"
#include "Core/ReshootCharacter.h"
#include "RESHOOT.h"

#include "AIController.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Navigation/PathFollowingComponent.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

AMedievalKnight::AMedievalKnight(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Universe = EUniverse::Medieval;

	bUseControllerRotationYaw = false;
	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->bOrientRotationToMovement = true;
	Move->RotationRate = FRotator(0.f, 720.f, 0.f);
	Move->MaxWalkSpeed = RunSpeed;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));

	// Armure en boîte grise : un bloc pour le corps, un cube pour le heaume.
	BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
	BodyMesh->SetupAttachment(GetCapsuleComponent());
	BodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BodyMesh->SetRelativeLocation(FVector(0.f, 0.f, -15.f));
	BodyMesh->SetRelativeScale3D(FVector(0.55f, 0.65f, 1.4f));

	HelmetMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HelmetMesh"));
	HelmetMesh->SetupAttachment(GetCapsuleComponent());
	HelmetMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HelmetMesh->SetRelativeLocation(FVector(0.f, 0.f, 72.f));
	HelmetMesh->SetRelativeScale3D(FVector(0.38f, 0.38f, 0.32f));

	if (CubeMesh.Succeeded())
	{
		BodyMesh->SetStaticMesh(CubeMesh.Object);
		HelmetMesh->SetStaticMesh(CubeMesh.Object);
	}
}

void AMedievalKnight::BeginPlay()
{
	Super::BeginPlay();

	GetCharacterMovement()->MaxWalkSpeed = RunSpeed;

	// Petit décalage aléatoire pour que les chevaliers d'une vague ne réfléchissent pas tous à la même image.
	GetWorldTimerManager().SetTimer(ThinkTimer, this, &AMedievalKnight::Think, ThinkInterval, true, FMath::FRandRange(0.f, ThinkInterval));

	// Spawné pendant un CUT! : tout démarre en pause.
	if (IsFrozen())
	{
		GetWorldTimerManager().PauseTimer(ThinkTimer);
	}
}

void AMedievalKnight::OnFreezeChanged(bool bFreeze)
{
	Super::OnFreezeChanged(bFreeze);

	FTimerManager& Timers = GetWorldTimerManager();
	if (bFreeze)
	{
		Timers.PauseTimer(ThinkTimer);
		Timers.PauseTimer(AttackTimer);
	}
	else
	{
		Timers.UnPauseTimer(ThinkTimer);
		Timers.UnPauseTimer(AttackTimer);
	}
}

float AMedievalKnight::GetGapToPlayer() const
{
	const AReshootCharacter* Player = GetTargetPlayer();
	if (!Player)
	{
		return TNumericLimits<float>::Max();
	}
	const float CenterDistance = FVector::Dist2D(GetActorLocation(), Player->GetActorLocation());
	const float Radii = GetCapsuleComponent()->GetScaledCapsuleRadius() + Player->GetCapsuleComponent()->GetScaledCapsuleRadius();
	// Ignore les écarts de hauteur raisonnables, mais pas un joueur perché très haut.
	const float HeightGap = FMath::Abs(GetActorLocation().Z - Player->GetActorLocation().Z);
	if (HeightGap > GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * 2.f)
	{
		return TNumericLimits<float>::Max();
	}
	return FMath::Max(0.f, CenterDistance - Radii);
}

void AMedievalKnight::FacePlayer()
{
	if (const AReshootCharacter* Player = GetTargetPlayer())
	{
		FRotator Facing = (Player->GetActorLocation() - GetActorLocation()).Rotation();
		Facing.Pitch = 0.f;
		Facing.Roll = 0.f;
		SetActorRotation(Facing);
	}
}

void AMedievalKnight::Think()
{
	if (IsFrozen() || IsDead() || bAttacking)
	{
		return;
	}

	AAIController* AI = Cast<AAIController>(GetController());
	AReshootCharacter* Player = GetTargetPlayer();
	const UHealthComponent* PlayerHealth = Player ? Player->GetHealth() : nullptr;
	if (!AI || !PlayerHealth || PlayerHealth->IsDead())
	{
		if (AI)
		{
			AI->StopMovement();
		}
		return;
	}

	const double Now = GetWorld()->GetTimeSeconds();

	// Au contact : préparation du coup.
	if (GetGapToPlayer() <= AttackRange)
	{
		if (Now - LastAttackTime >= AttackInterval)
		{
			bAttacking = true;
			LastAttackTime = Now;
			AI->StopMovement();
			FacePlayer();
			if (bDrawDebug)
			{
				DrawDebugSphere(GetWorld(), GetActorLocation() + GetActorForwardVector() * 60.f, 25.f, 8, FColor::Orange, false, AttackWindup);
			}
			GetWorldTimerManager().SetTimer(AttackTimer, this, &AMedievalKnight::FinishAttack, FMath::Max(AttackWindup, 0.01f), false);
		}
		return;
	}

	// Poursuite : une requête suffit, le suivi de chemin suit le joueur tant qu'il est la cible.
	if (AI->GetMoveStatus() == EPathFollowingStatus::Moving)
	{
		return;
	}

	const float Acceptance = AttackRange * 0.5f;
	EPathFollowingRequestResult::Type Result = EPathFollowingRequestResult::Failed;
	if (bUsePathfinding)
	{
		Result = AI->MoveToActor(Player, Acceptance, true, true, false, nullptr, true);
	}
	if (Result == EPathFollowingRequestResult::Failed)
	{
		// Pas de navmesh (boîte grise) : ligne droite.
		AI->MoveToActor(Player, Acceptance, true, false, false, nullptr, true);
	}
}

void AMedievalKnight::FinishAttack()
{
	bAttacking = false;
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

	if (AttackSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, AttackSound, GetActorLocation());
	}

	// Un peu de tolérance : un joueur qui recule à la dernière frame est quand même touché.
	if (GetGapToPlayer() <= AttackRange * 1.3f)
	{
		// Le bouclier du joueur réduit ces dégâts s'il fait face au chevalier.
		PlayerHealth->ApplyDamage(AttackDamage, this, false, true);
		if (bDrawDebug)
		{
			DrawDebugSphere(GetWorld(), GetActorLocation() + GetActorForwardVector() * 70.f, 35.f, 8, FColor::Red, false, 0.2f);
		}
	}
}

void AMedievalKnight::OnDeath(AActor* Killer, bool bHeadshot, bool bMelee)
{
	GetWorldTimerManager().ClearTimer(ThinkTimer);
	GetWorldTimerManager().ClearTimer(AttackTimer);
	bAttacking = false;

	Super::OnDeath(Killer, bHeadshot, bMelee);

	if (DeathSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, DeathSound, GetActorLocation());
	}

	// Mort théâtrale : l'armure part en physique, projetée loin du tueur, le heaume s'envole.
	FVector Away = Killer ? (GetActorLocation() - Killer->GetActorLocation()) : -GetActorForwardVector();
	Away.Z = 0.f;
	Away = Away.GetSafeNormal();
	if (Away.IsNearlyZero())
	{
		Away = -GetActorForwardVector();
	}

	auto Collapse = [&](UStaticMeshComponent* Part, float UpBoost, float Spin)
	{
		if (!Part || !Part->GetStaticMesh())
		{
			return;
		}
		Part->SetCollisionProfileName(UCollisionProfile::PhysicsActor_ProfileName);
		Part->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
		Part->SetSimulatePhysics(true);
		Part->AddImpulse((Away + FVector(0.f, 0.f, UpBoost)) * DeathImpulse, NAME_None, true);
		Part->AddAngularImpulseInDegrees(FVector(FMath::FRandRange(-1.f, 1.f), FMath::FRandRange(-1.f, 1.f), 0.f) * Spin, NAME_None, true);
	};

	Collapse(BodyMesh, 0.5f, 400.f);
	Collapse(HelmetMesh, bHeadshot ? 2.f : 1.2f, 900.f);
}
