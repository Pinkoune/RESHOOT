// RESHOOT — directive « pas un seul coup » : les kills rapportent tant que le joueur n'a pas été touché.

#pragma once

#include "CoreMinimal.h"
#include "Director/Directive.h"
#include "DirectiveNoDamage.generated.h"

class UHealthComponent;

UCLASS(BlueprintType)
class RESHOOT_API UDirectiveNoDamage : public UDirective
{
	GENERATED_BODY()

public:
	virtual void OnBegin(UShootDirectorSubsystem* Director) override;
	virtual void OnEnd(UShootDirectorSubsystem* Director) override;
	virtual bool IsKillRespected(const FKillInfo& Kill) const override;

private:
	UFUNCTION()
	void HandleHealthChanged(UHealthComponent* HealthComponent, float NewHealth, float Delta);

	UHealthComponent* GetPlayerHealth() const;

	bool bHit = false;
};
