#include "Core/ReshootCharacter.h"
#include "Core/HealthComponent.h"
#include "Core/ReshootWeapon.h"
#include "Core/UniverseDefinition.h"
#include "Director/ShootDirectorSubsystem.h"
#include "RESHOOT.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "TimerManager.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "InputActionValue.h"

AReshootCharacter::AReshootCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = false;

	GetCapsuleComponent()->InitCapsuleSize(40.f, 92.f);

	FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCamera->SetupAttachment(GetCapsuleComponent());
	FirstPersonCamera->SetRelativeLocation(FVector(0.f, 0.f, 64.f));
	FirstPersonCamera->bUsePawnControlRotation = true;

	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));

	bUseControllerRotationYaw = true;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;

	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->bOrientRotationToMovement = false;
	Move->GetNavAgentPropertiesRef().bCanCrouch = true;
	Move->GetNavAgentPropertiesRef().bCanFly = true;
	Move->bCanWalkOffLedgesWhenCrouching = true;
	Move->SetCrouchedHalfHeight(50.f);
}

// ---------------------------------------------------------------------------
// Cycle de vie
// ---------------------------------------------------------------------------

void AReshootCharacter::BeginPlay()
{
	Super::BeginPlay();

	ApplyBaseMovementSettings();
	FirstPersonCamera->SetFieldOfView(DefaultFOV);
	TargetAimFOV = DefaultFOV;
	HealthComponent->OnDeath.AddDynamic(this, &AReshootCharacter::HandleDeath);

	// Le réalisateur peut avoir lancé la scène avant l'apparition du joueur : on reprend son univers.
	EUniverse InitialUniverse = StartingUniverse;
	if (InitialUniverse == EUniverse::None)
	{
		if (const UShootDirectorSubsystem* Director = GetWorld()->GetSubsystem<UShootDirectorSubsystem>())
		{
			InitialUniverse = Director->GetCurrentUniverse();
		}
	}
	SetUniverse(InitialUniverse);
}

void AReshootCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(SlideTimer);
	GetWorldTimerManager().ClearTimer(AimFOVTimer);
	if (CurrentWeapon)
	{
		CurrentWeapon->Destroy();
		CurrentWeapon = nullptr;
	}
	Super::EndPlay(EndPlayReason);
}

void AReshootCharacter::HandleDeath(UHealthComponent* DeadComponent, AActor* Killer, bool bHeadshot, bool bMelee)
{
	if (UShootDirectorSubsystem* Director = GetWorld()->GetSubsystem<UShootDirectorSubsystem>())
	{
		Director->NotifyPlayerDeath();
	}
}

// ---------------------------------------------------------------------------
// Univers et arme
// ---------------------------------------------------------------------------

const UUniverseDefinition* AReshootCharacter::FindUniverseDefinition(EUniverse Universe) const
{
	if (Universe == EUniverse::None)
	{
		return nullptr;
	}
	if (const UShootDirectorSubsystem* Director = GetWorld()->GetSubsystem<UShootDirectorSubsystem>())
	{
		return Director->GetUniverseDefinition(Universe);
	}
	return nullptr;
}

void AReshootCharacter::SetUniverse(EUniverse NewUniverse)
{
	const UUniverseDefinition* Definition = FindUniverseDefinition(NewUniverse);
	CurrentUniverse = NewUniverse;
	bSpaceGravityOn = false;
	ResetAimFOV();

	ApplyUniverseMovement(Definition, NewUniverse);

	TSubclassOf<AReshootWeapon> WeaponClass = (Definition && Definition->WeaponClass) ? Definition->WeaponClass : DefaultWeaponClass;
	EquipWeapon(WeaponClass);

	UE_LOG(LogReshoot, Log, TEXT("Player universe -> %s (definition: %s)"),
		*UEnum::GetValueAsString(NewUniverse), *GetNameSafe(Definition));
	OnUniverseChanged.Broadcast(NewUniverse);
}

void AReshootCharacter::DebugUniverse(int32 UniverseIndex)
{
	const int32 Clamped = FMath::Clamp(UniverseIndex, 0, static_cast<int32>(EUniverse::Noir));
	SetUniverse(static_cast<EUniverse>(Clamped));
}

