// RESHOOT — arme du médiéval. Une seule classe, trois modes (le socle n'équipe qu'une arme par univers) :
//  - Épée   : clic gauche = coup dans un cône devant soi ; clic droit maintenu = bouclier (dégâts de face réduits).
//  - Arc    : clic gauche maintenu = bander, relâché = tirer une AMedievalArrow (dégâts selon la charge).
//  - Poings : clic gauche = coups de poing rapides qui repoussent ; clic droit maintenu = garde (réduction moindre).
// Changement de mode : touches 1, 2, 3, molette (cycle). Entrées créées par l'arme elle-même.

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
	Bow,
	Fists
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMedievalModeChanged, EMedievalWeaponMode, NewMode);

/** Combo mis à jour. Combo = 0 : combo cassé. PointsGained = points du dernier coup (0 à la casse). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnMedievalComboChanged, int32, Combo, int32, Multiplier, int32, Score, int32, PointsGained);

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

	// --- Combos de points (pour le HUD : getters + OnComboChanged) ---

	/** Un coup a touché (épée ou flèche du joueur). Appelé par l'arme et par AMedievalArrow. */
	UFUNCTION(BlueprintCallable, Category = "Medieval|Combo")
	void RegisterHit(bool bKilled, bool bHeadshot, bool bMelee);

	/** Coups enchaînés sans pause ni blessure. */
	UFUNCTION(BlueprintPure, Category = "Medieval|Combo")
	int32 GetCombo() const { return Combo; }

	UFUNCTION(BlueprintPure, Category = "Medieval|Combo")
	int32 GetComboMultiplier() const;

	/** Points marqués depuis que l'arme est en main. */
	UFUNCTION(BlueprintPure, Category = "Medieval|Combo")
	int32 GetComboScore() const { return ComboScore; }

	/** Temps restant avant que le combo retombe, entre 0 et 1 (jauge du HUD). */
	UFUNCTION(BlueprintPure, Category = "Medieval|Combo")
	float GetComboTimeLeftPct() const;

	UPROPERTY(BlueprintAssignable, Category = "Medieval|Combo")
	FOnMedievalComboChanged OnComboChanged;

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

	// --- Poings ---

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Medieval|Fists", meta = (ClampMin = "0"))
	float FistDamage = 20.f;

	/** Portée d'un coup de poing, en cm. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Medieval|Fists", meta = (ClampMin = "0"))
	float FistRange = 160.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Medieval|Fists", meta = (ClampMin = "0", ClampMax = "180"))
	float FistHalfAngle = 35.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Medieval|Fists", meta = (ClampMin = "0.1"))
	float FistPunchesPerSecond = 3.f;

	/** Vitesse donnée au figurant frappé (recul), en cm/s. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Medieval|Fists", meta = (ClampMin = "0"))
	float FistKnockback = 450.f;

	/** Part des dégâts de face qui passe en garde aux poings (0,5 = 50 %). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Medieval|Fists", meta = (ClampMin = "0", ClampMax = "1"))
	float FistGuardDamageMultiplier = 0.5f;

	// --- Combos ---

	/** Secondes sans toucher avant que le combo retombe. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Medieval|Combo", meta = (ClampMin = "0.1"))
	float ComboWindow = 2.5f;

	/** Nombre de coups pour gagner un cran de multiplicateur (3 = x2 au 3e coup, x3 au 6e…). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Medieval|Combo", meta = (ClampMin = "1"))
	int32 HitsPerMultiplierStep = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Medieval|Combo", meta = (ClampMin = "1"))
	int32 MaxComboMultiplier = 5;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Medieval|Combo", meta = (ClampMin = "0"))
	int32 PointsPerHit = 10;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Medieval|Combo", meta = (ClampMin = "0"))
	int32 PointsPerKill = 50;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Medieval|Combo", meta = (ClampMin = "0"))
	int32 HeadshotBonusPoints = 25;

	/** Être blessé casse le combo. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Medieval|Combo")
	bool bDamageBreaksCombo = true;

	/** Affichage provisoire en haut de l'écran, en attendant le HUD de l'équipe Interface. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Medieval|Combo")
	bool bShowComboOnScreen = true;

	// --- Positions en boîte grise (relatives à l'arme), réglables sans recompiler ---

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Medieval|Visuals")
	FVector ShieldLoweredLocation = FVector(0.f, -35.f, -15.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Medieval|Visuals")
	FVector ShieldRaisedLocation = FVector(40.f, -20.f, 0.f);

	/** Poings au repos (gauche, droit) et en garde, relatifs à l'arme. Le coup avance le poing de FistPunchReach. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Medieval|Visuals")
	FVector FistLeftRestLocation = FVector(15.f, -35.f, -5.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Medieval|Visuals")
	FVector FistRightRestLocation = FVector(15.f, 0.f, -5.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Medieval|Visuals")
	FVector FistLeftGuardLocation = FVector(25.f, -25.f, 12.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Medieval|Visuals")
	FVector FistRightGuardLocation = FVector(25.f, -8.f, 12.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Medieval|Visuals")
	float FistPunchReach = 35.f;

	/** Épée baissée sur le côté pendant la garde au bouclier. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Medieval|Visuals")
	FTransform SwordGuardTransform = FTransform(FRotator(-60.f, 15.f, 0.f), FVector(10.f, 15.f, -22.f), FVector(0.9f, 0.04f, 0.08f));

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Medieval|Visuals")
	FTransform BowRestTransform = FTransform(FRotator(0.f, 0.f, -30.f), FVector(20.f, -37.f, 5.f), FVector(0.03f, 0.03f, 0.5f));

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Medieval|Visuals")
	FTransform BowDrawTransform = FTransform(FRotator(0.f, 0.f, -40.f), FVector(10.f, -32.f, 5.f), FVector(0.03f, 0.03f, 0.5f));

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
	TObjectPtr<UInputAction> SelectFistsAction;

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

	/** Poings en boîte grise ; à remplacer dans le Blueprint. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Medieval")
	TObjectPtr<UStaticMeshComponent> FistLeftMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Medieval")
	TObjectPtr<UStaticMeshComponent> FistRightMesh;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Medieval")
	EMedievalWeaponMode Mode = EMedievalWeaponMode::Sword;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Medieval")
	bool bShieldRaised = false;

private:
	void SwordSwing();
	void ResetSwingPose();
	void Punch();
	void ResetPunchPose();

	/** Frappe au corps à corps dans un cône. Retourne le nombre de cibles touchées. */
	int32 MeleeSweep(float Range, float HalfAngle, float Damage, float Knockback);
	void RaiseShield(bool bRaise);
	void EnforceShieldSpeed();
	void StartBowCharge();
	void ReleaseBow();
	void UpdateVisuals();

	void ResetCombo();
	void ShowComboOnScreen(int32 PointsGained) const;

	UFUNCTION()
	void HandleOwnerHealthChanged(UHealthComponent* HealthComponent, float NewHealth, float Delta);

	void TrySetupInput();
	void CreateDefaultInput();
	void OnSelectSword();
	void OnSelectBow();
	void OnSelectFists();
	void OnToggleMode();

	bool bShieldAllowed = true;
	bool bBowAllowed = true;
	bool bCharging = false;
	bool bInputReady = false;
	double ChargeStartTime = 0.0;
	double LastSwingTime = -1000.0;
	double LastArrowTime = -1000.0;
	double LastPunchTime = -1000.0;
	bool bNextPunchLeft = true;
	FTimerHandle PunchPoseTimer;
	FTransform SwordRestTransform;

	FTimerHandle SwingRepeatTimer;
	FTimerHandle SwingPoseTimer;
	FTimerHandle ShieldSpeedTimer;
	FTimerHandle InputSetupTimer;
	FTimerHandle ComboTimer;

	int32 Combo = 0;
	int32 ComboScore = 0;

	UPROPERTY(Transient)
	TObjectPtr<UHealthComponent> BoundOwnerHealth;

	UPROPERTY(Transient)
	TObjectPtr<APlayerController> InputController;
};
