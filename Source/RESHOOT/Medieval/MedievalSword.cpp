#include "Medieval/MedievalSword.h"
#include "Medieval/MedievalArrow.h"
#include "Core/ReshootCharacter.h"
#include "Core/ReshootEnemy.h"
#include "Core/HealthComponent.h"
#include "RESHOOT.h"

#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "Engine/OverlapResult.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

AMedievalSword::AMedievalSword()
{
	DisplayName = NSLOCTEXT("Reshoot", "MedievalSwordName", "Épée et bouclier");
	ArrowClass = AMedievalArrow::StaticClass();

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));

	// Épée : lame fine et longue, tenue à droite, pointe vers l'avant.
	if (CubeMesh.Succeeded())
	{
		Mesh->SetStaticMesh(CubeMesh.Object);
	}
	Mesh->SetRelativeLocation(FVector(25.f, 0.f, 0.f));
	Mesh->SetRelativeRotation(FRotator(20.f, 0.f, 0.f));
	Mesh->SetRelativeScale3D(FVector(0.9f, 0.04f, 0.08f));

	// Bouclier : plaque devant le bras gauche, baissée au repos.
	ShieldMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShieldMesh"));
	ShieldMesh->SetupAttachment(Root);
	if (CubeMesh.Succeeded())
	{
		ShieldMesh->SetStaticMesh(CubeMesh.Object);
	}
	ShieldMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ShieldMesh->SetCastShadow(false);
	ShieldMesh->SetRelativeLocation(FVector(0.f, -35.f, -15.f));
	ShieldMesh->SetRelativeScale3D(FVector(0.05f, 0.45f, 0.55f));

	// Arc : cylindre vertical fin.
	BowMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BowMesh"));
	BowMesh->SetupAttachment(Root);
	if (CylinderMesh.Succeeded())
	{
		BowMesh->SetStaticMesh(CylinderMesh.Object);
	}
	BowMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BowMesh->SetCastShadow(false);
	BowMesh->SetRelativeLocation(FVector(10.f, -15.f, 5.f));
	BowMesh->SetRelativeScale3D(FVector(0.05f, 0.05f, 1.1f));
	BowMesh->SetVisibility(false);

	// Poings : deux petits cubes, cachés hors du mode poings.
	auto MakeFist = [&](const TCHAR* Name)
	{
		UStaticMeshComponent* Fist = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Fist->SetupAttachment(Root);
		if (CubeMesh.Succeeded())
		{
			Fist->SetStaticMesh(CubeMesh.Object);
		}
		Fist->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Fist->SetCastShadow(false);
		Fist->SetRelativeScale3D(FVector(0.12f, 0.1f, 0.1f));
		Fist->SetVisibility(false);
		return Fist;
	};
	FistLeftMesh = MakeFist(TEXT("FistLeftMesh"));
	FistRightMesh = MakeFist(TEXT("FistRightMesh"));
}

void AMedievalSword::BeginPlay()
{
	Super::BeginPlay();
	SwordRestTransform = Mesh->GetRelativeTransform();
	BowMesh->SetRelativeTransform(BowRestTransform);
	UpdateVisuals();
	TrySetupInput();

	// Être blessé casse le combo.
	if (AReshootCharacter* Character = GetOwnerCharacter())
	{
		BoundOwnerHealth = Character->GetHealth();
		if (BoundOwnerHealth)
		{
			BoundOwnerHealth->OnHealthChanged.AddDynamic(this, &AMedievalSword::HandleOwnerHealthChanged);
		}
	}
}

void AMedievalSword::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	RaiseShield(false);
	GetWorldTimerManager().ClearAllTimersForObject(this);
	if (BoundOwnerHealth)
	{
		BoundOwnerHealth->OnHealthChanged.RemoveDynamic(this, &AMedievalSword::HandleOwnerHealthChanged);
		BoundOwnerHealth = nullptr;
	}

	if (InputController)
	{
		if (ULocalPlayer* LocalPlayer = InputController->GetLocalPlayer())
		{
			if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer))
			{
				if (MedievalMappingContext)
				{
					Subsystem->RemoveMappingContext(MedievalMappingContext);
				}
			}
		}
		DisableInput(InputController);
		InputController = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

