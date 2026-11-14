// RESHOOT — directive « Le laser coûte cher ! » (spatial) : la surchauffe arrive plus vite, chaque élimination compte.

#pragma once

#include "CoreMinimal.h"
#include "Director/Directive.h"
#include "DirectiveLaserCost.generated.h"

UCLASS(BlueprintType)
class RESHOOT_API UDirectiveLaserCost : public UDirective
{
	GENERATED_BODY()

public:
	virtual void OnBegin(UShootDirectorSubsystem* Director) override;

	/** Multiplicateur de coût des armes (GDD : surchauffe deux fois plus vite). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Directive", meta = (ClampMin = "1"))
	float CostMultiplier = 2.f;
};
