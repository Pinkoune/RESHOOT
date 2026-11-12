// RESHOOT — directive « que des tirs à la tête ».

#pragma once

#include "CoreMinimal.h"
#include "Director/Directive.h"
#include "DirectiveHeadshots.generated.h"

UCLASS(BlueprintType)
class RESHOOT_API UDirectiveHeadshots : public UDirective
{
	GENERATED_BODY()

public:
	virtual bool IsKillRespected(const FKillInfo& Kill) const override;
};
