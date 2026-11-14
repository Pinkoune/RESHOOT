// RESHOOT — directive « Range cet arc ! » (Médiéval) : l'arc est interdit, chaque élimination compte.

#pragma once

#include "CoreMinimal.h"
#include "Director/Directive.h"
#include "DirectiveNoBow.generated.h"

UCLASS(BlueprintType)
class RESHOOT_API UDirectiveNoBow : public UDirective
{
	GENERATED_BODY()

public:
	virtual void OnBegin(UShootDirectorSubsystem* Director) override;
	virtual void OnEnd(UShootDirectorSubsystem* Director) override;

private:
	/** L'épée médiévale lit ses propres entrées : on la remet à l'épée si l'arc ressort. */
	void EnforceNoBow();

	FTimerHandle EnforceTimer;
};
