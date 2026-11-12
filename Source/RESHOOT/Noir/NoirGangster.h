// RESHOOT — point de départ (Noir). Figurant vide dérivé de AReshootEnemy.

#pragma once

#include "CoreMinimal.h"
#include "Core/ReshootEnemy.h"
#include "NoirGangster.generated.h"

UCLASS(Blueprintable)
class RESHOOT_API ANoirGangster : public AReshootEnemy
{
	GENERATED_BODY()

public:
	ANoirGangster(const FObjectInitializer& ObjectInitializer);
};
