#include "Space/SpaceDrone.h"
#include "Space/SpaceLaserFX.h"
#include "Core/HealthComponent.h"
#include "Core/ReshootCharacter.h"
#include "RESHOOT.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	FCollisionObjectQueryParams DroneHitObjectTypes()
	{
		FCollisionObjectQueryParams Types;
		Types.AddObjectTypesToQuery(ECC_WorldStatic);
		Types.AddObjectTypesToQuery(ECC_WorldDynamic);
		Types.AddObjectTypesToQuery(ECC_Pawn);
		return Types;
	}

	const FLinearColor DefaultShotColor(1.f, 0.15f, 0.1f);
}

ASpaceDrone::ASpaceDrone(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Universe = EUniverse::Space;

	GetCapsuleComponent()->InitCapsuleSize(45.f, 45.f);

	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->DefaultLandMovementMode = MOVE_Flying;
	Move->GravityScale = 0.f;
	Move->MaxFlySpeed = FlySpeed;
	Move->BrakingDecelerationFlying = 0.f; // La vitesse est imposée par Steer() ; aucun freinage entre deux mises à jour.
	Move->GetNavAgentPropertiesRef().bCanFly = true;
	Move->bOrientRotationToMovement = false;
	bUseControllerRotationYaw = false;

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

	LaserFX = CreateDefaultSubobject<USpaceLaserFXComponent>(TEXT("LaserFX"));
	LaserFX->BeamColor = DefaultShotColor;
	LaserFX->PoolSize = 2;
	LaserFX->CoreThickness = 1.2f;
	LaserFX->GlowThickness = 7.f;
	LaserFX->BoltSpeed = 9000.f;   // plus lent que le joueur : on voit venir le tir
	LaserFX->BeamLifetime = 0.18f;

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

	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->MaxFlySpeed = FlySpeed;
	Move->GravityScale = 0.f;
	Move->SetMovementMode(MOVE_Flying);

	// Les points d'apparition du plateau sont hors du studio spatial : un drone trop loin se replace devant le
	// joueur, dans le studio (sinon il reste coincé derrière les murs et le joueur ne le voit jamais).
	if (const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0))
	{
		const FVector PlayerLocation = Player->GetActorLocation();
		if (FVector::Dist2D(GetActorLocation(), PlayerLocation) > 1600.f)
		{
			const float Yaw = Player->GetControlRotation().Yaw + FMath::FRandRange(-70.f, 70.f);
			const FVector Dir = FRotator(0.f, Yaw, 0.f).Vector();
			for (int32 Attempt = 0; Attempt < 6; ++Attempt)
			{
				const float Dist = FMath::FRandRange(900.f, 1400.f) * (1.f - Attempt * 0.12f);
				const FVector Target = PlayerLocation + Dir * Dist + FVector(0.f, 0.f, FMath::FRandRange(150.f, 350.f));
				FHitResult Hit;
				FCollisionQueryParams Query(SCENE_QUERY_STAT(DroneRelocate), false, this);
				Query.AddIgnoredActor(Player);
				// Ligne de vue depuis le joueur : pas de mur entre lui et le drone.
				if (!GetWorld()->LineTraceSingleByChannel(Hit, PlayerLocation + FVector(0.f, 0.f, 60.f), Target, ECC_Visibility, Query)
					&& TeleportTo(Target, (PlayerLocation - Target).Rotation()))
				{
					break;
				}
			}
		}
	}

	if (ShotMaterial)
	{
		ShotBeam->SetMaterial(0, ShotMaterial);
	}
	else if (UMaterialInstanceDynamic* Dynamic = ShotBeam->CreateAndSetMaterialInstanceDynamic(0))
	{
		Dynamic->SetVectorParameterValue(TEXT("Color"), DefaultShotColor);
	}

	// Costume carton / réaliste : l'accessoire est tout le corps du drone. La sphère grise reste
	// (invisible) pour la physique de la mort, l'accessoire la suit.
	if (AccessoryProp || AccessoryReal)
	{
		Accessory->AttachToComponent(Body, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		Body->SetVisibility(false, false);
	}

	GetWorldTimerManager().SetTimer(SteerTimer, this, &ASpaceDrone::Steer, SteerInterval, true, FMath::FRandRange(0.f, SteerInterval));
	TrackFreezableTimer(SteerTimer);
}