// ---------------------------------------------------------------------------
// Entrées : clics transmis par le joueur, changement de mode géré ici.

void AMedievalSword::StartPrimary()
{
	Super::StartPrimary();
	if (!IsEnabled())
	{
		return;
	}

	if (Mode == EMedievalWeaponMode::Sword)
	{
		SwordSwing();
		if (bSwordAutoRepeat)
		{
			GetWorldTimerManager().SetTimer(SwingRepeatTimer, this, &AMedievalSword::SwordSwing, 1.f / SwordAttacksPerSecond, true);
		}
	}
	else if (Mode == EMedievalWeaponMode::Fists)
	{
		Punch();
		GetWorldTimerManager().SetTimer(SwingRepeatTimer, this, &AMedievalSword::Punch, 1.f / FistPunchesPerSecond, true);
	}
	else
	{
		StartBowCharge();
	}
}

void AMedievalSword::StopPrimary()
{
	Super::StopPrimary();
	GetWorldTimerManager().ClearTimer(SwingRepeatTimer);

	if (Mode == EMedievalWeaponMode::Bow && bCharging)
	{
		ReleaseBow();
	}
}

void AMedievalSword::StartSecondary()
{
	Super::StartSecondary();
	if (IsEnabled() && Mode != EMedievalWeaponMode::Bow)
	{
		RaiseShield(true);
	}
}

void AMedievalSword::StopSecondary()
{
	Super::StopSecondary();
	RaiseShield(false);
}

void AMedievalSword::SetEnabled(bool bInEnabled)
{
	if (!bInEnabled)
	{
		// Gel : la flèche bandée est perdue, pas tirée.
		bCharging = false;
	}
	Super::SetEnabled(bInEnabled);

	// Pendant le gel (CUT!), le combo ne s'écoule pas.
	if (GetWorldTimerManager().TimerExists(ComboTimer))
	{
		if (bInEnabled)
		{
			GetWorldTimerManager().UnPauseTimer(ComboTimer);
		}
		else
		{
			GetWorldTimerManager().PauseTimer(ComboTimer);
		}
	}
}

// ---------------------------------------------------------------------------
// Modes

void AMedievalSword::SetMode(EMedievalWeaponMode NewMode)
{
	if (NewMode == EMedievalWeaponMode::Bow && !bBowAllowed)
	{
		return;
	}
	if (NewMode == Mode)
	{
		return;
	}

	// On lâche tout avant de changer.
	GetWorldTimerManager().ClearTimer(SwingRepeatTimer);
	RaiseShield(false);
	bCharging = false;

	Mode = NewMode;
	switch (Mode)
	{
	case EMedievalWeaponMode::Sword: DisplayName = NSLOCTEXT("Reshoot", "MedievalSwordName", "Épée et bouclier"); break;
	case EMedievalWeaponMode::Bow:   DisplayName = NSLOCTEXT("Reshoot", "MedievalBowName", "Arc"); break;
	case EMedievalWeaponMode::Fists: DisplayName = NSLOCTEXT("Reshoot", "MedievalFistsName", "Poings"); break;
	}

	UpdateVisuals();
	OnModeChanged.Broadcast(Mode);

	// Clic droit toujours enfoncé : la garde (bouclier ou poings) se relève.
	if (Mode != EMedievalWeaponMode::Bow && bSecondaryHeld && IsEnabled())
	{
		RaiseShield(true);
	}
}

void AMedievalSword::ToggleMode()
{
	// Cycle épée -> arc -> poings -> épée (l'arc est sauté s'il est interdit).
	switch (Mode)
	{
	case EMedievalWeaponMode::Sword: SetMode(bBowAllowed ? EMedievalWeaponMode::Bow : EMedievalWeaponMode::Fists); break;
	case EMedievalWeaponMode::Bow:   SetMode(EMedievalWeaponMode::Fists); break;
	case EMedievalWeaponMode::Fists: SetMode(EMedievalWeaponMode::Sword); break;
	}
}

void AMedievalSword::SetShieldAllowed(bool bAllowed)
{
	bShieldAllowed = bAllowed;
	if (!bAllowed)
	{
		RaiseShield(false);
	}
	UpdateVisuals();
}