AReshootWeapon* AReshootCharacter::EquipWeapon(TSubclassOf<AReshootWeapon> WeaponClass)
{
	if (CurrentWeapon)
	{
		CurrentWeapon->OnWeaponEvent.RemoveDynamic(this, &AReshootCharacter::HandleWeaponEvent);
		CurrentWeapon->Destroy();
		CurrentWeapon = nullptr;
	}
	ResetAimFOV();

	if (WeaponClass)
	{
		FActorSpawnParameters Params;
		Params.Owner = this;
		Params.Instigator = this;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		CurrentWeapon = GetWorld()->SpawnActor<AReshootWeapon>(WeaponClass, GetActorTransform(), Params);
		if (CurrentWeapon)
		{
			CurrentWeapon->AttachToComponent(FirstPersonCamera, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
			CurrentWeapon->SetActorRelativeTransform(WeaponOffset);
			CurrentWeapon->OnWeaponEvent.AddDynamic(this, &AReshootCharacter::HandleWeaponEvent);
			CurrentWeapon->SetEnabled(!bFrozen);
			if (!IsWeaponActionAllowed(EWeaponAction::SwitchMode))
			{
				CurrentWeapon->SelectMode(0);
			}
		}
	}

	UpdateMaxSpeeds();
	OnWeaponChanged.Broadcast(CurrentWeapon);
	return CurrentWeapon;
}

void AReshootCharacter::HandleWeaponEvent(AReshootWeapon* Weapon, EWeaponEvent Event)
{
	if (Event == EWeaponEvent::SecondaryStarted || Event == EWeaponEvent::SecondaryStopped)
	{
		UpdateMaxSpeeds(); // Bouclier levé : pas de sprint.
	}
	OnPlayerWeaponEvent.Broadcast(Weapon, Event);
}

void AReshootCharacter::ResetForTake(const FTransform& StartTransform)
{
	EndSlide();
	UnCrouch();
	SetActorLocationAndRotation(StartTransform.GetLocation(), StartTransform.Rotator(), false, nullptr, ETeleportType::TeleportPhysics);
	if (Controller)
	{
		Controller->SetControlRotation(StartTransform.Rotator());
	}
	GetCharacterMovement()->StopMovementImmediately();
	HealthComponent->ResetHealth();
	ResetAimFOV();
	if (CurrentWeapon)
	{
		CurrentWeapon->StopPrimary();
		CurrentWeapon->StopSecondary();
		CurrentWeapon->Refill();
	}
}

// ---------------------------------------------------------------------------
// Restrictions du réalisateur
// ---------------------------------------------------------------------------

void AReshootCharacter::SetWeaponActionAllowed(EWeaponAction Action, bool bAllowed)
{
	const uint8 Bit = 1 << static_cast<uint8>(Action);
	BlockedWeaponActions = bAllowed ? (BlockedWeaponActions & ~Bit) : (BlockedWeaponActions | Bit);

	if (bAllowed || !CurrentWeapon)
	{
		return;
	}
	switch (Action)
	{
	case EWeaponAction::Primary:
		CurrentWeapon->StopPrimary();
		break;
	case EWeaponAction::Secondary:
		CurrentWeapon->StopSecondary();
		break;
	case EWeaponAction::SwitchMode:
		CurrentWeapon->SelectMode(0);
		break;
	default:
		break;
	}
}

bool AReshootCharacter::IsWeaponActionAllowed(EWeaponAction Action) const
{
	return (BlockedWeaponActions & (1 << static_cast<uint8>(Action))) == 0;
}

void AReshootCharacter::SetWeaponCostMultiplier(float Multiplier)
{
	WeaponCostMultiplier = FMath::Max(Multiplier, 0.f);
}

void AReshootCharacter::ClearWeaponRestrictions()
{
	BlockedWeaponActions = 0;
	WeaponCostMultiplier = 1.f;
}

// ---------------------------------------------------------------------------
// Visée
// ---------------------------------------------------------------------------

void AReshootCharacter::SetAimFOV(float TargetFOV)
{
	TargetAimFOV = FMath::Clamp(TargetFOV, 30.f, 140.f);
	if (!GetWorld())
	{
		return;
	}
	if (!AimFOVTimer.IsValid() || !GetWorldTimerManager().IsTimerActive(AimFOVTimer))
	{
		GetWorldTimerManager().SetTimer(AimFOVTimer, this, &AReshootCharacter::UpdateAimFOV, 1.f / 60.f, true);
	}
}

void AReshootCharacter::ResetAimFOV()
{
	SetAimFOV(DefaultFOV);
}

void AReshootCharacter::UpdateAimFOV()
{
	const float Current = FirstPersonCamera->FieldOfView;
	const float Next = FMath::FInterpTo(Current, TargetAimFOV, 1.f / 60.f, AimFOVInterpSpeed);
	if (FMath::IsNearlyEqual(Next, TargetAimFOV, 0.05f))
	{
		FirstPersonCamera->SetFieldOfView(TargetAimFOV);
		GetWorldTimerManager().ClearTimer(AimFOVTimer);
		return;
	}
	FirstPersonCamera->SetFieldOfView(Next);
}

// ---------------------------------------------------------------------------
// Gravité temporaire en apesanteur (événement spatial)
// ---------------------------------------------------------------------------

void AReshootCharacter::SetSpaceGravity(bool bGravityOn)
{
	if (CurrentUniverse != EUniverse::Space || bSpaceGravityOn == bGravityOn)
	{
		return;
	}
	bSpaceGravityOn = bGravityOn;

	UCharacterMovementComponent* Move = GetCharacterMovement();
	if (bGravityOn)
	{
		Move->GravityScale = 1.f;
		Move->MaxAcceleration = MaxAcceleration;
		Move->BrakingDecelerationWalking = DefaultWalkingBraking;
		CurrentWalkingBraking = DefaultWalkingBraking;
		Move->DefaultLandMovementMode = MOVE_Walking;
		Move->SetMovementMode(MOVE_Falling);
	}
	else
	{
		ApplyUniverseMovement(FindUniverseDefinition(EUniverse::Space), EUniverse::Space);
	}
	UpdateMaxSpeeds();
}

void AReshootCharacter::SetFrozen(bool bInFrozen)
{
	bFrozen = bInFrozen;
	if (bFrozen)
	{
		EndSlide();
		GetCharacterMovement()->StopMovementImmediately();
	}
	if (CurrentWeapon)
	{
		CurrentWeapon->SetEnabled(!bFrozen);
	}
}

// ---------------------------------------------------------------------------
// Réglages de déplacement
// ---------------------------------------------------------------------------

void AReshootCharacter::ApplyBaseMovementSettings()
{
	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->MaxWalkSpeed = WalkSpeed;
	Move->MaxWalkSpeedCrouched = CrouchSpeed;
	Move->MaxFlySpeed = WalkSpeed;
	Move->JumpZVelocity = JumpVelocity;
	Move->MaxAcceleration = MaxAcceleration;
	Move->GroundFriction = GroundFriction;
	Move->AirControl = AirControl;
	Move->BrakingDecelerationWalking = DefaultWalkingBraking;
	Move->BrakingDecelerationFalling = 0.f; // Conserve l'élan en l'air.
	Move->FallingLateralFriction = 0.f;
	CurrentWalkingBraking = DefaultWalkingBraking;
	// Pas de dégâts de chute : rien n'est branché sur Landed(), c'est volontaire.
}

void AReshootCharacter::ApplyUniverseMovement(const UUniverseDefinition* Definition, EUniverse Universe)
{
	UCharacterMovementComponent* Move = GetCharacterMovement();

	const bool bSpace = Universe == EUniverse::Space;
	const EMovementMode Mode = Definition ? Definition->MovementMode.GetValue() : (bSpace ? MOVE_Flying : MOVE_Walking);
	const float Gravity = Definition ? Definition->GravityScale : (bSpace ? 0.f : 1.f);

	Move->GravityScale = Gravity;

	if (Mode == MOVE_Flying)
	{
		EndSlide();
		UnCrouch();
		Move->BrakingDecelerationFlying = Definition ? Definition->BrakingDeceleration : DefaultSpaceBraking;
		Move->MaxAcceleration = SpaceAcceleration;
		Move->DefaultLandMovementMode = MOVE_Flying;
		Move->SetMovementMode(MOVE_Flying);
	}
	else
	{
		CurrentWalkingBraking = Definition ? Definition->BrakingDeceleration : DefaultWalkingBraking;
		Move->BrakingDecelerationWalking = CurrentWalkingBraking;
		Move->MaxAcceleration = MaxAcceleration;
		Move->DefaultLandMovementMode = MOVE_Walking;
		if (Move->MovementMode == MOVE_Flying || Move->MovementMode == MOVE_None)
		{
			// Le composant retrouvera le sol tout seul.
			Move->SetMovementMode(MOVE_Falling);
		}
	}

	UpdateMaxSpeeds();
}

bool AReshootCharacter::IsInSpaceMode() const
{
	return GetCharacterMovement()->MovementMode == MOVE_Flying;
}

void AReshootCharacter::UpdateMaxSpeeds()
{
	UCharacterMovementComponent* Move = GetCharacterMovement();
	const bool bWeaponBlocksSprint = CurrentWeapon && CurrentWeapon->PreventsSprint();
	const bool bSprint = bSprintHeld && !bIsCrouched && !bWeaponBlocksSprint;
	Move->MaxWalkSpeed = bSprint ? SprintSpeed : WalkSpeed;
	Move->MaxFlySpeed = (bSprintHeld && !bWeaponBlocksSprint) ? SprintSpeed : WalkSpeed; // Sprint = boost en apesanteur.
}

bool AReshootCharacter::IsSprinting() const
{
	const bool bWeaponBlocksSprint = CurrentWeapon && CurrentWeapon->PreventsSprint();
	return bSprintHeld && !bIsCrouched && !bSliding && !bWeaponBlocksSprint && GetHorizontalSpeed() > WalkSpeed * 0.5f;
}

bool AReshootCharacter::IsInAir() const
{
	return GetCharacterMovement()->MovementMode == MOVE_Falling;
}

bool AReshootCharacter::IsMoving() const
{
	return GetVelocity().SizeSquared() > FMath::Square(MovingSpeedThreshold);
}

float AReshootCharacter::GetHorizontalSpeed() const
{
	return GetVelocity().Size2D();
}

bool AReshootCharacter::CanJumpInternal_Implementation() const
{
	// Le moteur interdit le saut accroupi ; on l'autorise (saut en sortie de glissade).
	// JumpIsAllowedInternal vérifie bWantsToCrouch, qu'on remet à faux avant de sauter.
	return JumpIsAllowedInternal();
}

// ---------------------------------------------------------------------------
// Glissade
// ---------------------------------------------------------------------------

bool AReshootCharacter::CanStartSlide() const
{
	const UCharacterMovementComponent* Move = GetCharacterMovement();
	const float Now = GetWorld()->GetTimeSeconds();
	return !bSliding
		&& IsSprinting()
		&& Move->IsMovingOnGround()
		&& GetHorizontalSpeed() >= SlideMinStartSpeed
		&& Now - LastSlideEndTime >= SlideCooldown;
}

void AReshootCharacter::StartSlide()
{
	UCharacterMovementComponent* Move = GetCharacterMovement();

	bSliding = true;
	SlideStartTime = GetWorld()->GetTimeSeconds();

	Crouch();
	Move->GroundFriction = SlideGroundFriction;
	Move->BrakingDecelerationWalking = SlideBrakingDeceleration;

	const FVector Direction = FVector(Move->Velocity.X, Move->Velocity.Y, 0.f).GetSafeNormal();
	Move->Velocity += Direction * SlideImpulse;

	GetWorldTimerManager().SetTimer(SlideTimer, this, &AReshootCharacter::UpdateSlide, 0.05f, true);
}

void AReshootCharacter::UpdateSlide()
{
	if (!bSliding)
	{
		GetWorldTimerManager().ClearTimer(SlideTimer);
		return;
	}

	const float Elapsed = GetWorld()->GetTimeSeconds() - SlideStartTime;
	const bool bTooSlow = GetCharacterMovement()->IsMovingOnGround() && GetHorizontalSpeed() < SlideEndSpeed;
	if (bTooSlow || Elapsed >= SlideMaxDuration)
	{
		EndSlide();
		if (!bCrouchHeld)
		{
			UnCrouch();
		}
	}
}

void AReshootCharacter::EndSlide()
{
	if (!bSliding)
	{
		return;
	}
	bSliding = false;
	LastSlideEndTime = GetWorld()->GetTimeSeconds();
	GetWorldTimerManager().ClearTimer(SlideTimer);

	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->GroundFriction = GroundFriction;
	Move->BrakingDecelerationWalking = CurrentWalkingBraking;
}

// ---------------------------------------------------------------------------
// Entrées
// ---------------------------------------------------------------------------

void AReshootCharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();
	AddMappingContext();
}

