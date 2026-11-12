// RESHOOT — point de départ (Medieval). Arme vide dérivée de AReshootWeapon.

#pragma once

#include "CoreMinimal.h"
#include "Core/ReshootWeapon.h"
#include "MedievalSword.generated.h"

UCLASS(Blueprintable)
class RESHOOT_API AMedievalSword : public AReshootWeapon
{
	GENERATED_BODY()

public:
	AMedievalSword();

	virtual void StartPrimary() override;
	virtual void StopPrimary() override;
};