void AMedievalSword::SetBowAllowed(bool bAllowed)
{
	bBowAllowed = bAllowed;
	if (!bAllowed && Mode == EMedievalWeaponMode::Bow)
	{
		SetMode(EMedievalWeaponMode::Sword);
	}
}

void AMedievalSword::UpdateVisuals()
{
	const bool bSword = (Mode == EMedievalWeaponMode::Sword);
	const bool bFists = (Mode == EMedievalWeaponMode::Fists);
	Mesh->SetVisibility(bSword);
	ShieldMesh->SetVisibility(bSword && bShieldAllowed);
	BowMesh->SetVisibility(Mode == EMedievalWeaponMode::Bow);
	FistLeftMesh->SetVisibility(bFists);
	FistRightMesh->SetVisibility(bFists);

	// Bouclier levé : devant, au centre, sans croiser la lame. Épée baissée sur le côté pendant la garde.
	ShieldMesh->SetRelativeLocation(bShieldRaised && bSword ? ShieldRaisedLocation : ShieldLoweredLocation);
	if (bShieldRaised && bSword)
	{
		Mesh->SetRelativeTransform(SwordGuardTransform);
	}
	else if (!GetWorldTimerManager().IsTimerActive(SwingPoseTimer))
	{
		Mesh->SetRelativeTransform(SwordRestTransform);
	}

	if (!GetWorldTimerManager().IsTimerActive(PunchPoseTimer))
	{
		ResetPunchPose();
	}
}

// ---------------------------------------------------------------------------
// Épée

void AMedievalSword::SwordSwing()
{
	if (!IsEnabled() || bShieldRaised || Mode != EMedievalWeaponMode::Sword)
	{
		return;
	}

	const double Now = GetWorld()->GetTimeSeconds();
	// Petite marge pour que le timer de répétition ne soit pas refusé à cause des arrondis.
	if (Now - LastSwingTime < (1.f / SwordAttacksPerSecond) - 0.02)
	{
		return;
	}
	LastSwingTime = Now;

	FVector Start, Direction;
	GetAim(Start, Direction);

	if (SwingSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, SwingSound, GetActorLocation());
	}

	// Pose de coup : la lame bascule vers le bas, puis revient.
	Mesh->SetRelativeRotation(FRotator(-35.f, -25.f, 0.f));
	GetWorldTimerManager().SetTimer(SwingPoseTimer, this, &AMedievalSword::ResetSwingPose, 0.15f, false);

	const int32 HitCount = MeleeSweep(SwordRange, SwordHalfAngle, SwordDamage, 0.f);

	if (HitCount > 0 && SwordHitSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, SwordHitSound, Start + Direction * SwordRange * 0.5f);
	}
}

