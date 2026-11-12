// RESHOOT — fusil laser (Space). Hitscan en rafale continue, pas de chargeur : il surchauffe, puis refroidit.
// GDD : 12 dégâts, 8 tirs/s, surchauffe après 3 s de tir continu, refroidit en 1,5 s, clic droit = zoom léger.

#pragma once

#include "CoreMinimal.h"
#include "Core/ReshootWeapon.h"
#include "SpaceLaser.generated.h"

class UStaticMeshComponent;
class UMaterialInterface;
class USoundBase;

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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Laser|Audio")
	TObjectPtr<USoundBase> FireSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Laser|Audio")
	TObjectPtr<USoundBase> OverheatSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Laser|Audio")
	TObjectPtr<USoundBase> CooledSound;

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

	float Heat = 0.f;
	bool bOverheated = false;
	bool bAiming = false;
	float LastShotTime = -1000.f;

	FTimerHandle FireTimer;
	FTimerHandle HeatTimer;
	FTimerHandle BeamTimer;
};