void AReshootCharacter::AddMappingContext()
{
	EnsureDefaultInput();

	const APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC || !DefaultMappingContext)
	{
		return;
	}
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
	{
		Subsystem->AddMappingContext(DefaultMappingContext, 0);
	}
}

void AReshootCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	EnsureDefaultInput();

	UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!Input)
	{
		UE_LOG(LogReshoot, Error, TEXT("AReshootCharacter needs an EnhancedInputComponent (check DefaultInput.ini)."));
		return;
	}

	if (MoveAction)
	{
		Input->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AReshootCharacter::OnMove);
	}
	if (LookAction)
	{
		Input->BindAction(LookAction, ETriggerEvent::Triggered, this, &AReshootCharacter::OnLook);
	}
	if (JumpAction)
	{
		Input->BindAction(JumpAction, ETriggerEvent::Started, this, &AReshootCharacter::OnJumpStarted);
		Input->BindAction(JumpAction, ETriggerEvent::Triggered, this, &AReshootCharacter::OnJumpTriggered);
		Input->BindAction(JumpAction, ETriggerEvent::Completed, this, &AReshootCharacter::OnJumpCompleted);
	}
	if (CrouchAction)
	{
		Input->BindAction(CrouchAction, ETriggerEvent::Started, this, &AReshootCharacter::OnCrouchStarted);
		Input->BindAction(CrouchAction, ETriggerEvent::Triggered, this, &AReshootCharacter::OnCrouchTriggered);
		Input->BindAction(CrouchAction, ETriggerEvent::Completed, this, &AReshootCharacter::OnCrouchCompleted);
	}
	if (SprintAction)
	{
		Input->BindAction(SprintAction, ETriggerEvent::Started, this, &AReshootCharacter::OnSprintStarted);
		Input->BindAction(SprintAction, ETriggerEvent::Completed, this, &AReshootCharacter::OnSprintCompleted);
	}
	if (PrimaryAction)
	{
		Input->BindAction(PrimaryAction, ETriggerEvent::Started, this, &AReshootCharacter::OnPrimaryStarted);
		Input->BindAction(PrimaryAction, ETriggerEvent::Completed, this, &AReshootCharacter::OnPrimaryCompleted);
	}
	if (SecondaryAction)
	{
		Input->BindAction(SecondaryAction, ETriggerEvent::Started, this, &AReshootCharacter::OnSecondaryStarted);
		Input->BindAction(SecondaryAction, ETriggerEvent::Completed, this, &AReshootCharacter::OnSecondaryCompleted);
	}
	if (ReloadAction)
	{
		Input->BindAction(ReloadAction, ETriggerEvent::Started, this, &AReshootCharacter::OnReload);
	}
	if (Mode1Action)
	{
		Input->BindAction(Mode1Action, ETriggerEvent::Started, this, &AReshootCharacter::OnMode1);
	}
	if (Mode2Action)
	{
		Input->BindAction(Mode2Action, ETriggerEvent::Started, this, &AReshootCharacter::OnMode2);
	}
	if (CycleModeAction)
	{
		Input->BindAction(CycleModeAction, ETriggerEvent::Triggered, this, &AReshootCharacter::OnCycleMode);
	}
}

