// RESHOOT — point de départ (Medieval). Figurant vide dérivé de AReshootEnemy.

#pragma once

#include "CoreMinimal.h"
#include "Core/ReshootEnemy.h"
#include "MedievalKnight.generated.h"

UCLASS(Blueprintable)
class RESHOOT_API AMedievalKnight : public AReshootEnemy
{
	GENERATED_BODY()

public:
	AMedievalKnight(const FObjectInitializer& ObjectInitializer);
};
