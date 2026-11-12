// RESHOOT — classe de base des événements d'univers (chute de herse, panne de gravité, coupure de courant...).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UniverseEvent.generated.h"

class AUniverseEvent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnUniverseEventStateChanged, AUniverseEvent*, Event, bool, bActive);

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

	/** Gel au « CUT! » : les sous-classes suspendent mouvements et timers. Par défaut : met en pause le timer de Duration. */
	UFUNCTION(BlueprintCallable, Category = "Universe Event")
	virtual void SetEventPaused(bool bPaused);

	/** Durée de l'événement ; StopEvent est appelé tout seul à la fin. 0 = jusqu'à StopEvent. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Universe Event", meta = (ClampMin = "0"))
	float Duration = 10.f;

	UPROPERTY(BlueprintAssignable, Category = "Universe Event")
	FOnUniverseEventStateChanged OnEventStateChanged;

protected:
	FTimerHandle DurationTimer;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Universe Event")
	bool bEventActive = false;
};
