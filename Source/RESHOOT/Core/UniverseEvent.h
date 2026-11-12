// RESHOOT — classe de base des événements d'univers (chute de herse, panne de gravité, coupure de courant...).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UniverseEvent.generated.h"

UCLASS(Abstract, Blueprintable)
class RESHOOT_API AUniverseEvent : public AActor
{
	GENERATED_BODY()

public:
	AUniverseEvent();

	UFUNCTION(BlueprintCallable, Category = "Universe Event")
	virtual void StartEvent();

	UFUNCTION(BlueprintCallable, Category = "Universe Event")
	virtual void StopEvent();

	UFUNCTION(BlueprintPure, Category = "Universe Event")
	bool IsEventActive() const { return bEventActive; }

protected:
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Universe Event")
	bool bEventActive = false;
};
