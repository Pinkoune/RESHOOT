// RESHOOT — arme du médiéval. Une seule classe, deux modes (le socle n'équipe qu'une arme par univers) :
//  - Épée : clic gauche = coup dans un cône devant soi ; clic droit maintenu = bouclier (dégâts de face réduits).
//  - Arc  : clic gauche maintenu = bander, relâché = tirer une AMedievalArrow (dégâts selon la charge).
// Changement de mode : touche 1 (épée), touche 2 (arc), molette (bascule). Entrées créées par l'arme elle-même.

#pragma once

#include "CoreMinimal.h"
#include "Core/ReshootWeapon.h"
#include "MedievalSword.generated.h"

class AMedievalArrow;
class UStaticMeshComponent;
class UInputAction;
class UInputMappingContext;
class USoundBase;
class APlayerController;

UENUM(BlueprintType)
enum class EMedievalWeaponMode : uint8
{
	Sword,
	Bow
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMedievalModeChanged, EMedievalWeaponMode, NewMode);

UCLASS(Blueprintable)
class RESHOOT_API AMedievalSword : public AReshootWeapon
{
	GENERATED_BODY()

public:
	AMedievalSword();

	virtual void StartPrimary() override;
	virtual void StopPrimary() override;
	virtual void StartSecondary() override;
	virtual void StopSecondary() override;
	virtual void SetEnabled(bool bInEnabled) override;

	UFUNCTION(BlueprintCallable, Category = "Medieval")
	void SetMode(EMedievalWeaponMode NewMode);

	UFUNCTION(BlueprintCallable, Category = "Medieval")
	void ToggleMode();

	UFUNCTION(BlueprintPure, Category = "Medieval")
	EMedievalWeaponMode GetMode() const { return Mode; }

	UFUNCTION(BlueprintPure, Category = "Medieval")
	bool IsShieldRaised() const { return bShieldRaised; }

	/** Charge de l'arc entre 0 et 1 (0 si on ne bande pas). Pour le HUD. */
	UFUNCTION(BlueprintPure, Category = "Medieval")
	float GetBowCharge() const;

	/** Directive « Sans le bouclier ! ». */
	UFUNCTION(BlueprintCallable, Category = "Medieval")
	void SetShieldAllowed(bool bAllowed);

	/** Directive « Range cet arc ! ». Repasse à l'épée si l'arc était en main. */
	UFUNCTION(BlueprintCallable, Category = "Medieval")
	void SetBowAllowed(bool bAllowed);

	UPROPERTY(BlueprintAssignable, Category = "Medieval")
	FOnMedievalModeChanged OnModeChanged;

	// --- Épée ---

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Medieval|Sword", meta = (ClampMin = "0"))
	float SwordDamage = 50.f;

	/** Portée du cône, en cm (2,5 m). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Medieval|Sword", meta = (ClampMin = "0"))
	float SwordRange = 250.f;

	/** Demi-angle du cône de frappe, en degrés. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Medieval|Sword", meta = (ClampMin = "0", ClampMax = "180"))
	float SwordHalfAngle = 50.f;

	/** Coups par seconde. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Medieval|Sword", meta = (ClampMin = "0.1"))
	float SwordAttacksPerSecond = 1.5f;

	/** Garder le clic enfoncé enchaîne les coups. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Medieval|Sword")
	bool bSwordAutoRepeat = true;

	// --- Bouclier ---

	/** Part des dégâts de face qui passe quand le bouclier est levé (0,2 = 20 %). Cône réglé sur le UHealthComponent du joueur. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Medieval|Shield", meta = (ClampMin = "0", ClampMax = "1"))
	float ShieldDamageMultiplier = 0.2f;

	/** Vitesse max du joueur bouclier levé (pas de sprint). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Medieval|Shield", meta = (ClampMin = "0"))
	float ShieldMaxWalkSpeed = 600.f;

	// --- Arc ---

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Medieval|Bow")
	TSubclassOf<AMedievalArrow> ArrowClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Medieval|Bow", meta = (ClampMin = "0"))
	float BowMinDamage = 40.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Medieval|Bow", meta = (ClampMin = "0"))
	float BowMaxDamage = 100.f;

	/** Secondes pour une charge complète. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Medieval|Bow", meta = (ClampMin = "0.05"))
	float BowFullChargeTime = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Medieval|Bow", meta = (ClampMin = "0"))
	float BowMinArrowSpeed = 1800.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Medieval|Bow", meta = (ClampMin = "0"))
	float BowMaxArrowSpeed = 4500.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Medieval|Bow", meta = (ClampMin = "0"))
	float BowArrowGravityScale = 0.3f;

	/** Délai minimal entre deux flèches. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Medieval|Bow", meta = (ClampMin = "0"))
	float BowCooldown = 0.25f;

	// --- Debug et audio ---

	/** Dessine le cône de l'épée à chaque coup (boîtes grises). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Medieval|Debug")
	bool bDrawDebug = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Medieval|Audio")
	TObjectPtr<USoundBase> SwingSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Medieval|Audio")
	TObjectPtr<USoundBase> SwordHitSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Medieval|Audio")
	TObjectPtr<USoundBase> ShieldUpSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Medieval|Audio")
	TObjectPtr<USoundBase> BowReleaseSound;

	// --- Entrées propres à l'arme (créées en C++ si vides) ---

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Medieval|Input")
	TObjectPtr<UInputMappingContext> MedievalMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Medieval|Input")
	TObjectPtr<UInputAction> SelectSwordAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Medieval|Input")
	TObjectPtr<UInputAction> SelectBowAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Medieval|Input")
	TObjectPtr<UInputAction> ToggleModeAction;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Bouclier en boîte grise ; à remplacer dans le Blueprint. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Medieval")
	TObjectPtr<UStaticMeshComponent> ShieldMesh;

	/** Arc en boîte grise ; à remplacer dans le Blueprint. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Medieval")
	TObjectPtr<UStaticMeshComponent> BowMesh;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Medieval")
	EMedievalWeaponMode Mode = EMedievalWeaponMode::Sword;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Medieval")
	bool bShieldRaised = false;

private:
	void SwordSwing();
	void ResetSwingPose();
	void RaiseShield(bool bRaise);
	void EnforceShieldSpeed();
	void StartBowCharge();
	void ReleaseBow();
	void UpdateVisuals();

	void TrySetupInput();
	void CreateDefaultInput();
	void OnSelectSword();
	void OnSelectBow();
	void OnToggleMode();

	bool bShieldAllowed = true;
	bool bBowAllowed = true;
	bool bCharging = false;
	bool bInputReady = false;
	double ChargeStartTime = 0.0;
	double LastSwingTime = -1000.0;
	double LastArrowTime = -1000.0;
	FTransform SwordRestTransform;

	FTimerHandle SwingRepeatTimer;
	FTimerHandle SwingPoseTimer;
	FTimerHandle ShieldSpeedTimer;
	FTimerHandle InputSetupTimer;

	UPROPERTY(Transient)
	TObjectPtr<APlayerController> InputController;
};
