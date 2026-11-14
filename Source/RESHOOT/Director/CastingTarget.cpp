#include "Director/CastingTarget.h"
#include "Director/ShootDirectorSubsystem.h"
#include "Core/HealthComponent.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"

ACastingTarget::ACastingTarget()
{
	PrimaryActorTick.bCanEverTick = false;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));

	// Panneau de carton 1 m x 1,6 m posé sur un pivot au sol : il bascule en arrière quand il tombe.
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
	Board = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Board"));
	Board->SetupAttachment(GetRootComponent());
	Board->SetRelativeLocation(FVector(0.f, 0.f, 80.f));
	Board->SetStaticMesh(CubeMesh.Object);
	Board->SetRelativeScale3D(FVector(0.05f, 1.f, 1.6f));
	// Les armes tracent par type d'objet (WorldDynamic compris) et blessent tout acteur qui a un UHealthComponent.
	Board->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	Board->SetMobility(EComponentMobility::Movable);

	Bullseye = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Bullseye"));
	Bullseye->SetupAttachment(Board);
	Bullseye->SetStaticMesh(CylinderMesh.Object);
	Bullseye->SetRelativeLocation(FVector(-60.f, 0.f, 15.f));
	Bullseye->SetRelativeRotation(FRotator(90.f, 0.f, 0.f));
	Bullseye->SetRelativeScale3D(FVector(0.5f / 1.f, 0.5f / 0.05f, 0.02f / 1.6f));
	Bullseye->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Health = CreateDefaultSubobject<UHealthComponent>(TEXT("Health"));
	Health->MaxHealth = 1.f;
}

void ACastingTarget::BeginPlay()
{
	Super::BeginPlay();
	StandingRotation = GetActorRotation();
	Health->OnDeath.AddDynamic(this, &ACastingTarget::HandleDeath);
}

void ACastingTarget::ResetTarget()
{
	bDown = false;
	SetActorRotation(StandingRotation);
	Health->ResetHealth();
}

void ACastingTarget::HandleDeath(UHealthComponent* DeadComponent, AActor* Killer, bool bHeadshot, bool bMelee)
{
	if (bDown)
	{
		return;
	}
	bDown = true;
	SetActorRotation(StandingRotation + FRotator(DownPitch, 0.f, 0.f));
	if (UShootDirectorSubsystem* Director = GetWorld()->GetSubsystem<UShootDirectorSubsystem>())
	{
		Director->NotifyCastingTargetHit(this);
	}
}
