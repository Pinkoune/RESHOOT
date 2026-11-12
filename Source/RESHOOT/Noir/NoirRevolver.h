// RESHOOT — revolver du film noir : hitscan, coup par coup, chargeur de 6, recharge (R ou auto quand vide).

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
	virtual void Reload() override;
	virtual void SetEnabled(bool bInEnabled) override;

	virtual int32 GetAmmo() const override { return CurrentAmmo; }
	virtual int32 GetMaxAmmo() const override { return MagazineSize; }

	UFUNCTION(BlueprintPure, Category = "Revolver")
	bool IsReloading() const { return bReloading; }

	/** Dégâts d'une balle (le multiplicateur de tête est dans UHealthComponent). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Revolver", meta = (ClampMin = "0"))
	float Damage = 40.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Revolver", meta = (ClampMin = "0"))
	float Range = 10000.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Revolver", meta = (ClampMin = "1"))
	int32 MagazineSize = 6;

	/** Temps minimal entre deux tirs (secondes). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Revolver", meta = (ClampMin = "0"))
	float FireInterval = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Revolver", meta = (ClampMin = "0"))
	float ReloadTime = 1.5f;

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

	void Fire();
	void FinishReload();

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Revolver")
	int32 CurrentAmmo = 6;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Revolver")
	bool bReloading = false;

	double LastFireTime = -1000.0;
	FTimerHandle ReloadTimer;
};