void AReshootCharacter::EnsureDefaultInput()
{
	if (DefaultMappingContext)
	{
		return;
	}

	auto MakeAction = [this](const TCHAR* Name, EInputActionValueType Type)
	{
		UInputAction* Action = NewObject<UInputAction>(this, Name);
		Action->ValueType = Type;
		return Action;
	};

	MoveAction = MakeAction(TEXT("IA_Move_Runtime"), EInputActionValueType::Axis2D);
	LookAction = MakeAction(TEXT("IA_Look_Runtime"), EInputActionValueType::Axis2D);
	JumpAction = MakeAction(TEXT("IA_Jump_Runtime"), EInputActionValueType::Boolean);
	CrouchAction = MakeAction(TEXT("IA_Crouch_Runtime"), EInputActionValueType::Boolean);
	SprintAction = MakeAction(TEXT("IA_Sprint_Runtime"), EInputActionValueType::Boolean);
	PrimaryAction = MakeAction(TEXT("IA_Primary_Runtime"), EInputActionValueType::Boolean);
	SecondaryAction = MakeAction(TEXT("IA_Secondary_Runtime"), EInputActionValueType::Boolean);
	ReloadAction = MakeAction(TEXT("IA_Reload_Runtime"), EInputActionValueType::Boolean);
	Mode1Action = MakeAction(TEXT("IA_Mode1_Runtime"), EInputActionValueType::Boolean);
	Mode2Action = MakeAction(TEXT("IA_Mode2_Runtime"), EInputActionValueType::Boolean);
	CycleModeAction = MakeAction(TEXT("IA_CycleMode_Runtime"), EInputActionValueType::Axis1D);

	UInputMappingContext* Context = NewObject<UInputMappingContext>(this, TEXT("IMC_Default_Runtime"));

	// Déplacement 2D : X = droite, Y = avant. ZQSD (AZERTY) et WASD (QWERTY).
	auto MapMove = [this, Context](const FKey& Key, bool bSwizzle, bool bNegate)
	{
		FEnhancedActionKeyMapping& Mapping = Context->MapKey(MoveAction, Key);
		if (bSwizzle)
		{
			Mapping.Modifiers.Add(NewObject<UInputModifierSwizzleAxis>(Context));
		}
		if (bNegate)
		{
			Mapping.Modifiers.Add(NewObject<UInputModifierNegate>(Context));
		}
	};
	MapMove(EKeys::Z, true, false);
	MapMove(EKeys::W, true, false);
	MapMove(EKeys::S, true, true);
	MapMove(EKeys::Q, false, true);
	MapMove(EKeys::A, false, true);
	MapMove(EKeys::D, false, false);

	// Souris : Y inversé pour que AddControllerPitchInput(Y) regarde vers le haut quand la souris monte.
	{
		FEnhancedActionKeyMapping& Mapping = Context->MapKey(LookAction, EKeys::Mouse2D);
		UInputModifierNegate* NegateY = NewObject<UInputModifierNegate>(Context);
		NegateY->bX = false;
		NegateY->bY = true;
		NegateY->bZ = false;
		Mapping.Modifiers.Add(NegateY);
	}

	Context->MapKey(JumpAction, EKeys::SpaceBar);
	Context->MapKey(CrouchAction, EKeys::LeftControl);
	Context->MapKey(SprintAction, EKeys::LeftShift);
	Context->MapKey(PrimaryAction, EKeys::LeftMouseButton);
	Context->MapKey(SecondaryAction, EKeys::RightMouseButton);
	Context->MapKey(ReloadAction, EKeys::R);
	// Touches 1 et 2 : même code de touche en AZERTY (&, é) qu'en QWERTY.
	Context->MapKey(Mode1Action, EKeys::One);
	Context->MapKey(Mode2Action, EKeys::Two);
	Context->MapKey(CycleModeAction, EKeys::MouseWheelAxis);

	DefaultMappingContext = Context;
	UE_LOG(LogReshoot, Log, TEXT("No input assets assigned on %s: using runtime default mapping context."), *GetName());
}

