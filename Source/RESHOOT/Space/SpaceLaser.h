// RESHOOT — fusil laser (Space). Hitscan en rafale continue, pas de chargeur : il surchauffe, puis refroidit.
// GDD : 12 dégâts, 8 tirs/s, surchauffe après 3 s de tir continu, refroidit en 1,5 s, clic droit = zoom léger.

#pragma once

#include "CoreMinimal.h"
#include "Core/ReshootWeapon.h"
#include "SpaceLaser.generated.h"

class UStaticMeshComponent;
class UMaterialInterface;
class USoundBase;
class USpaceLaserFXComponent;
class UStaticMesh;
class UMaterialInstanceDynamic;

UCLASS(Blueprintable)
class RESHOOT_API ASpaceLaser : public AReshootWeapon
{
	GENERATED_BODY()

public:
	ASpaceLaser();

	virtual void StartPrimary() override;
	virtual void StopPrimary() override;
	virtual void StartSecondary() override;
	virtual void StopSecondary() override;
	virtual void SetEnabled(bool bInEnabled) override;
	virtual void Refill() override;
	virtual float GetGaugePct() const override { return Heat; }
	virtual void SetDressingReal(bool bReal) override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintPure, Category = "Laser")
	bool IsOverheated() const { return bOverheated; }

	// --- Réglages ---

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Laser", meta = (ClampMin = "0"))
	float DamagePerShot = 12.f;

	/** Tirs par seconde. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Laser", meta = (ClampMin = "0.1"))
	float FireRate = 8.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Laser", meta = (ClampMin = "0"))
	float Range = 12000.f;

	/** Dispersion (demi-angle en degrés) ; réduite en visée. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Laser", meta = (ClampMin = "0"))
	float SpreadDegrees = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Laser", meta = (ClampMin = "0"))
	float AimSpreadDegrees = 0.15f;

	/** Secondes de tir continu avant surchauffe (divisé par le multiplicateur de coût du porteur). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Laser|Heat", meta = (ClampMin = "0.1"))
	float SecondsToOverheat = 3.f;

	/** Temps pour passer de 100 % à 0 % de chaleur, en surchauffe comme au repos. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Laser|Heat", meta = (ClampMin = "0.1"))
	float CooldownTime = 1.5f;

	/** Délai après le dernier tir avant que le refroidissement commence (hors surchauffe). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Laser|Heat", meta = (ClampMin = "0"))
	float CooldownDelay = 0.25f;

	/** Champ de vision en visée (clic droit). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Laser|Aim", meta = (ClampMin = "30"))
	float AimFOV = 70.f;

	/** Recul vertical par tir, en degrés. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Laser|Feel", meta = (ClampMin = "0"))
	float RecoilPitch = 0.12f;

	/** Durée d'affichage du rayon par tir. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Laser|Feel", meta = (ClampMin = "0.01"))
	float BeamVisibleTime = 0.05f;

	/** Épaisseur du rayon (échelle Y/Z du mesh, qui fait 100 unités par défaut). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Laser|Feel", meta = (ClampMin = "0"))
	float BeamThickness = 0.03f;

	/** Matériau du rayon (émissif cyan conseillé). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Laser|Feel")
	TObjectPtr<UMaterialInterface> BeamMaterial;

	/** Son de tir du modèle réaliste. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Laser|Audio")
	TObjectPtr<USoundBase> FireSound;

	/** Carton seulement : sons de tir faits à la bouche, un tiré au hasard à chaque tir. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Laser|Audio")
	TArray<TObjectPtr<USoundBase>> PropFireSounds;

	/** Variation aléatoire de hauteur des sons carton (± fraction), pour qu'ils ne se répètent pas à l'identique. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Laser|Audio", meta = (ClampMin = "0", ClampMax = "0.5"))
	float PropPitchJitter = 0.08f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Laser|Audio")
	TObjectPtr<USoundBase> OverheatSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Laser|Audio")
	TObjectPtr<USoundBase> CooledSound;

	// --- Modèle réaliste (pivot et taille différents du carton) ---

	/** Transform relatif du mesh quand le modèle réaliste est affiché. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Laser|Real")
	FTransform RealMeshTransform = FTransform(FRotator(0.f, -90.f, 0.f), FVector(0.f, 0.f, -6.f));

	/** Bout du canon, dans l'espace du mesh réaliste. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Laser|Real")
	FVector RealMuzzleLocal = FVector(0.f, 52.f, 4.f);

	/** Point de visée (viseur), dans l'espace du mesh réaliste : aligné sur l'axe de la caméra en visée. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Laser|Real")
	FVector RealSightLocal = FVector(0.f, 5.f, 14.f);

	/** Pièces lumineuses posées sur le modèle réaliste (cube de 100 cm, transforms dans l'espace du mesh). Couleur selon la chaleur. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Laser|Real")
	TArray<FTransform> RealGlowParts;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Laser|Real")
	TObjectPtr<UStaticMesh> GlowPartMesh;

	/** Matériau émissif avec les paramètres Color et Intensity. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Laser|Real")
	TObjectPtr<UMaterialInterface> GlowPartMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Laser|Real")
	FLinearColor CoolColor = FLinearColor(0.1f, 0.75f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Laser|Real")
	FLinearColor HotColor = FLinearColor(1.f, 0.25f, 0.03f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Laser|Real", meta = (ClampMin = "0"))
	float GlowPartIntensity = 8.f;

	// --- Animations procédurales de l'arme en main ---

	/** Recul visuel par tir : recul vers l'arrière (cm) et cabrage (degrés). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Laser|Anim", meta = (ClampMin = "0"))
	float KickBack = 2.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Laser|Anim", meta = (ClampMin = "0"))
	float KickPitch = 2.5f;

	/** Vitesse de retour après le recul. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Laser|Anim", meta = (ClampMin = "0"))
	float KickReturnSpeed = 14.f;

	/** Retard de l'arme quand on tourne la vue (degrés par degré de rotation, borné). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Laser|Anim", meta = (ClampMin = "0"))
	float LookSway = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Laser|Anim", meta = (ClampMin = "0"))
	float MaxLookSway = 4.f;

	/** Balancement à la marche (cm) et longueur d'une foulée (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Laser|Anim", meta = (ClampMin = "0"))
	float WalkBob = 0.9f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Laser|Anim", meta = (ClampMin = "1"))
	float StrideLength = 170.f;

	/** Durée de la montée de l'arme (prise en main, reprise après le gel). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Laser|Anim", meta = (ClampMin = "0.01"))
	float EquipTime = 0.35f;

	/** Vitesse de passage en visée. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Laser|Anim", meta = (ClampMin = "0"))
	float AimSpeed = 12.f;

	/** Selon la direction de déplacement, à pleine vitesse : l'arme recule en avançant (cm)... */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Laser|Anim", meta = (ClampMin = "0"))
	float MoveLagForward = 1.5f;

	/** ... traîne à l'opposé du pas de côté (cm)... */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Laser|Anim", meta = (ClampMin = "0"))
	float MoveLagSide = 1.f;

	/** ... baisse le canon en avançant (degrés)... */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Laser|Anim", meta = (ClampMin = "0"))
	float MovePitch = 1.2f;

	/** ... tourne (degrés)... */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Laser|Anim", meta = (ClampMin = "0"))
	float MoveYaw = 1.5f;

	/** ... et penche du côté du pas de côté (degrés). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Laser|Anim", meta = (ClampMin = "0"))
	float MoveRoll = 5.f;

	/** Effets du tir : rayon, trait, éclairs, impacts. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Laser")
	TObjectPtr<USpaceLaserFXComponent> LaserFX;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Rayon visible : un cylindre étiré du canon au point d'impact. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Laser")
	TObjectPtr<UStaticMeshComponent> Beam;

	/** Position du canon relative à l'arme (départ visuel du rayon). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Laser|Feel")
	FVector MuzzleOffset = FVector(40.f, 0.f, 5.f);

private:
	void FireShot();
	void UpdateHeat();
	void ShowBeam(const FVector& From, const FVector& To);
	void HideBeam();
	void SetHeat(float NewHeat);
	FVector GetMuzzleLocation() const;
	FTransform GetBaseMeshTransform() const;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> GlowParts;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> GlowPartMids;

	FTransform PropMeshTransform = FTransform::Identity;
	FRotator LastControlRotation = FRotator::ZeroRotator;
	FRotator SwayRotation = FRotator::ZeroRotator;
	float KickAlpha = 0.f;
	float KickYaw = 0.f;
	float AimAlpha = 0.f;
	float OverheatAlpha = 0.f;
	float EquipAlpha = 0.f;
	float BobPhase = 0.f;
	float AnimTime = 0.f;

	float Heat = 0.f;
	bool bOverheated = false;
	bool bAiming = false;
	float LastShotTime = -1000.f;

	FTimerHandle FireTimer;
	FTimerHandle HeatTimer;
	FTimerHandle BeamTimer;
};
