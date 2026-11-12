#include "Space/SpaceDrone.h"
#include "Core/HealthComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "UObject/ConstructorHelpers.h"

// Lot de Codex : comportement à écrire dans ce fichier (voir docs/briefs/codex-space-drone.md).

ASpaceDrone::ASpaceDrone(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Universe = EUniverse::Space;

	GetCapsuleComponent()->InitCapsuleSize(45.f, 45.f);

	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->DefaultLandMovementMode = MOVE_Flying;
	Move->GravityScale = 0.f;
	Move->MaxFlySpeed = FlySpeed;
	Move->BrakingDecelerationFlying = 1000.f;
	Move->GetNavAgentPropertiesRef().bCanFly = true;

	HealthComponent->MaxHealth = 40.f;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));

	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	Body->SetupAttachment(GetCapsuleComponent());
	Body->SetCollisionProfileName(TEXT("NoCollision"));
	Body->SetRelativeScale3D(FVector(0.8f));
	if (SphereMesh.Succeeded())
	{
		Body->SetStaticMesh(SphereMesh.Object);
	}

	ShotBeam = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShotBeam"));
	ShotBeam->SetupAttachment(GetCapsuleComponent());
	ShotBeam->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ShotBeam->SetCastShadow(false);
	ShotBeam->SetHiddenInGame(true);
	ShotBeam->SetUsingAbsoluteLocation(true);
	ShotBeam->SetUsingAbsoluteRotation(true);
	ShotBeam->SetUsingAbsoluteScale(true);
	if (CylinderMesh.Succeeded())
	{
		ShotBeam->SetStaticMesh(CylinderMesh.Object);
	}
}

void ASpaceDrone::BeginPlay()
{
	Super::BeginPlay();
}

void ASpaceDrone::SetGravityEnabled(bool bGravityOn)
{
	bGravityEnabled = bGravityOn;
}

void ASpaceDrone::OnFreezeChanged(bool bFreeze)
{
	Super::OnFreezeChanged(bFreeze);
}

void ASpaceDrone::OnDeath(AActor* Killer, bool bHeadshot, bool bMelee)
{
	Super::OnDeath(Killer, bHeadshot, bMelee);
}

void ASpaceDrone::Steer()
{
}

void ASpaceDrone::TryFire()
{
}
