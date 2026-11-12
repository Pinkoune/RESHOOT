// RESHOOT — directive « au corps-à-corps ».

#pragma once

#include "CoreMinimal.h"
#include "Director/Directive.h"
#include "DirectiveMeleeOnly.generated.h"

UCLASS(BlueprintType)
class RESHOOT_API UDirectiveMeleeOnly : public UDirective
{
	GENERATED_BODY()

public:
	virtual bool IsKillRespected(const FKillInfo& Kill) const override;
};