void AReshootCharacter::OnMove(const FInputActionValue& Value)
{
	if (bFrozen || !Controller)
	{
		return;
	}
	const FVector2D Axis = Value.Get<FVector2D>();

	if (IsInSpaceMode())
	{
		// Apesanteur : la caméra donne la direction, tangage compris.
		const FRotator ViewRotation = Controller->GetControlRotation();
		const FRotationMatrix ViewMatrix(ViewRotation);
		AddMovementInput(ViewMatrix.GetUnitAxis(EAxis::X), Axis.Y);
		AddMovementInput(ViewMatrix.GetUnitAxis(EAxis::Y), Axis.X);
	}
	else
	{
		const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);
		const FRotationMatrix YawMatrix(YawRotation);
		AddMovementInput(YawMatrix.GetUnitAxis(EAxis::X), Axis.Y);
		AddMovementInput(YawMatrix.GetUnitAxis(EAxis::Y), Axis.X);
	}
}

void AReshootCharacter::OnLook(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	AddControllerYawInput(Axis.X);
	AddControllerPitchInput(Axis.Y);
}

void AReshootCharacter::OnJumpStarted()
{
	if (bFrozen || IsInSpaceMode())
	{
		return; // En apesanteur, Espace monte (voir OnJumpTriggered).
	}

	if (bSliding)
	{
		// Saut en sortie de glissade : on garde (et on amplifie un peu) l'élan horizontal.
		EndSlide();
		UCharacterMovementComponent* Move = GetCharacterMovement();
		Move->Velocity.X *= SlideJumpSpeedMultiplier;
		Move->Velocity.Y *= SlideJumpSpeedMultiplier;
	}
	if (bIsCrouched || GetCharacterMovement()->bWantsToCrouch)
	{
		UnCrouch();
	}
	Jump();
}

