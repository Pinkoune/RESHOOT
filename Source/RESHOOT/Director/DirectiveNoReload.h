// RESHOOT — directive « pas de recharge » : les kills rapportent tant que le joueur n'a pas rechargé.
// Neutre avec une arme sans munitions (épée, laser). Recharge détectée par un timer (munitions qui remontent).

#pragma once

#include "CoreMinimal.h"
#include "Director/Directive.h"
#include "DirectiveNoReload.generated.h"

class AReshootWeapon;

UCLASS(BlueprintType)
class RESHOOT_API UDirectiveNoReload : public UDirective
{
	GENERATED_BODY()

public:
	virtual void OnBegin(UShootDirectorSubsystem* Director) override;
	virtual void OnEnd(UShootDirectorSubsystem* Director) override;
	virtual bool IsKillRespected(const FKillInfo& Kill) const override;

	/** Intervalle de vérification des munitions, en secondes. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Directive", meta = (ClampMin = "0.02"))
	float AmmoCheckInterval = 0.1f;

private:
	void CheckAmmo();

	FTimerHandle AmmoTimer;
	TWeakObjectPtr<AReshootWeapon> WatchedWeapon;
	int32 LastAmmo = -1;
	bool bReloaded = false;
};
