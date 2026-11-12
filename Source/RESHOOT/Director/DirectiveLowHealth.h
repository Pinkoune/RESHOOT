// RESHOOT — directive « joue-la blessé ».

#pragma once

#include "CoreMinimal.h"
#include "Director/Directive.h"
#include "DirectiveLowHealth.generated.h"

UCLASS(BlueprintType)
class RESHOOT_API UDirectiveLowHealth : public UDirective
{
	GENERATED_BODY()

public:
	virtual float EvaluateKill(const FKillInfo& Kill) const override;

	/** Le kill compte si la vie du joueur est à ce pourcentage ou moins (0..1). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Directive", meta = (ClampMin = "0", ClampMax = "1"))
	float MaxHealthPct = 0.4f;
};
