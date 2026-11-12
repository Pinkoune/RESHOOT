// RESHOOT — revolver du film noir : hitscan, coup par coup, chargeur de 6, recharge (R ou auto quand vide),
// clic droit = visée avec zoom léger. Chaque tir fait un éclair de lumière (utile pendant la coupure de courant).

#pragma once

#include "CoreMinimal.h"
#include "Core/ReshootWeapon.h"
#include "NoirRevolver.generated.h"

class USoundBase;

UCLASS(Blueprintable)
class RESHOOT_API ANoirRevolver : public AReshootWeapon
{
	GENERATED_BODY()

public:
	ANoirRevolver();

	virtual void StartPrimary() override;
	virtual void StopPrimary() override;
	virtual void StartSecondary() override;
	virtual void StopSecondary() override;
	virtual void Reload() override;
	virtual void SetEnabled(bool bInEnabled) override;

	virtual int32 GetAmmo() const override { return CurrentAmmo; }
	virtual int32 GetMaxAmmo() const override { return MagazineSize; }

	UFUNCTION(BlueprintPure, Category = "Revolver")
	bool IsReloading() const { return bReloading; }

	/** Remplit le chargeur sans délai (directive « Plan-séquence » : un chargeur plein à chaque élimination). */
	UFUNCTION(BlueprintCallable, Category = "Revolver")
	void RefillMagazine();

	/** Interdit la recharge (directive « Plan-séquence »). */
	UFUNCTION(BlueprintCallable, Category = "Revolver")
	void SetReloadBlocked(bool bBlocked);

	/** Éclair de tir : lumière ponctuelle très brève, attachée à Owner. Utilisé aussi par les gangsters. */
	static void SpawnMuzzleFlash(AActor* Owner, const FVector& Location, float Intensity, float Duration);

	/** Dégâts d'une balle (le multiplicateur de tête est dans UHealthComponent). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Revolver", meta = (ClampMin = "0"))
	float Damage = 45.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Revolver", meta = (ClampMin = "0"))
	float Range = 10000.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Revolver", meta = (ClampMin = "1"))
	int32 MagazineSize = 6;

	/** Temps minimal entre deux tirs (secondes). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Revolver", meta = (ClampMin = "0"))
	float FireInterval = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Revolver", meta = (ClampMin = "0"))
	float ReloadTime = 1.5f;

	/** Champ de vision pendant la visée (clic droit). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Revolver", meta = (ClampMin = "10", ClampMax = "170"))
	float AimFieldOfView = 65.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Revolver|Flash", meta = (ClampMin = "0"))
	float MuzzleFlashIntensity = 3000.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Revolver|Flash", meta = (ClampMin = "0"))
	float MuzzleFlashDuration = 0.06f;

	/** Trait de debug à chaque tir (boîtes grises). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Revolver")
	bool bDrawShotTrace = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Revolver|Audio")
	TObjectPtr<USoundBase> FireSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Revolver|Audio")
	TObjectPtr<USoundBase> EmptySound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Revolver|Audio")
	TObjectPtr<USoundBase> ReloadSound;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void Fire();
	void FinishReload();
	void SetAiming(bool bAim);

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Revolver")
	int32 CurrentAmmo = 6;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Revolver")
	bool bReloading = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Revolver")
	bool bReloadBlocked = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Revolver")
	bool bAiming = false;

	float DefaultFieldOfView = 90.f;
	double LastFireTime = -1000.0;
	FTimerHandle ReloadTimer;
};
