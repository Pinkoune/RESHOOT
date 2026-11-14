#include "Director/CastingTarget.h"
#include "Director/ShootDirectorSubsystem.h"
#include "Core/HealthComponent.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

ACastingTarget::ACastingTarget()
{
	PrimaryActorTick.bCanEverTick = false;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> CardboardMat(TEXT("/Game/RESHOOT/Director/Casting/MI_Casting_Cardboard.MI_Casting_Cardboard"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> RedMat(TEXT("/Game/RESHOOT/Director/Casting/MI_Casting_TargetRed.MI_Casting_TargetRed"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> WhiteMat(TEXT("/Game/RESHOOT/Director/Casting/MI_Casting_White.MI_Casting_White"));

	// Panneau de carton 1 m x 1,6 m posé sur un pivot au sol : il bascule en arrière quand il tombe.
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
	Board = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Board"));
	Board->SetupAttachment(GetRootComponent());
	Board->SetRelativeLocation(FVector(0.f, 0.f, 80.f));
	Board->SetStaticMesh(CubeMesh.Object);
	Board->SetRelativeScale3D(FVector(0.05f, 1.f, 1.6f));
	Board->SetMaterial(0, CardboardMat.Object);
	// Les armes tracent par type d'objet (WorldDynamic compris) et blessent tout acteur qui a un UHealthComponent.
	Board->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	Board->SetMobility(EComponentMobility::Movable);

	// Cible peinte sur la face avant (+X) : trois disques rouge, blanc, rouge. Attachés à la racine, pas au panneau,
	// pour ne pas hériter de son échelle non uniforme.
	auto MakeDisc = [&](const TCHAR* Name, float Diameter, float Offset, UMaterialInterface* Material)
	{
		UStaticMeshComponent* Disc = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Disc->SetupAttachment(GetRootComponent());
		Disc->SetStaticMesh(CylinderMesh.Object);
		Disc->SetRelativeLocation(FVector(2.5f + Offset, 0.f, 105.f));
		Disc->SetRelativeRotation(FRotator(90.f, 0.f, 0.f));
		Disc->SetRelativeScale3D(FVector(Diameter / 100.f, Diameter / 100.f, 0.01f));
		Disc->SetMaterial(0, Material);
		Disc->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		return Disc;
	};
	Bullseye = MakeDisc(TEXT("Bullseye"), 56.f, 0.6f, RedMat.Object);
	MakeDisc(TEXT("BullseyeRing"), 38.f, 1.2f, WhiteMat.Object);
	MakeDisc(TEXT("BullseyeCenter"), 18.f, 1.8f, RedMat.Object);

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
