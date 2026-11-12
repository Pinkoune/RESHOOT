#include "Space/SpaceLaser.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

ASpaceLaser::ASpaceLaser()
{
	DisplayName = NSLOCTEXT("Reshoot", "SpaceLaser", "Fusil laser");

	Beam = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Beam"));
	Beam->SetupAttachment(Root);
	Beam->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Beam->SetCastShadow(false);
	Beam->SetHiddenInGame(true);
	Beam->SetUsingAbsoluteLocation(true);
	Beam->SetUsingAbsoluteRotation(true);
	Beam->SetUsingAbsoluteScale(true);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderMesh.Succeeded())
	{
		Beam->SetStaticMesh(CylinderMesh.Object);
	}
}

void ASpaceLaser::BeginPlay()
{
	Super::BeginPlay();
}

void ASpaceLaser::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void ASpaceLaser::StartPrimary()
{
	Super::StartPrimary();
}

void ASpaceLaser::StopPrimary()
{
	Super::StopPrimary();
}

void ASpaceLaser::StartSecondary()
{
	Super::StartSecondary();
}

void ASpaceLaser::StopSecondary()
{
	Super::StopSecondary();
}

void ASpaceLaser::SetEnabled(bool bInEnabled)
{
	Super::SetEnabled(bInEnabled);
}

void ASpaceLaser::Refill()
{
}

void ASpaceLaser::FireShot()
{
}

void ASpaceLaser::UpdateHeat()
{
}

void ASpaceLaser::ShowBeam(const FVector& From, const FVector& To)
{
}

void ASpaceLaser::HideBeam()
{
}

void ASpaceLaser::SetHeat(float NewHeat)
{
}
