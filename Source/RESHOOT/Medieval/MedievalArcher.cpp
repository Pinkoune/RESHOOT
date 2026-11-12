#include "Medieval/MedievalArcher.h"
#include "Medieval/MedievalArrow.h"
#include "Core/HealthComponent.h"
#include "Core/ReshootCharacter.h"
#include "RESHOOT.h"

#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

AMedievalArcher::AMedievalArcher(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Universe = EUniverse::Medieval;
	ArrowClass = AMedievalArrow::StaticClass();
	HealthComponent->MaxHealth = 50.f;

	// Fixe : il ne marche jamais.
	GetCharacterMovement()->MaxWalkSpeed = 0.f;
	bUseControllerRotationYaw = false;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
	BodyMesh->SetupAttachment(GetCapsuleComponent());
	BodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BodyMesh->SetRelativeScale3D(FVector(0.5f, 0.5f, 1.75f));
	if (CylinderMesh.Succeeded())
	{
		BodyMesh->SetStaticMesh(CylinderMesh.Object);
	}
}

void AMedievalArcher::BeginPlay()
{
	Super::BeginPlay();

	const float FirstDelay = FireInterval * 0.5f + FMath::FRandRange(0.f, FirstShotMaxDelay);
	GetWorldTimerManager().SetTimer(FireTimer, this, &AMedievalArcher::Fire, FireInterval, true, FirstDelay);

	if (IsFrozen())
	{
		GetWorldTimerManager().PauseTimer(FireTimer);
	}
}

void AMedievalArcher::OnFreezeChanged(bool bFreeze)
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

void AMedievalArcher::OnDeath(AActor* Killer, bool bHeadshot, bool bMelee)
{
	GetWorldTimerManager().ClearTimer(FireTimer);
	Super::OnDeath(Killer, bHeadshot, bMelee);

	// Mort théâtrale : bascule en physique et tombe de son perchoir.
	if (BodyMesh && BodyMesh->GetStaticMesh())
	{
		BodyMesh->SetCollisionProfileName(TEXT("PhysicsActor"));
		BodyMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
		BodyMesh->SetSimulatePhysics(true);
		FVector Away = Killer ? (GetActorLocation() - Killer->GetActorLocation()).GetSafeNormal2D() : -GetActorForwardVector();
		BodyMesh->AddImpulse((Away + FVector(0.f, 0.f, 0.6f)) * 500.f, NAME_None, true);
		BodyMesh->AddAngularImpulseInDegrees(FVector(FMath::FRandRange(-1.f, 1.f), FMath::FRandRange(-1.f, 1.f), 0.f) * 500.f, NAME_None, true);
	}
}

void AMedievalArcher::Fire()
{
	if (IsFrozen() || IsDead() || !ArrowClass)
	{
		return;
	}

	AReshootCharacter* Player = GetTargetPlayer();
	const UHealthComponent* PlayerHealth = Player ? Player->GetHealth() : nullptr;
	if (!PlayerHealth || PlayerHealth->IsDead())
	{
		return;
	}

	const FVector Start = GetPawnViewLocation();
	const FVector PlayerLocation = Player->GetActorLocation();
	const float Distance = FVector::Dist(Start, PlayerLocation);
	if (Distance > Range)
	{
		return;
	}

	// Ligne de vue.
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(MedievalArcherSight), true, this);
	if (GetWorld()->LineTraceSingleByChannel(Hit, Start, PlayerLocation, ECC_Visibility, Params) && Hit.GetActor() != Player)
	{
		return;
	}

	// Anticipation partielle : rend la flèche esquivable mais pas inutile.
	const float TravelTime = ArrowSpeed > 0.f ? Distance / ArrowSpeed : 0.f;
	const FVector Target = PlayerLocation + Player->GetVelocity() * TravelTime * LeadFactor;
	const FVector Direction = (Target - Start).GetSafeNormal();

	FRotator Facing = Direction.Rotation();
	Facing.Pitch = 0.f;
	Facing.Roll = 0.f;
	SetActorRotation(Facing);

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	const FVector SpawnLocation = Start + Direction * (GetCapsuleComponent()->GetScaledCapsuleRadius() + 30.f);
	if (AMedievalArrow* Arrow = GetWorld()->SpawnActor<AMedievalArrow>(ArrowClass, SpawnLocation, Direction.Rotation(), SpawnParams))
	{
		// Flèche droite (sans gravité) : lente et lisible.
		Arrow->Launch(Direction, ArrowSpeed, ArrowDamage, 0.f);
	}

	if (FireSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, FireSound, Start);
	}
}
