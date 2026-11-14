// RESHOOT — directive « Sans le bouclier ! » (Médiéval) : bouclier interdit, chaque élimination compte.

#pragma once

#include "CoreMinimal.h"
#include "Director/Directive.h"
#include "DirectiveNoShield.generated.h"

UCLASS(BlueprintType)
class RESHOOT_API UDirectiveNoShield : public UDirective
{
	GENERATED_BODY()

public:
	virtual void OnBegin(UShootDirectorSubsystem* Director) override;
	virtual void OnEnd(UShootDirectorSubsystem* Director) override;

private:
	/** L'épée médiévale lit ses propres entrées : on rabaisse le bouclier s'il remonte. */
	void EnforceShieldDown();

	FTimerHandle EnforceTimer;
};
