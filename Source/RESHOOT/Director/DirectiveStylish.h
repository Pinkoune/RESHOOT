// RESHOOT — directive « du style : glissades, sauts ».

#pragma once

#include "CoreMinimal.h"
#include "Director/Directive.h"
#include "DirectiveStylish.generated.h"

UCLASS(BlueprintType)
class RESHOOT_API UDirectiveStylish : public UDirective
{
	GENERATED_BODY()

public:
	virtual bool IsKillRespected(const FKillInfo& Kill) const override;
};