void AReshootCharacter::OnJumpTriggered()
{
	if (!bFrozen && IsInSpaceMode())
	{
		AddMovementInput(FVector::UpVector, 1.f);
	}
}

void AReshootCharacter::OnJumpCompleted()
{
	StopJumping();
}

void AReshootCharacter::OnCrouchStarted()
{
	bCrouchHeld = true;
	if (bFrozen || IsInSpaceMode())
	{
		return; // En apesanteur, Ctrl descend (voir OnCrouchTriggered).
	}

	if (CanStartSlide())
	{
		StartSlide();
	}
	else
	{
		Crouch();
	}
	UpdateMaxSpeeds();
}

void AReshootCharacter::OnCrouchTriggered()
{
	if (!bFrozen && IsInSpaceMode())
	{
		AddMovementInput(FVector::DownVector, 1.f);
	}
}

void AReshootCharacter::OnCrouchCompleted()
{
	bCrouchHeld = false;
	EndSlide();
	UnCrouch();
	UpdateMaxSpeeds();
}

void AReshootCharacter::OnSprintStarted()
{
	bSprintHeld = true;
	UpdateMaxSpeeds();
}

void AReshootCharacter::OnSprintCompleted()
{
	bSprintHeld = false;
	UpdateMaxSpeeds();
}

