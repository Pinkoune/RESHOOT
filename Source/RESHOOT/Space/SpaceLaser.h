// RESHOOT — point de départ (Space). Arme vide dérivée de AReshootWeapon.

#pragma once

#include "CoreMinimal.h"
#include "Core/ReshootWeapon.h"
#include "SpaceLaser.generated.h"

UCLASS(Blueprintable)
class RESHOOT_API ASpaceLaser : public AReshootWeapon
{
	GENERATED_BODY()

public:
	ASpaceLaser();

	virtual void StartPrimary() override;
	virtual void StopPrimary() override;
};
