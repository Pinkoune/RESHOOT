#include "Space/SpaceLaser.h"
#include "Core/ReshootCharacter.h"
#include "RESHOOT.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	FCollisionObjectQueryParams LaserHitObjectTypes()
	{
		FCollisionObjectQueryParams Types;
		Types.AddObjectTypesToQuery(ECC_WorldStatic);
		Types.AddObjectTypesToQuery(ECC_WorldDynamic);
		Types.AddObjectTypesToQuery(ECC_Pawn);
		Types.AddObjectTypesToQuery(ECC_PhysicsBody);
		return Types;
	}

	constexpr float HeatTickInterval = 0.05f;
	const FLinearColor DefaultBeamColor(0.1f, 0.85f, 1.f);
}

ASpaceLaser::ASpaceLaser()
{
	DisplayName = NSLOCTEXT("Reshoot", "SpaceLaser", "Fusil laser");

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));

	// Boîte grise en attendant le mesh généré (assigné dans BP_SpaceLaser).
	if (CubeMesh.Succeeded())
	{
		Mesh->SetStaticMesh(CubeMesh.Object);
		Mesh->SetRelativeLocation(FVector(15.f, 0.f, 0.f));
		Mesh->SetRelativeScale3D(FVector(0.6f, 0.08f, 0.1f));
	}

	Beam = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Beam"));
	Beam->SetupAttachment(Root);
	Beam->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Beam->SetCastShadow(false);
	Beam->SetHiddenInGame(true);
	Beam->SetUsingAbsoluteLocation(true);
	Beam->SetUsingAbsoluteRotation(true);
	Beam->SetUsingAbsoluteScale(true);
	if (CylinderMesh.Succeeded())
	{
		Beam->SetStaticMesh(CylinderMesh.Object);
	}
}

void ASpaceLaser::BeginPlay()
{
	Super::BeginPlay();

	if (BeamMaterial)
	{
		Beam->SetMaterial(0, BeamMaterial);
	}
	else if (UMaterialInstanceDynamic* Dynamic = Beam->CreateAndSetMaterialInstanceDynamic(0))
	{
		// BasicShapeMaterial expose un paramètre « Color » : rayon cyan lisible sans aucun asset.
		Dynamic->SetVectorParameterValue(TEXT("Color"), DefaultBeamColor);
	}
}

void ASpaceLaser::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(FireTimer);
		World->GetTimerManager().ClearTimer(HeatTimer);
		World->GetTimerManager().ClearTimer(BeamTimer);
	}
	Super::EndPlay(EndPlayReason);
}

// ---------------------------------------------------------------------------
// Tir
// ---------------------------------------------------------------------------

void ASpaceLaser::StartPrimary()
{
	Super::StartPrimary();
	if (!bEnabled || bOverheated)
	{
		return;
	}

	// Respecte la cadence même si le joueur martèle le clic.
	const float Interval = 1.f / FMath::Max(FireRate, 0.1f);
	const float SinceLast = GetWorld()->GetTimeSeconds() - LastShotTime;
	const float FirstDelay = FMath::Max(Interval - SinceLast, 0.f);
	GetWorldTimerManager().SetTimer(FireTimer, this, &ASpaceLaser::FireShot, Interval, true, FirstDelay);
}

void ASpaceLaser::StopPrimary()
{
	Super::StopPrimary();
	GetWorldTimerManager().ClearTimer(FireTimer);
}

void ASpaceLaser::FireShot()
{
	if (!bPrimaryHeld || !bEnabled || bOverheated || !IsActionAllowed(EWeaponAction::Primary))
	{
		GetWorldTimerManager().ClearTimer(FireTimer);
		return;
	}

	FVector Start, Direction;
	GetAim(Start, Direction);
	const float Spread = bAiming ? AimSpreadDegrees : SpreadDegrees;
	if (Spread > 0.f)
	{
		Direction = FMath::VRandCone(Direction, FMath::DegreesToRadians(Spread));
	}
	const FVector End = Start + Direction * Range;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(SpaceLaserTrace), true, this);
	Params.AddIgnoredActor(GetOwner());
	FHitResult Hit;
	const bool bHit = GetWorld()->LineTraceSingleByObjectType(Hit, Start, End, LaserHitObjectTypes(), Params);
	const FVector Impact = bHit ? Hit.ImpactPoint : End;

	if (bHit && Hit.GetActor())
	{
		DealDamage(Hit.GetActor(), DamagePerShot, IsHeadshot(Hit), false);
	}

	ShowBeam(GetActorTransform().TransformPosition(MuzzleOffset), Impact);

	if (FireSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, FireSound, GetActorLocation());
	}

	// Recul : léger coup vers le haut sur la rotation de contrôle (pitch positif = vers le haut).
	if (AReshootCharacter* Character = GetOwnerCharacter())
	{
		if (AController* Controller = Character->GetController())
		{
			Controller->SetControlRotation(Controller->GetControlRotation() + FRotator(RecoilPitch, 0.f, 0.f));
		}
	}

	LastShotTime = GetWorld()->GetTimeSeconds();
	BroadcastWeaponEvent(EWeaponEvent::Fired);

	// Chaleur : SecondsToOverheat de tir continu, accéléré par « Le laser coûte cher ! ».
	const float HeatPerShot = 1.f / (FMath::Max(SecondsToOverheat, 0.1f) * FMath::Max(FireRate, 0.1f));
	SetHeat(Heat + HeatPerShot * GetCostMultiplier());

	if (Heat >= 1.f)
	{
		bOverheated = true;
		GetWorldTimerManager().ClearTimer(FireTimer);
		if (OverheatSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, OverheatSound, GetActorLocation());
		}
		BroadcastWeaponEvent(EWeaponEvent::Overheated);
	}

	if (!GetWorldTimerManager().IsTimerActive(HeatTimer))
	{
		GetWorldTimerManager().SetTimer(HeatTimer, this, &ASpaceLaser::UpdateHeat, HeatTickInterval, true);
	}
}

