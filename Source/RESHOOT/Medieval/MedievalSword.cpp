#include "Medieval/MedievalSword.h"
#include "Medieval/MedievalArrow.h"
#include "Core/ReshootCharacter.h"
#include "Core/ReshootEnemy.h"
#include "Core/HealthComponent.h"
#include "RESHOOT.h"

#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/OverlapResult.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	// Arc en boîte grise : tenu à gauche, incliné, partiellement hors champ (relatif à la racine de l'arme).
	const FVector BowRestLocation(20.f, -37.f, 5.f);
	const FRotator BowRestRotation(0.f, 0.f, -30.f);
	const FVector BowDrawLocation(10.f, -32.f, 5.f);
	const FRotator BowDrawRotation(0.f, 0.f, -40.f);
	const FVector BowScale(0.03f, 0.03f, 0.5f);
}

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
}

void AMedievalSword::BeginPlay()
{
	Super::BeginPlay();
	SwordRestTransform = Mesh->GetRelativeTransform();
	BowMesh->SetRelativeLocationAndRotation(BowRestLocation, BowRestRotation);
	BowMesh->SetRelativeScale3D(BowScale);
	UpdateVisuals();
	TrySetupInput();
}

void AMedievalSword::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	RaiseShield(false);
	GetWorldTimerManager().ClearAllTimersForObject(this);

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
	if (IsEnabled() && Mode == EMedievalWeaponMode::Sword)
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
	DisplayName = (Mode == EMedievalWeaponMode::Sword)
		? NSLOCTEXT("Reshoot", "MedievalSwordName", "Épée et bouclier")
		: NSLOCTEXT("Reshoot", "MedievalBowName", "Arc");

	UpdateVisuals();
	OnModeChanged.Broadcast(Mode);

	// Clic droit toujours enfoncé en revenant à l'épée : le bouclier se relève.
	if (Mode == EMedievalWeaponMode::Sword && bSecondaryHeld && IsEnabled())
	{
		RaiseShield(true);
	}
}

void AMedievalSword::ToggleMode()
{
	SetMode(Mode == EMedievalWeaponMode::Sword ? EMedievalWeaponMode::Bow : EMedievalWeaponMode::Sword);
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
	Mesh->SetVisibility(bSword);
	ShieldMesh->SetVisibility(bSword && bShieldAllowed);
	BowMesh->SetVisibility(!bSword);

	ShieldMesh->SetRelativeLocation(bShieldRaised ? FVector(20.f, -5.f, -5.f) : FVector(0.f, -35.f, -15.f));
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

	// Tous les pions dans la sphère de portée, puis filtre par angle.
	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(MedievalSwordSwing), false, this);
	Params.AddIgnoredActor(GetOwner());
	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);
	ObjectParams.AddObjectTypesToQuery(ECC_PhysicsBody);
	GetWorld()->OverlapMultiByObjectType(Overlaps, Start, FQuat::Identity, ObjectParams, FCollisionShape::MakeSphere(SwordRange), Params);

	const float CosHalfAngle = FMath::Cos(FMath::DegreesToRadians(SwordHalfAngle));
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
		FVector ToTarget = Closest - Start;
		const float Distance = ToTarget.Size();
		if (Distance > SwordRange)
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
		FCollisionQueryParams SightParams(SCENE_QUERY_STAT(MedievalSwordSight), false, this);
		SightParams.AddIgnoredActor(GetOwner());
		SightParams.AddIgnoredActor(Target);
		if (GetWorld()->LineTraceSingleByObjectType(WallHit, Start, Closest, FCollisionObjectQueryParams(ECC_WorldStatic), SightParams))
		{
			continue;
		}

		if (DealDamage(Target, SwordDamage, false, true) > 0.f)
		{
			++HitCount;
		}
	}

	if (HitCount > 0 && SwordHitSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, SwordHitSound, Start + Direction * SwordRange * 0.5f);
	}

	if (bDrawDebug)
	{
		const FColor Color = HitCount > 0 ? FColor::Red : FColor::Yellow;
		DrawDebugCone(GetWorld(), Start, Direction, SwordRange, FMath::DegreesToRadians(SwordHalfAngle),
			FMath::DegreesToRadians(SwordHalfAngle * 0.5f), 12, Color, false, 0.2f, 0, 1.f);
	}
}

void AMedievalSword::ResetSwingPose()
{
	Mesh->SetRelativeTransform(SwordRestTransform);
}

// ---------------------------------------------------------------------------
// Bouclier

void AMedievalSword::RaiseShield(bool bRaise)
{
	if (bRaise && (!bShieldAllowed || !IsEnabled()))
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
		Health->SetFrontalBlock(bRaise, ShieldDamageMultiplier);
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
	BowMesh->SetRelativeLocationAndRotation(BowDrawLocation, BowDrawRotation);
}

void AMedievalSword::ReleaseBow()
{
	const float Charge = GetBowCharge();
	bCharging = false;
	BowMesh->SetRelativeLocationAndRotation(BowRestLocation, BowRestRotation);

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
	ToggleModeAction = MakeAction(TEXT("IA_MedievalToggle_Runtime"));

	MedievalMappingContext = NewObject<UInputMappingContext>(this, TEXT("IMC_Medieval_Runtime"));
	MedievalMappingContext->MapKey(SelectSwordAction, EKeys::One);
	MedievalMappingContext->MapKey(SelectBowAction, EKeys::Two);
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

void AMedievalSword::OnToggleMode()
{
	if (IsEnabled())
	{
		ToggleMode();
	}
}