int32 AMedievalSword::MeleeSweep(float Range, float HalfAngle, float Damage, float Knockback)
{
	FVector Start, Direction;
	GetAim(Start, Direction);

	// Tous les pions dans la sphère de portée, puis filtre par angle.
	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(MedievalMeleeSweep), false, this);
	Params.AddIgnoredActor(GetOwner());
	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);
	ObjectParams.AddObjectTypesToQuery(ECC_PhysicsBody);
	GetWorld()->OverlapMultiByObjectType(Overlaps, Start, FQuat::Identity, ObjectParams, FCollisionShape::MakeSphere(Range), Params);

	const float CosHalfAngle = FMath::Cos(FMath::DegreesToRadians(HalfAngle));
	TSet<AActor*> AlreadyHit;
	int32 HitCount = 0;

	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Target = Overlap.GetActor();
		if (!Target || AlreadyHit.Contains(Target))
		{
			continue;
		}
		AlreadyHit.Add(Target);

		UHealthComponent* Health = Target->FindComponentByClass<UHealthComponent>();
		if (!Health || Health->IsDead())
		{
			continue;
		}

		// Point le plus proche de la cible, pour qu'un ennemi collé au joueur compte toujours.
		FVector Closest = Target->GetActorLocation();
		if (const UPrimitiveComponent* Prim = Overlap.GetComponent())
		{
			FVector OnBody;
			if (Prim->GetClosestPointOnCollision(Start, OnBody) > 0.f)
			{
				Closest = OnBody;
			}
		}
		const FVector ToTarget = Closest - Start;
		const float Distance = ToTarget.Size();
		if (Distance > Range)
		{
			continue;
		}
		// Collé : toujours touché. Sinon, doit être dans le cône.
		if (Distance > 40.f && FVector::DotProduct(ToTarget / Distance, Direction) < CosHalfAngle)
		{
			continue;
		}

		// Pas à travers les murs (seul le décor statique bloque : un coup peut toucher plusieurs figurants).
		FHitResult WallHit;
		FCollisionQueryParams SightParams(SCENE_QUERY_STAT(MedievalMeleeSight), false, this);
		SightParams.AddIgnoredActor(GetOwner());
		SightParams.AddIgnoredActor(Target);
		if (GetWorld()->LineTraceSingleByObjectType(WallHit, Start, Closest, FCollisionObjectQueryParams(ECC_WorldStatic), SightParams))
		{
			continue;
		}

		if (DealDamage(Target, Damage, false, true) > 0.f)
		{
			++HitCount;
			RegisterHit(Health->IsDead(), false, true);

			// Recul : le figurant vivant est repoussé (bagarre).
			ACharacter* HitCharacter = Cast<ACharacter>(Target);
			const AReshootEnemy* HitEnemy = Cast<AReshootEnemy>(Target);
			if (Knockback > 0.f && HitCharacter && !Health->IsDead() && !(HitEnemy && HitEnemy->IsFrozen()))
			{
				FVector Push = Target->GetActorLocation() - GetOwner()->GetActorLocation();
				Push.Z = 0.f;
				HitCharacter->LaunchCharacter(Push.GetSafeNormal() * Knockback + FVector(0.f, 0.f, Knockback * 0.35f), true, true);
			}
		}
	}

	if (bDrawDebug)
	{
		const FColor Color = HitCount > 0 ? FColor::Red : FColor::Yellow;
		DrawDebugCone(GetWorld(), Start, Direction, Range, FMath::DegreesToRadians(HalfAngle),
			FMath::DegreesToRadians(HalfAngle * 0.5f), 12, Color, false, 0.2f, 0, 1.f);
	}
	return HitCount;
}

// ---------------------------------------------------------------------------
// Poings

void AMedievalSword::Punch()
{
	if (!IsEnabled() || bShieldRaised || Mode != EMedievalWeaponMode::Fists)
	{
		return;
	}

	const double Now = GetWorld()->GetTimeSeconds();
	if (Now - LastPunchTime < (1.f / FistPunchesPerSecond) - 0.02)
	{
		return;
	}
	LastPunchTime = Now;

	// Gauche, droite, gauche… : le poing part en avant puis revient.
	UStaticMeshComponent* Fist = bNextPunchLeft ? FistLeftMesh : FistRightMesh;
	const FVector Rest = bNextPunchLeft ? FistLeftRestLocation : FistRightRestLocation;
	bNextPunchLeft = !bNextPunchLeft;
	Fist->SetRelativeLocation(Rest + FVector(FistPunchReach, Rest.Y < -15.f ? 12.f : -8.f, 10.f));
	GetWorldTimerManager().SetTimer(PunchPoseTimer, this, &AMedievalSword::ResetPunchPose, 0.1f, false);

	if (SwingSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, SwingSound, GetActorLocation(), 0.6f, 1.4f);
	}

	const int32 HitCount = MeleeSweep(FistRange, FistHalfAngle, FistDamage, FistKnockback);
	if (HitCount > 0 && SwordHitSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, SwordHitSound, GetActorLocation(), 0.8f, 0.7f);
	}
}

void AMedievalSword::ResetPunchPose()
{
	const bool bGuard = bShieldRaised && Mode == EMedievalWeaponMode::Fists;
	FistLeftMesh->SetRelativeLocation(bGuard ? FistLeftGuardLocation : FistLeftRestLocation);
	FistRightMesh->SetRelativeLocation(bGuard ? FistRightGuardLocation : FistRightRestLocation);
}

void AMedievalSword::ResetSwingPose()
{
	Mesh->SetRelativeTransform(bShieldRaised ? SwordGuardTransform : SwordRestTransform);
}