// ---------------------------------------------------------------------------
// Déplacement : ligne droite vers le joueur, pas de navmesh
// ---------------------------------------------------------------------------

void ASpaceDrone::Steer()
{
	if (IsDead() || IsFrozen() || bGravityEnabled)
	{
		return;
	}
	const AReshootCharacter* Player = GetTargetPlayer();
	if (!Player)
	{
		return;
	}

	const FVector ToPlayer = Player->GetActorLocation() - GetActorLocation();
	const float Distance = ToPlayer.Size();
	if (Distance > KINDA_SMALL_NUMBER)
	{
		SetActorRotation(ToPlayer.Rotation());
	}
	if (Distance > StopDistance)
	{
		// Ligne droite : la vitesse est imposée directement, sans accélération ni navmesh.
		FVector Direction = ToPlayer / Distance;

		// Évitement minimal : un décor bloque le passage juste devant -> on monte par-dessus
		// (le plateau n'a pas de plafond). Pas de calcul de chemin.
		const float LookAhead = FMath::Min(Distance, 400.f);
		FCollisionQueryParams WallParams(SCENE_QUERY_STAT(SpaceDroneAvoid), false, this);
		WallParams.AddIgnoredActor(Player);
		FCollisionObjectQueryParams Walls;
		Walls.AddObjectTypesToQuery(ECC_WorldStatic);
		Walls.AddObjectTypesToQuery(ECC_WorldDynamic);
		FHitResult WallHit;
		if (GetWorld()->SweepSingleByObjectType(WallHit, GetActorLocation(), GetActorLocation() + Direction * LookAhead,
			FQuat::Identity, Walls, FCollisionShape::MakeSphere(GetCapsuleComponent()->GetScaledCapsuleRadius()), WallParams))
		{
			Direction = (FVector(Direction.X, Direction.Y, 0.f) * 0.2f + FVector::UpVector).GetSafeNormal();
		}
		GetCharacterMovement()->Velocity = Direction * FlySpeed;
	}
	else
	{
		GetCharacterMovement()->Velocity = FVector::ZeroVector;
	}

	// Tir : armé quand le joueur entre à portée, coupé quand il sort.
	const bool bInRange = Distance <= FireRange;
	const bool bFiring = GetWorldTimerManager().TimerExists(FireTimer);
	if (bInRange && !bFiring)
	{
		GetWorldTimerManager().SetTimer(FireTimer, this, &ASpaceDrone::TryFire, 1.f / FMath::Max(FireRate, 0.1f), true, FirstShotDelay);
		TrackFreezableTimer(FireTimer);
	}
	else if (!bInRange && bFiring)
	{
		GetWorldTimerManager().ClearTimer(FireTimer);
	}
}

// ---------------------------------------------------------------------------
// Tir
// ---------------------------------------------------------------------------

