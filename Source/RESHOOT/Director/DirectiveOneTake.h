// RESHOOT — directive « Pas de coupure ! » (film noir) : recharge interdite, le chargeur se remplit à chaque élimination.

#pragma once

#include "CoreMinimal.h"
#include "Director/Directive.h"
#include "DirectiveOneTake.generated.h"

UCLASS(BlueprintType)
class RESHOOT_API UDirectiveOneTake : public UDirective
{
	GENERATED_BODY()

public:
	virtual void OnBegin(UShootDirectorSubsystem* Director) override;
	virtual void OnEnd(UShootDirectorSubsystem* Director) override;
	virtual void OnKill(UShootDirectorSubsystem* Director, const FKillInfo& Kill) override;

private:
	void SetRevolverReloadBlocked(bool bBlocked) const;
};