// ---------------------------------------------------------------------------
// Combos : chaque coup qui touche prolonge le combo ; tous les HitsPerMultiplierStep coups,
// le multiplicateur monte d'un cran. Une pause de ComboWindow secondes ou une blessure casse le combo.

int32 AMedievalSword::GetComboMultiplier() const
{
	return FMath::Clamp(1 + Combo / FMath::Max(HitsPerMultiplierStep, 1), 1, FMath::Max(MaxComboMultiplier, 1));
}

float AMedievalSword::GetComboTimeLeftPct() const
{
	if (Combo <= 0 || ComboWindow <= 0.f)
	{
		return 0.f;
	}
	return FMath::Clamp(GetWorldTimerManager().GetTimerRemaining(ComboTimer) / ComboWindow, 0.f, 1.f);
}

void AMedievalSword::RegisterHit(bool bKilled, bool bHeadshot, bool bMelee)
{
	++Combo;
	const int32 Multiplier = GetComboMultiplier();
	const int32 BasePoints = PointsPerHit + (bKilled ? PointsPerKill : 0) + (bHeadshot ? HeadshotBonusPoints : 0);
	const int32 Gained = BasePoints * Multiplier;
	ComboScore += Gained;

	GetWorldTimerManager().SetTimer(ComboTimer, this, &AMedievalSword::ResetCombo, ComboWindow, false);
	if (!IsEnabled())
	{
		GetWorldTimerManager().PauseTimer(ComboTimer);
	}

	UE_LOG(LogReshoot, Verbose, TEXT("Medieval combo %d (x%d) +%d -> %d"), Combo, Multiplier, Gained, ComboScore);
	OnComboChanged.Broadcast(Combo, Multiplier, ComboScore, Gained);
	ShowComboOnScreen(Gained);
}

void AMedievalSword::ResetCombo()
{
	GetWorldTimerManager().ClearTimer(ComboTimer);
	if (Combo == 0)
	{
		return;
	}
	Combo = 0;
	OnComboChanged.Broadcast(0, 1, ComboScore, 0);
	ShowComboOnScreen(0);
}

void AMedievalSword::HandleOwnerHealthChanged(UHealthComponent* HealthComponent, float NewHealth, float Delta)
{
	if (bDamageBreaksCombo && Delta < 0.f)
	{
		ResetCombo();
	}
}

void AMedievalSword::ShowComboOnScreen(int32 PointsGained) const
{
	if (!bShowComboOnScreen || !GEngine)
	{
		return;
	}
	// Clés fixes : les lignes se mettent à jour au lieu de s'empiler.
	const uint64 ScoreKey = 0x4D454449;
	const uint64 ComboKey = ScoreKey + 1;
	GEngine->AddOnScreenDebugMessage(ScoreKey, 600.f, FColor(255, 215, 90), FString::Printf(TEXT("SCORE  %d"), ComboScore), true, FVector2D(1.6f, 1.6f));
	if (Combo > 0)
	{
		const int32 Multiplier = GetComboMultiplier();
		const FColor Color = Multiplier >= 4 ? FColor(255, 70, 40) : (Multiplier >= 2 ? FColor(255, 160, 40) : FColor::White);
		GEngine->AddOnScreenDebugMessage(ComboKey, ComboWindow, Color,
			FString::Printf(TEXT("COMBO %d   x%d   +%d"), Combo, Multiplier, PointsGained), true, FVector2D(1.4f, 1.4f));
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(ComboKey, 1.f, FColor(150, 150, 150), TEXT("combo perdu"), true, FVector2D(1.2f, 1.2f));
	}
}

// ---------------------------------------------------------------------------
// Bouclier

