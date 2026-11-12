// RESHOOT — directive « plus vite ! » : chaque kill doit suivre le précédent (ou le « Action! ») de près.

#pragma once

#include "CoreMinimal.h"
#include "Director/Directive.h"
#include "DirectiveFast.generated.h"

UCLASS(BlueprintType)
class RESHOOT_API UDirectiveFast : public UDirective
{
	GENERATED_BODY()

public:
	virtual void OnBegin(UShootDirectorSubsystem* Director) override;
	virtual void OnEnd(UShootDirectorSubsystem* Director) override;
	virtual float EvaluateKill(const FKillInfo& Kill) const override;

	/** Délai maximal, en secondes, entre deux kills (ou entre « Action! » et le premier kill). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Directive", meta = (ClampMin = "0.1"))
	float MaxSecondsBetweenKills = 5.f;

private:
	UFUNCTION()
	void HandleAction();

	double Now() const;

	mutable double LastKillTime = 0.0;
};
