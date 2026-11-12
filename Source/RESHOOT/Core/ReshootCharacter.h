// RESHOOT — le joueur, vue FPS. Tout le code de déplacement du jeu vit ici (aucun univers n'en écrit).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Core/ReshootTypes.h"
#include "ReshootCharacter.generated.h"

class UCameraComponent;
class UHealthComponent;
class AReshootWeapon;
class UUniverseDefinition;
class UInputAction;
class UInputMappingContext;
struct FInputActionValue;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponChanged, AReshootWeapon*, NewWeapon);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerUniverseChanged, EUniverse, NewUniverse);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPlayerWeaponEvent, AReshootWeapon*, Weapon, EWeaponEvent, Event);

UCLASS(Blueprintable)
class RESHOOT_API AReshootCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AReshootCharacter(const FObjectInitializer& ObjectInitializer);

	// --- API publique (docs/ARCHITECTURE.md) ---

	/** Change d'arme et de mode de déplacement. Lit le UUniverseDefinition du script courant ; valeurs par défaut sinon. */
	UFUNCTION(BlueprintCallable, Category = "Reshoot")
	void SetUniverse(EUniverse NewUniverse);

	UFUNCTION(BlueprintPure, Category = "Reshoot")
	UHealthComponent* GetHealth() const { return HealthComponent; }

	// --- Compléments ---

	UFUNCTION(BlueprintPure, Category = "Reshoot")
	EUniverse GetUniverse() const { return CurrentUniverse; }

	UFUNCTION(BlueprintPure, Category = "Reshoot")
	AReshootWeapon* GetCurrentWeapon() const { return CurrentWeapon; }

	UFUNCTION(BlueprintPure, Category = "Reshoot")
	UCameraComponent* GetFirstPersonCamera() const { return FirstPersonCamera; }

	/** Détruit l'arme actuelle et équipe une instance de WeaponClass (nul = mains vides). */
	UFUNCTION(BlueprintCallable, Category = "Reshoot")
	AReshootWeapon* EquipWeapon(TSubclassOf<AReshootWeapon> WeaponClass);

	/** Gel du joueur pendant « CUT! » : entrées de déplacement et d'arme ignorées, vitesse annulée. */
	UFUNCTION(BlueprintCallable, Category = "Reshoot")
	void SetFrozen(bool bInFrozen);

	UFUNCTION(BlueprintPure, Category = "Reshoot")
	bool IsFrozen() const { return bFrozen; }

	UFUNCTION(BlueprintPure, Category = "Reshoot|Movement")
	bool IsSprinting() const;

	UFUNCTION(BlueprintPure, Category = "Reshoot|Movement")
	bool IsSliding() const { return bSliding; }

	/** Vrai si la vitesse dépasse MovingSpeedThreshold (utilisé pour FKillInfo::bPlayerMoving). */
	UFUNCTION(BlueprintPure, Category = "Reshoot|Movement")
	bool IsMoving() const;

	/** Vrai si le joueur est en l'air (saut, chute). Faux en apesanteur. */
	UFUNCTION(BlueprintPure, Category = "Reshoot|Movement")
	bool IsInAir() const;

	/** Début de prise ou reprise : téléporte, annule la vitesse, vie pleine, arme rechargée, glissade et zoom annulés. */
	UFUNCTION(BlueprintCallable, Category = "Reshoot")
	void ResetForTake(const FTransform& StartTransform);

	// --- Restrictions du réalisateur (directives). Survivent au changement d'arme. ---

	/** Interdit ou autorise une action d'arme. Interdire Secondary relâche le bouclier ; interdire SwitchMode force le mode 0. */
	UFUNCTION(BlueprintCallable, Category = "Reshoot|Directives")
	void SetWeaponActionAllowed(EWeaponAction Action, bool bAllowed);

	UFUNCTION(BlueprintPure, Category = "Reshoot|Directives")
	bool IsWeaponActionAllowed(EWeaponAction Action) const;

	/** Multiplicateur de coût des armes (surchauffe du laser). 1 = normal. */
	UFUNCTION(BlueprintCallable, Category = "Reshoot|Directives")
	void SetWeaponCostMultiplier(float Multiplier);

	UFUNCTION(BlueprintPure, Category = "Reshoot|Directives")
	float GetWeaponCostMultiplier() const { return WeaponCostMultiplier; }

	/** Lève toutes les restrictions (fin de directive, reprise). */
	UFUNCTION(BlueprintCallable, Category = "Reshoot|Directives")
	void ClearWeaponRestrictions();

	// --- Visée (zoom léger du clic droit, utilisé par le laser et le revolver) ---

	/** Interpole le champ de vision de la caméra vers TargetFOV. */
	UFUNCTION(BlueprintCallable, Category = "Reshoot|Aim")
	void SetAimFOV(float TargetFOV);

	/** Revient au DefaultFOV. */
	UFUNCTION(BlueprintCallable, Category = "Reshoot|Aim")
	void ResetAimFOV();

	// --- Apesanteur ---

	/** Événement spatial « la gravité se rallume » : vrai = marche avec gravité, faux = retour au vol. Sans effet hors apesanteur. */
	UFUNCTION(BlueprintCallable, Category = "Reshoot|Movement")
	void SetSpaceGravity(bool bGravityOn);

	UFUNCTION(BlueprintPure, Category = "Reshoot|Movement")
	bool IsSpaceGravityOn() const { return bSpaceGravityOn; }

	/** Commande console de test : DebugUniverse 1 (Medieval), 2 (Space), 3 (Noir). */
	UFUNCTION(Exec)
	void DebugUniverse(int32 UniverseIndex);

	UPROPERTY(BlueprintAssignable, Category = "Reshoot")
	FOnWeaponChanged OnWeaponChanged;

	UPROPERTY(BlueprintAssignable, Category = "Reshoot")
	FOnPlayerUniverseChanged OnUniverseChanged;

	/** Relais de AReshootWeapon::OnWeaponEvent de l'arme courante : on s'abonne une fois, ça survit aux changements d'arme. */
	UPROPERTY(BlueprintAssignable, Category = "Reshoot")
	FOnPlayerWeaponEvent OnPlayerWeaponEvent;

	// --- Réglages : univers et arme ---

	/** Univers appliqué au BeginPlay (pour tester un univers seul). None = marche + DefaultWeaponClass. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reshoot")
	EUniverse StartingUniverse = EUniverse::None;

	/** Arme utilisée quand aucun UUniverseDefinition n'est trouvé. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reshoot")
	TSubclassOf<AReshootWeapon> DefaultWeaponClass;

	/** Position de l'arme relative à la caméra. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reshoot")
	FTransform WeaponOffset = FTransform(FRotator::ZeroRotator, FVector(30.f, 15.f, -20.f));

	// --- Réglages : Enhanced Input ---
	// Si DefaultMappingContext est vide, un contexte et des actions sont créés en C++ au lancement
	// (ZQSD + WASD, souris, Espace, Ctrl, Maj, clics, R) : le jeu est jouable sans aucun asset.

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> CrouchAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> SprintAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> PrimaryAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> SecondaryAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> ReloadAction;

	/** Touche 1 : mode 0 de l'arme (épée). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> Mode1Action;

	/** Touche 2 : mode 1 de l'arme (arc). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> Mode2Action;

	/** Molette : mode suivant / précédent (Axis1D). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> CycleModeAction;

	// --- Réglages : caméra ---

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera", meta = (ClampMin = "30", ClampMax = "140"))
	float DefaultFOV = 90.f;

	/** Vitesse d'interpolation du zoom (FInterpTo). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera", meta = (ClampMin = "0"))
	float AimFOVInterpSpeed = 18.f;

	// --- Réglages : déplacement au sol (Médiéval, Noir) ---

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement", meta = (ClampMin = "0"))
	float WalkSpeed = 600.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement", meta = (ClampMin = "0"))
	float SprintSpeed = 950.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement", meta = (ClampMin = "0"))
	float CrouchSpeed = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement", meta = (ClampMin = "0"))
	float JumpVelocity = 620.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement", meta = (ClampMin = "0"))
	float MaxAcceleration = 4096.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement", meta = (ClampMin = "0"))
	float GroundFriction = 8.f;

	/** Contrôle aérien (0 = aucun, 1 = total). Élevé par design. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement", meta = (ClampMin = "0", ClampMax = "1"))
	float AirControl = 0.8f;

	/** Décélération au sol quand aucun UUniverseDefinition ne la fournit. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement", meta = (ClampMin = "0"))
	float DefaultWalkingBraking = 2048.f;

	/** Vitesse au-delà de laquelle le joueur est considéré « en mouvement ». */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement", meta = (ClampMin = "0"))
	float MovingSpeedThreshold = 50.f;

	// --- Réglages : glissade (accroupi pendant le sprint) ---

	/** Vitesse horizontale minimale pour déclencher une glissade. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|Slide", meta = (ClampMin = "0"))
	float SlideMinStartSpeed = 700.f;

	/** Vitesse ajoutée dans la direction du mouvement au début de la glissade. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|Slide", meta = (ClampMin = "0"))
	float SlideImpulse = 450.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|Slide", meta = (ClampMin = "0"))
	float SlideGroundFriction = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|Slide", meta = (ClampMin = "0"))
	float SlideBrakingDeceleration = 350.f;

	/** La glissade s'arrête sous cette vitesse horizontale. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|Slide", meta = (ClampMin = "0"))
	float SlideEndSpeed = 400.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|Slide", meta = (ClampMin = "0"))
	float SlideMaxDuration = 1.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|Slide", meta = (ClampMin = "0"))
	float SlideCooldown = 0.4f;

	/** Multiplicateur de la vitesse horizontale au saut en sortie de glissade (1 = élan conservé tel quel). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|Slide", meta = (ClampMin = "0"))
	float SlideJumpSpeedMultiplier = 1.1f;

	// --- Réglages : apesanteur (Space) ---

	/** Accélération en vol : plus faible = plus d'inertie au démarrage. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|Space", meta = (ClampMin = "0"))
	float SpaceAcceleration = 1500.f;

	/** Freinage en vol quand aucun UUniverseDefinition ne le fournit. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|Space", meta = (ClampMin = "0"))
	float DefaultSpaceBraking = 150.f;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void NotifyControllerChanged() override;
	virtual bool CanJumpInternal_Implementation() const override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCameraComponent> FirstPersonCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UHealthComponent> HealthComponent;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Reshoot")
	TObjectPtr<AReshootWeapon> CurrentWeapon;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Reshoot")
	EUniverse CurrentUniverse = EUniverse::None;

private:
	// Entrées
	void OnMove(const FInputActionValue& Value);
	void OnLook(const FInputActionValue& Value);
	void OnJumpStarted();
	void OnJumpTriggered();
	void OnJumpCompleted();
	void OnCrouchStarted();
	void OnCrouchTriggered();
	void OnCrouchCompleted();
	void OnSprintStarted();
	void OnSprintCompleted();
	void OnPrimaryStarted();
	void OnPrimaryCompleted();
	void OnSecondaryStarted();
	void OnSecondaryCompleted();
	void OnReload();
	void OnMode1();
	void OnMode2();
	void OnCycleMode(const FInputActionValue& Value);

	UFUNCTION()
	void HandleWeaponEvent(AReshootWeapon* Weapon, EWeaponEvent Event);

	void UpdateAimFOV();

	/** Crée contexte et actions en mémoire si DefaultMappingContext n'est pas assigné. */
	void EnsureDefaultInput();
	void AddMappingContext();

	// Déplacement
	void ApplyBaseMovementSettings();
	void ApplyUniverseMovement(const UUniverseDefinition* Definition, EUniverse Universe);
	void UpdateMaxSpeeds();
	bool IsInSpaceMode() const;
	bool CanStartSlide() const;
	void StartSlide();
	void UpdateSlide();
	void EndSlide();
	float GetHorizontalSpeed() const;

	const UUniverseDefinition* FindUniverseDefinition(EUniverse Universe) const;

	UFUNCTION()
	void HandleDeath(UHealthComponent* DeadComponent, AActor* Killer, bool bHeadshot, bool bMelee);

	bool bFrozen = false;
	bool bSprintHeld = false;
	bool bCrouchHeld = false;
	bool bSliding = false;
	float SlideStartTime = 0.f;
	float LastSlideEndTime = -1000.f;
	float CurrentWalkingBraking = 2048.f;
	FTimerHandle SlideTimer;

	/** Bits indexés par EWeaponAction : 1 = interdit. */
	uint8 BlockedWeaponActions = 0;
	float WeaponCostMultiplier = 1.f;
	float TargetAimFOV = 90.f;
	FTimerHandle AimFOVTimer;
	bool bSpaceGravityOn = false;
};