void AMedievalSword::RaiseShield(bool bRaise)
{
	// Garde : bouclier en mode épée (interdit par « Sans le bouclier ! »), poings levés en mode poings.
	const bool bFistGuard = (Mode == EMedievalWeaponMode::Fists);
	if (bRaise && (!IsEnabled() || Mode == EMedievalWeaponMode::Bow || (!bFistGuard && !bShieldAllowed)))
	{
		return;
	}
	if (bRaise == bShieldRaised)
	{
		return;
	}
	bShieldRaised = bRaise;

	AReshootCharacter* Character = GetOwnerCharacter();
	if (UHealthComponent* Health = Character ? Character->GetHealth() : nullptr)
	{
		Health->SetFrontalBlock(bRaise, bFistGuard ? FistGuardDamageMultiplier : ShieldDamageMultiplier);
	}

	if (bRaise)
	{
		// Bouclier levé = on ne frappe plus.
		GetWorldTimerManager().ClearTimer(SwingRepeatTimer);
		if (ShieldUpSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, ShieldUpSound, GetActorLocation());
		}
		// Pas de sprint : le joueur remet sa vitesse de sprint à chaque appui sur Maj, on la rabaisse en continu.
		EnforceShieldSpeed();
		GetWorldTimerManager().SetTimer(ShieldSpeedTimer, this, &AMedievalSword::EnforceShieldSpeed, 0.05f, true);
	}
	else
	{
		GetWorldTimerManager().ClearTimer(ShieldSpeedTimer);
		if (Character)
		{
			// Rend la vitesse normale (le joueur la recalculera au prochain changement de sprint/accroupi).
			UCharacterMovementComponent* Move = Character->GetCharacterMovement();
			Move->MaxWalkSpeed = Character->IsSprinting() ? Character->SprintSpeed : (Character->bIsCrouched ? Character->CrouchSpeed : Character->WalkSpeed);
		}
		// Clic gauche toujours enfoncé : reprend les coups.
		if (bPrimaryHeld && IsEnabled() && Mode == EMedievalWeaponMode::Sword && bSwordAutoRepeat)
		{
			GetWorldTimerManager().SetTimer(SwingRepeatTimer, this, &AMedievalSword::SwordSwing, 1.f / SwordAttacksPerSecond, true, 0.05f);
		}
		else if (bPrimaryHeld && IsEnabled() && Mode == EMedievalWeaponMode::Fists)
		{
			GetWorldTimerManager().SetTimer(SwingRepeatTimer, this, &AMedievalSword::Punch, 1.f / FistPunchesPerSecond, true, 0.05f);
		}
	}

	UpdateVisuals();
}

void AMedievalSword::EnforceShieldSpeed()
{
	if (AReshootCharacter* Character = GetOwnerCharacter())
	{
		UCharacterMovementComponent* Move = Character->GetCharacterMovement();
		Move->MaxWalkSpeed = FMath::Min(Move->MaxWalkSpeed, ShieldMaxWalkSpeed);
	}
}

// ---------------------------------------------------------------------------
// Arc

float AMedievalSword::GetBowCharge() const
{
	if (!bCharging)
	{
		return 0.f;
	}
	return FMath::Clamp(static_cast<float>(GetWorld()->GetTimeSeconds() - ChargeStartTime) / BowFullChargeTime, 0.f, 1.f);
}

void AMedievalSword::StartBowCharge()
{
	if (!IsEnabled() || !bBowAllowed)
	{
		return;
	}
	const double Now = GetWorld()->GetTimeSeconds();
	if (Now - LastArrowTime < BowCooldown)
	{
		return;
	}
	bCharging = true;
	ChargeStartTime = Now;
	BowMesh->SetRelativeTransform(BowDrawTransform);
}

void AMedievalSword::ReleaseBow()
{
	const float Charge = GetBowCharge();
	bCharging = false;
	BowMesh->SetRelativeTransform(BowRestTransform);

	if (!IsEnabled() || !ArrowClass)
	{
		return;
	}
	LastArrowTime = GetWorld()->GetTimeSeconds();

	FVector Start, Direction;
	GetAim(Start, Direction);

	// La flèche part un peu devant la caméra pour ne pas toucher le joueur.
	const FVector SpawnLocation = Start + Direction * 60.f;
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = GetOwner();
	SpawnParams.Instigator = Cast<APawn>(GetOwner());
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AMedievalArrow* Arrow = GetWorld()->SpawnActor<AMedievalArrow>(ArrowClass, SpawnLocation, Direction.Rotation(), SpawnParams);
	if (!Arrow)
	{
		return;
	}

	const float Damage = FMath::Lerp(BowMinDamage, BowMaxDamage, Charge);
	const float Speed = FMath::Lerp(BowMinArrowSpeed, BowMaxArrowSpeed, Charge);
	Arrow->Launch(Direction, Speed, Damage, BowArrowGravityScale);

	if (BowReleaseSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, BowReleaseSound, GetActorLocation());
	}
}