void ASpaceDrone::TryFire()
{
	if (IsDead() || IsFrozen() || bGravityEnabled)
	{
		return;
	}
	AReshootCharacter* Player = GetTargetPlayer();
	if (!Player)
	{
		return;
	}

	FVector Target = Player->GetActorLocation();
	if (const UCameraComponent* Camera = Player->GetFirstPersonCamera())
	{
		Target = Camera->GetComponentLocation() - FVector(0.f, 0.f, 20.f); // Le haut du torse.
	}

	FVector Direction = (Target - GetActorLocation()).GetSafeNormal();
	const FVector Start = GetActorLocation() + Direction * MuzzleForward;
	if (SpreadDegrees > 0.f)
	{
		Direction = FMath::VRandCone(Direction, FMath::DegreesToRadians(SpreadDegrees));
	}
	const FVector End = Start + Direction * FireRange * 1.2f;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(SpaceDroneTrace), false, this);
	FHitResult Hit;
	const bool bHit = GetWorld()->LineTraceSingleByObjectType(Hit, Start, End, DroneHitObjectTypes(), Params);

	if (bHit && Hit.GetActor() == Player)
	{
		if (UHealthComponent* PlayerHealth = Player->GetHealth())
		{
			PlayerHealth->ApplyDamage(DamagePerShot, this);
		}
	}

	// Tir visible : rayon rouge, trait, éclair, impact (le joueur ne reçoit ni brûlure ni étincelles sur lui).
	const FVector Impact = bHit ? FVector(Hit.ImpactPoint) : End;
	const bool bHitPlayer = bHit && Hit.GetActor() == Player;
	const bool bStaticSurface = bHit && Hit.GetActor() && !Hit.GetActor()->IsA<APawn>();
	if (IsDressingReal())
	{
		LaserFX->FireBeam(Start, Impact, bHit && !bHitPlayer, bHit ? FVector(Hit.ImpactNormal) : FVector::ZeroVector, bStaticSurface);
	}
	else if (const float Length = FVector::Distance(Start, Impact); Length > KINDA_SMALL_NUMBER)
	{
		// Carton : un simple tube, sans effet.
		const FVector Delta = Impact - Start;
		ShotBeam->SetWorldLocationAndRotation(Start + Delta * 0.5f, FRotationMatrix::MakeFromZ(Delta / Length).Rotator());
		ShotBeam->SetWorldScale3D(FVector(0.04f, 0.04f, Length / 100.f));
		ShotBeam->SetHiddenInGame(false);
		GetWorldTimerManager().SetTimer(ShotBeamTimer, FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			ShotBeam->SetHiddenInGame(true);
		}), 0.06f, false);
	}

	if (IsDressingReal())
	{
		if (FireSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, FireSound, Start);
		}
	}
	else if (PropFireSounds.Num() > 0)
	{
		// Carton : un des sons faits à la bouche, au hasard, hauteur légèrement variée.
		if (USoundBase* Sound = PropFireSounds[FMath::RandHelper(PropFireSounds.Num())])
		{
			UGameplayStatics::PlaySoundAtLocation(this, Sound, Start, 1.f, 1.f + FMath::FRandRange(-PropPitchJitter, PropPitchJitter));
		}
	}
}

// ---------------------------------------------------------------------------
// Gel, gravité, mort
// ---------------------------------------------------------------------------

void ASpaceDrone::OnFreezeChanged(bool bFreeze)
{
	Super::OnFreezeChanged(bFreeze);
	if (bFreeze)
	{
		ShotBeam->SetHiddenInGame(true);
		GetCharacterMovement()->Velocity = FVector::ZeroVector;
	}
}

void ASpaceDrone::SetGravityEnabled(bool bGravityOn)
{
	if (IsDead() || bGravityEnabled == bGravityOn)
	{
		return;
	}
	bGravityEnabled = bGravityOn;

	UCharacterMovementComponent* Move = GetCharacterMovement();
	if (bGravityOn)
	{
		GetWorldTimerManager().ClearTimer(FireTimer);
		ShotBeam->SetHiddenInGame(true);
		Move->GravityScale = 1.f;
		Move->DefaultLandMovementMode = MOVE_Walking;
		Move->SetMovementMode(MOVE_Falling);
	}
	else
	{
		Move->GravityScale = 0.f;
		Move->DefaultLandMovementMode = MOVE_Flying;
		Move->SetMovementMode(MOVE_Flying);
		Move->Velocity = FVector(0.f, 0.f, 200.f); // Petit coup de réacteur en redécollant.
	}
}

void ASpaceDrone::OnDeath(AActor* Killer, bool bHeadshot, bool bMelee)
{
	GetWorldTimerManager().ClearTimer(SteerTimer);
	GetWorldTimerManager().ClearTimer(FireTimer);
	GetWorldTimerManager().ClearTimer(ShotBeamTimer);
	ShotBeam->SetHiddenInGame(true);

	if (DeathSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, DeathSound, GetActorLocation());
	}

	Super::OnDeath(Killer, bHeadshot, bMelee);

	// Mort théâtrale : le corps tombe en tournoyant (physique sur le mesh seul, la capsule n'a plus de collision).
	Body->SetCollisionProfileName(TEXT("PhysicsActor"));
	Body->SetSimulatePhysics(true);
	Body->SetEnableGravity(true);
	Body->AddImpulse(FVector(FMath::FRandRange(-150.f, 150.f), FMath::FRandRange(-150.f, 150.f), 250.f), NAME_None, true);
	Body->AddAngularImpulseInDegrees(FVector(FMath::FRandRange(-720.f, 720.f), FMath::FRandRange(-720.f, 720.f), 720.f), NAME_None, true);
}