// ---------------------------------------------------------------------------
// Chaleur
// ---------------------------------------------------------------------------

void ASpaceLaser::SetHeat(float NewHeat)
{
	Heat = FMath::Clamp(NewHeat, 0.f, 1.f);
}

void ASpaceLaser::UpdateHeat()
{
	const float CoolPerTick = HeatTickInterval / FMath::Max(CooldownTime, 0.1f);

	if (bOverheated)
	{
		SetHeat(Heat - CoolPerTick);
		if (Heat <= 0.f)
		{
			bOverheated = false;
			if (CooledSound)
			{
				UGameplayStatics::PlaySoundAtLocation(this, CooledSound, GetActorLocation());
			}
			BroadcastWeaponEvent(EWeaponEvent::CooledDown);

			// Gâchette toujours tenue : le tir reprend tout seul.
			if (bPrimaryHeld && bEnabled)
			{
				StartPrimary();
			}
		}
		return;
	}

	const bool bFiring = GetWorldTimerManager().IsTimerActive(FireTimer);
	const float SinceLast = GetWorld()->GetTimeSeconds() - LastShotTime;
	if (!bFiring && SinceLast >= CooldownDelay)
	{
		SetHeat(Heat - CoolPerTick);
	}

	if (Heat <= 0.f && !bFiring)
	{
		GetWorldTimerManager().ClearTimer(HeatTimer);
	}
}

void ASpaceLaser::Refill()
{
	const bool bWasOverheated = bOverheated;
	SetHeat(0.f);
	bOverheated = false;
	if (bWasOverheated)
	{
		BroadcastWeaponEvent(EWeaponEvent::CooledDown);
	}
}

// ---------------------------------------------------------------------------
// Visée
// ---------------------------------------------------------------------------

void ASpaceLaser::StartSecondary()
{
	Super::StartSecondary();
	bAiming = true;
	if (AReshootCharacter* Character = GetOwnerCharacter())
	{
		Character->SetAimFOV(AimFOV);
	}
}

void ASpaceLaser::StopSecondary()
{
	const bool bWasAiming = bAiming;
	Super::StopSecondary();
	bAiming = false;
	if (bWasAiming)
	{
		if (AReshootCharacter* Character = GetOwnerCharacter())
		{
			Character->ResetAimFOV();
		}
	}
}

void ASpaceLaser::SetEnabled(bool bInEnabled)
{
	Super::SetEnabled(bInEnabled);
	if (!bInEnabled)
	{
		GetWorldTimerManager().ClearTimer(FireTimer);
		HideBeam();
	}
}

// ---------------------------------------------------------------------------
// Rayon visible
// ---------------------------------------------------------------------------

void ASpaceLaser::ShowBeam(const FVector& From, const FVector& To)
{
	const FVector Delta = To - From;
	const float Length = Delta.Size();
	if (Length < KINDA_SMALL_NUMBER)
	{
		return;
	}

	// Le cylindre de base fait 100 unités de haut, centré, axe Z.
	Beam->SetWorldLocationAndRotation(From + Delta * 0.5f, FRotationMatrix::MakeFromZ(Delta / Length).Rotator());
	Beam->SetWorldScale3D(FVector(BeamThickness, BeamThickness, Length / 100.f));
	Beam->SetHiddenInGame(false);

	GetWorldTimerManager().SetTimer(BeamTimer, this, &ASpaceLaser::HideBeam, BeamVisibleTime, false);
}

void ASpaceLaser::HideBeam()
{
	Beam->SetHiddenInGame(true);
}
