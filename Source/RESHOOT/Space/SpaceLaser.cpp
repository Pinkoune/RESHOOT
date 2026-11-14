#include "Space/SpaceLaser.h"
#include "Space/SpaceLaserFX.h"
#include "Core/ReshootCharacter.h"
#include "RESHOOT.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/CharacterMovementComponent.h"
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

	/** Modèle réaliste affiché : effets, visée et animations. En carton, rien de tout ça. */
	bool IsRealLook(const ASpaceLaser* Laser)
	{
		return Laser->IsDressingReal() && Laser->RealMesh;
	}
	const FLinearColor DefaultBeamColor(0.1f, 0.85f, 1.f);

	// Animation selon la direction de déplacement, à pleine vitesse (cm et degrés). En dur pour rester compatible
	// Live Coding ; à passer en UPROPERTY au prochain changement de header.
	constexpr float MoveLagForward = 1.5f;   // l'arme recule en avançant, avance en reculant
	constexpr float MoveLagSide = 1.0f;      // l'arme traîne à l'opposé du pas de côté
	constexpr float MovePitch = 1.2f;        // canon un peu plus bas en avançant
	constexpr float MoveYaw = 1.5f;
	constexpr float MoveRoll = 5.f;          // penche du côté du pas de côté
}

ASpaceLaser::ASpaceLaser()
{
	DisplayName = NSLOCTEXT("Reshoot", "SpaceLaser", "Fusil laser");

	// Tick : animations de l'arme en main (recul, balancement, visée) ; pas de logique de jeu ici.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	LaserFX = CreateDefaultSubobject<USpaceLaserFXComponent>(TEXT("LaserFX"));

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

	PropMeshTransform = Mesh->GetRelativeTransform();
	if (const AReshootCharacter* Character = GetOwnerCharacter())
	{
		if (const AController* Controller = Character->GetController())
		{
			LastControlRotation = Controller->GetControlRotation();
		}
	}

	// Pièces lumineuses du modèle réaliste : attachées au mesh, elles suivent le recul et la visée.
	for (const FTransform& Part : RealGlowParts)
	{
		UStaticMeshComponent* Comp = NewObject<UStaticMeshComponent>(this);
		Comp->SetStaticMesh(GlowPartMesh);
		Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Comp->SetCastShadow(false);
		Comp->SetupAttachment(Mesh);
		Comp->SetRelativeTransform(Part);
		Comp->RegisterComponent();
		UMaterialInstanceDynamic* Mid = GlowPartMaterial ? UMaterialInstanceDynamic::Create(GlowPartMaterial, Comp) : Comp->CreateAndSetMaterialInstanceDynamic(0);
		if (Mid)
		{
			Comp->SetMaterial(0, Mid);
		}
		GlowParts.Add(Comp);
		GlowPartMids.Add(Mid);
	}
	SetDressingReal(bDressingReal);

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
	if (LaserFX)
	{
		LaserFX->HideAll();
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

	if (IsRealLook(this))
	{
		// Brûlure sur le décor seulement : un décal ne suit pas un figurant qui bouge.
		const bool bStaticSurface = bHit && Hit.GetActor() && !Hit.GetActor()->IsA<APawn>();
		LaserFX->FireBeam(GetMuzzleLocation(), Impact, bHit, bHit ? FVector(Hit.ImpactNormal) : FVector::ZeroVector, bStaticSurface);
		KickAlpha = FMath::Min(KickAlpha + 1.f, 1.6f);
		KickYaw = FMath::FRandRange(-1.f, 1.f);
	}
	else
	{
		// Carton : un simple tube gris, sans effet.
		ShowBeam(GetMuzzleLocation(), Impact);
	}

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
	if (!IsRealLook(this))
	{
		return;   // pas de visée avec le fusil en carton
	}
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
	const bool bWasEnabled = bEnabled;
	Super::SetEnabled(bInEnabled);
	if (!bInEnabled)
	{
		GetWorldTimerManager().ClearTimer(FireTimer);
		HideBeam();
		if (LaserFX)
		{
			LaserFX->HideAll();
		}
	}
	else if (!bWasEnabled)
	{
		EquipAlpha = 0.f;   // l'arme remonte après le gel
	}
}

// ---------------------------------------------------------------------------
// Modèle et animations en main
// ---------------------------------------------------------------------------

void ASpaceLaser::SetDressingReal(bool bReal)
{
	Super::SetDressingReal(bReal);
	const bool bShowReal = bReal && RealMesh;
	if (!bShowReal && bAiming)
	{
		StopSecondary();
	}
	if (!bShowReal && LaserFX)
	{
		LaserFX->HideAll();
	}
	Mesh->SetRelativeTransform(GetBaseMeshTransform());
	for (UStaticMeshComponent* Part : GlowParts)
	{
		if (Part)
		{
			Part->SetVisibility(bShowReal);
		}
	}
}

FTransform ASpaceLaser::GetBaseMeshTransform() const
{
	return IsRealLook(this) ? RealMeshTransform : PropMeshTransform;
}

FVector ASpaceLaser::GetMuzzleLocation() const
{
	const FVector Local = (bDressingReal && RealMesh) ? RealMuzzleLocal : PropMeshTransform.InverseTransformPosition(MuzzleOffset);
	return Mesh->GetComponentTransform().TransformPosition(Local);
}

void ASpaceLaser::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	AnimTime += DeltaSeconds;

	// Carton : arme fixe, sans animation.
	if (!IsRealLook(this))
	{
		Mesh->SetRelativeTransform(PropMeshTransform);
		return;
	}

	const AReshootCharacter* Character = GetOwnerCharacter();
	const AController* Controller = Character ? Character->GetController() : nullptr;

	// Retard de l'arme quand la vue tourne (normalisé à 60 i/s pour ne pas dépendre de la cadence).
	FRotator SwayTarget = FRotator::ZeroRotator;
	if (Controller && DeltaSeconds > KINDA_SMALL_NUMBER)
	{
		const FRotator Control = Controller->GetControlRotation();
		const FRotator Delta = (Control - LastControlRotation).GetNormalized();
		LastControlRotation = Control;
		const float Scale = LookSway * (1.f - 0.7f * AimAlpha) / (DeltaSeconds * 60.f);
		SwayTarget.Pitch = FMath::Clamp(-Delta.Pitch * Scale, -MaxLookSway, MaxLookSway);
		SwayTarget.Yaw = FMath::Clamp(-Delta.Yaw * Scale, -MaxLookSway, MaxLookSway);
	}
	SwayRotation = FMath::RInterpTo(SwayRotation, SwayTarget, DeltaSeconds, 9.f);

	KickAlpha = FMath::FInterpTo(KickAlpha, 0.f, DeltaSeconds, KickReturnSpeed);
	AimAlpha = FMath::FInterpTo(AimAlpha, bAiming ? 1.f : 0.f, DeltaSeconds, AimSpeed);
	OverheatAlpha = FMath::FInterpTo(OverheatAlpha, bOverheated ? 1.f : 0.f, DeltaSeconds, 7.f);
	EquipAlpha = FMath::Min(EquipAlpha + DeltaSeconds / FMath::Max(EquipTime, 0.01f), 1.f);
	const float Equip = 1.f - FMath::Pow(1.f - EquipAlpha, 3.f);

	// Balancement de marche.
	float BobScale = 0.f;
	if (Character)
	{
		const UCharacterMovementComponent* Move = Character->GetCharacterMovement();
		const float Speed = Character->GetVelocity().Size2D();
		if (Move && Move->IsMovingOnGround() && Speed > 10.f)
		{
			BobPhase += DeltaSeconds * PI * Speed / FMath::Max(StrideLength, 1.f);
			BobScale = FMath::Clamp(Speed / 600.f, 0.f, 1.2f) * (1.f - 0.8f * AimAlpha);
		}
	}
	const float BobSin = FMath::Sin(BobPhase);

	// Inclinaison selon la direction de déplacement (clavier) : l'arme penche du côté où l'on se décale, recule
	// un peu en avançant. La vitesse du personnage monte et descend progressivement : pas besoin de lissage.
	float MoveForward = 0.f, MoveRight = 0.f;
	if (Character)
	{
		const UCharacterMovementComponent* Move = Character->GetCharacterMovement();
		const float MaxSpeed = Move ? FMath::Max(Move->GetMaxSpeed(), 1.f) : 600.f;
		const FVector LocalVel = Character->GetActorTransform().InverseTransformVectorNoScale(Character->GetVelocity()) / MaxSpeed;
		const float Damp = 1.f - 0.75f * AimAlpha;
		MoveForward = FMath::Clamp(LocalVel.X, -1.f, 1.f) * Damp;
		MoveRight = FMath::Clamp(LocalVel.Y, -1.f, 1.f) * Damp;
	}

	// Pose en main : modèle réaliste + recul, balancement, déplacement, levée, surchauffe.
	const FTransform Base = GetBaseMeshTransform();
	const FVector Loc = FVector(-KickBack * KickAlpha, 0.f, 0.f)
		+ FVector(-MoveForward * MoveLagForward, -MoveRight * MoveLagSide, -FMath::Abs(MoveForward) * 0.4f)
		+ FVector(0.f, BobSin * WalkBob * BobScale, -FMath::Abs(BobSin) * WalkBob * 0.8f * BobScale)
		+ FVector(0.f, 0.f, FMath::Sin(AnimTime * 1.6f) * 0.18f)
		+ FVector(0.f, 0.f, -18.f) * (1.f - Equip)
		+ FVector(-3.f, 2.f, -5.f) * OverheatAlpha;
	const FRotator Rot(
		KickPitch * KickAlpha + SwayRotation.Pitch - 35.f * (1.f - Equip) - 12.f * OverheatAlpha - MoveForward * MovePitch,
		SwayRotation.Yaw + KickYaw * KickAlpha * 0.6f - MoveRight * MoveYaw,
		SwayRotation.Yaw * 0.6f + 25.f * OverheatAlpha + MoveRight * MoveRoll);
	FTransform Final = Base * FTransform(Rot, Loc);

	// Visée, comme dans les FPS modernes : arme dans l'axe, viseur tout près de l'œil, sur l'axe de la caméra
	// (l'acteur est attaché à la caméra). Recul et balancement atténués.
	if (AimAlpha > KINDA_SMALL_NUMBER)
	{
		constexpr float AimEyeDistance = 16.f;
		const FVector CameraAxisInActor = -GetRootComponent()->GetRelativeLocation();
		const FVector Scale = Base.GetScale3D();
		const FVector Target(CameraAxisInActor.X + AimEyeDistance, CameraAxisInActor.Y, CameraAxisInActor.Z);
		const FTransform AimBase(FRotator::ZeroRotator, Target - Scale * RealSightLocal, Scale);
		const FTransform AimAnim(
			FRotator(KickPitch * KickAlpha * 0.35f + SwayRotation.Pitch * 0.3f, SwayRotation.Yaw * 0.3f + KickYaw * KickAlpha * 0.2f, 0.f),
			FVector(-KickBack * KickAlpha * 0.7f, 0.f, 0.f));
		FTransform Aimed;
		Aimed.Blend(Final, AimBase * AimAnim, FMath::SmoothStep(0.f, 1.f, AimAlpha));
		Final = Aimed;
	}
	Mesh->SetRelativeTransform(Final);

	// Pièces lumineuses : cyan au repos, orange en chauffant, clignotantes en surchauffe.
	if (bDressingReal && GlowPartMids.Num() > 0)
	{
		const FLinearColor Color = FMath::Lerp(CoolColor, HotColor, Heat);
		const float Pulse = bOverheated ? 0.55f + 0.45f * FMath::Sin(AnimTime * 18.f) : 1.f;
		const float Intensity = GlowPartIntensity * (0.7f + 0.6f * Heat) * Pulse;
		for (UMaterialInstanceDynamic* Mid : GlowPartMids)
		{
			if (Mid)
			{
				Mid->SetVectorParameterValue(TEXT("Color"), Color);
				Mid->SetScalarParameterValue(TEXT("Intensity"), Intensity);
			}
		}
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