// ---------------------------------------------------------------------------
// Entrées de changement de mode (1, 2, molette). Le joueur ne les fournit pas :
// l'arme ajoute son propre contexte Enhanced Input tant qu'elle existe.

void AMedievalSword::TrySetupInput()
{
	if (bInputReady)
	{
		return;
	}

	const AReshootCharacter* Character = GetOwnerCharacter();
	APlayerController* PC = Character ? Cast<APlayerController>(Character->GetController()) : nullptr;
	ULocalPlayer* LocalPlayer = PC ? PC->GetLocalPlayer() : nullptr;
	UEnhancedInputLocalPlayerSubsystem* Subsystem = LocalPlayer ? ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer) : nullptr;

	if (!Subsystem)
	{
		// L'arme peut apparaître avant que le joueur soit possédé : on réessaie.
		GetWorldTimerManager().SetTimer(InputSetupTimer, this, &AMedievalSword::TrySetupInput, 0.2f, false);
		return;
	}

	if (!MedievalMappingContext)
	{
		CreateDefaultInput();
	}

	EnableInput(PC);
	UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(InputComponent);
	if (!Input)
	{
		UE_LOG(LogReshoot, Error, TEXT("AMedievalSword: InputComponent is not an EnhancedInputComponent."));
		return;
	}

	if (SelectSwordAction)
	{
		Input->BindAction(SelectSwordAction, ETriggerEvent::Started, this, &AMedievalSword::OnSelectSword);
	}
	if (SelectBowAction)
	{
		Input->BindAction(SelectBowAction, ETriggerEvent::Started, this, &AMedievalSword::OnSelectBow);
	}
	if (SelectFistsAction)
	{
		Input->BindAction(SelectFistsAction, ETriggerEvent::Started, this, &AMedievalSword::OnSelectFists);
	}
	if (ToggleModeAction)
	{
		Input->BindAction(ToggleModeAction, ETriggerEvent::Started, this, &AMedievalSword::OnToggleMode);
	}

	Subsystem->AddMappingContext(MedievalMappingContext, 1);
	InputController = PC;
	bInputReady = true;
}

void AMedievalSword::CreateDefaultInput()
{
	auto MakeAction = [this](const TCHAR* Name)
	{
		UInputAction* Action = NewObject<UInputAction>(this, Name);
		Action->ValueType = EInputActionValueType::Boolean;
		return Action;
	};

	SelectSwordAction = MakeAction(TEXT("IA_MedievalSword_Runtime"));
	SelectBowAction = MakeAction(TEXT("IA_MedievalBow_Runtime"));
	SelectFistsAction = MakeAction(TEXT("IA_MedievalFists_Runtime"));
	ToggleModeAction = MakeAction(TEXT("IA_MedievalToggle_Runtime"));

	MedievalMappingContext = NewObject<UInputMappingContext>(this, TEXT("IMC_Medieval_Runtime"));
	MedievalMappingContext->MapKey(SelectSwordAction, EKeys::One);
	MedievalMappingContext->MapKey(SelectBowAction, EKeys::Two);
	MedievalMappingContext->MapKey(SelectFistsAction, EKeys::Three);
	MedievalMappingContext->MapKey(ToggleModeAction, EKeys::MouseScrollUp);
	MedievalMappingContext->MapKey(ToggleModeAction, EKeys::MouseScrollDown);
}

void AMedievalSword::OnSelectSword()
{
	if (IsEnabled())
	{
		SetMode(EMedievalWeaponMode::Sword);
	}
}

void AMedievalSword::OnSelectBow()
{
	if (IsEnabled())
	{
		SetMode(EMedievalWeaponMode::Bow);
	}
}

void AMedievalSword::OnSelectFists()
{
	if (IsEnabled())
	{
		SetMode(EMedievalWeaponMode::Fists);
	}
}

void AMedievalSword::OnToggleMode()
{
	if (IsEnabled())
	{
		ToggleMode();
	}
}
