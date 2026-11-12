// RESHOOT — point de départ (Noir). Arme vide dérivée de AReshootWeapon.

#pragma once

#include "CoreMinimal.h"
#include "Core/ReshootWeapon.h"
#include "NoirRevolver.generated.h"

UCLASS(Blueprintable)
class RESHOOT_API ANoirRevolver : public AReshootWeapon
{
	GENERATED_BODY()

public:
	ANoirRevolver();

	virtual void StartPrimary() override;
	virtual void StopPrimary() override;
};