void AReshootCharacter::OnPrimaryStarted()
{
	if (!bFrozen && CurrentWeapon && IsWeaponActionAllowed(EWeaponAction::Primary))
	{
		CurrentWeapon->StartPrimary();
	}
}

void AReshootCharacter::OnPrimaryCompleted()
{
	if (CurrentWeapon)
	{
		CurrentWeapon->StopPrimary();
	}
}

void AReshootCharacter::OnSecondaryStarted()
{
	if (!bFrozen && CurrentWeapon && IsWeaponActionAllowed(EWeaponAction::Secondary))
	{
		CurrentWeapon->StartSecondary();
	}
}

void AReshootCharacter::OnSecondaryCompleted()
{
	if (CurrentWeapon)
	{
		CurrentWeapon->StopSecondary();
	}
}

void AReshootCharacter::OnReload()
{
	if (!bFrozen && CurrentWeapon && IsWeaponActionAllowed(EWeaponAction::Reload))
	{
		CurrentWeapon->Reload();
	}
}

void AReshootCharacter::OnMode1()
{
	if (!bFrozen && CurrentWeapon)
	{
		CurrentWeapon->SelectMode(0); // Le mode 0 reste toujours permis.
	}
}

void AReshootCharacter::OnMode2()
{
	if (!bFrozen && CurrentWeapon && IsWeaponActionAllowed(EWeaponAction::SwitchMode))
	{
		CurrentWeapon->SelectMode(1);
	}
}

void AReshootCharacter::OnCycleMode(const FInputActionValue& Value)
{
	const float Axis = Value.Get<float>();
	if (!bFrozen && CurrentWeapon && Axis != 0.f && IsWeaponActionAllowed(EWeaponAction::SwitchMode))
	{
		CurrentWeapon->CycleMode(Axis > 0.f ? 1 : -1);
	}
}
