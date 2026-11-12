// RESHOOT — événement spatial (lot d'Argon) : la gravité artificielle se rallume pendant Duration (10 s) ;
// joueur et drones retombent au sol, puis l'apesanteur revient. Implémentation dans SpaceGravityEvent.cpp uniquement.

#pragma once

#include "CoreMinimal.h"
#include "Core/UniverseEvent.h"
#include "SpaceGravityEvent.generated.h"

class USoundBase;

UCLASS(Blueprintable)
class RESHOOT_API ASpaceGravityEvent : public AUniverseEvent
{
	GENERATED_BODY()

public:
	ASpaceGravityEvent();

	/** Rallume la gravité : AReshootCharacter::SetSpaceGravity(true), ASpaceDrone::SetGravityEnabled(true). */
	virtual void StartEvent() override;

	/** Rétablit l'apesanteur pour le joueur et tous les drones vivants. */
	virtual void StopEvent() override;

	virtual void SetEventPaused(bool bPaused) override;

	/** Applique aussi la gravité aux drones. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gravity Event")
	bool bAffectDrones = true;

	/** Les lumières qui portent ce tag clignotent pendant l'événement (alarme). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gravity Event")
	FName AlarmLightTag = TEXT("GravityAlarm");

	/** Période de clignotement des lumières d'alarme. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gravity Event", meta = (ClampMin = "0.05"))
	float AlarmBlinkInterval = 0.4f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gravity Event|Audio")
	TObjectPtr<USoundBase> GravityOnSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gravity Event|Audio")
	TObjectPtr<USoundBase> GravityOffSound;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void ApplyGravity(bool bGravityOn);
	void BlinkAlarm();

	bool bAlarmLit = false;
	FTimerHandle AlarmTimer;
};
