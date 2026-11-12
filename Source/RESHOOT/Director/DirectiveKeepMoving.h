// RESHOOT — directive « ne t'arrête jamais ».

#pragma once

#include "CoreMinimal.h"
#include "Director/Directive.h"
#include "DirectiveKeepMoving.generated.h"

UCLASS(BlueprintType)
class RESHOOT_API UDirectiveKeepMoving : public UDirective
{
	GENERATED_BODY()

public:
	virtual float EvaluateKill(const FKillInfo& Kill) const override;
};
