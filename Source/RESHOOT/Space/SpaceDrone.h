// RESHOOT — point de départ (Space). Figurant vide dérivé de AReshootEnemy.

#pragma once

#include "CoreMinimal.h"
#include "Core/ReshootEnemy.h"
#include "SpaceDrone.generated.h"

UCLASS(Blueprintable)
class RESHOOT_API ASpaceDrone : public AReshootEnemy
{
	GENERATED_BODY()

public:
	ASpaceDrone(const FObjectInitializer& ObjectInitializer);
};
