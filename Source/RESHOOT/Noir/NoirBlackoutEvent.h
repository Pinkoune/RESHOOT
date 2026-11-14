// RESHOOT — événement du film noir : coupure de courant. Toutes les lumières du plateau s'éteignent
// pendant Duration secondes ; seuls les éclairs de tir (revolver, gangsters) éclairent. L'exposition est figée
// pour que l'œil de la caméra ne compense pas le noir.

#pragma once

#include "CoreMinimal.h"
#include "Core/UniverseEvent.h"
#include "NoirBlackoutEvent.generated.h"

class ULightComponentBase;
class UPostProcessComponent;
class USoundBase;

UCLASS(Blueprintable)
class RESHOOT_API ANoirBlackoutEvent : public AUniverseEvent
{
	GENERATED_BODY()

public:
	ANoirBlackoutEvent();

	virtual void StartEvent() override;
	virtual void StopEvent() override;

	// Durée de la coupure : AUniverseEvent::Duration (10 s par défaut, 0 = jusqu'à StopEvent).

	/** Multiplicateur appliqué à l'intensité de toutes les lumières pendant la coupure. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Blackout", meta = (ClampMin = "0", ClampMax = "1"))
	float LightScale = 0.f;

	/** Exposition figée pendant la coupure (compensation en EV, mode manuel). Plus haut = plus clair. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Blackout")
	float BlackoutExposureBias = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Blackout|Audio")
	TObjectPtr<USoundBase> PowerDownSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Blackout|Audio")
	TObjectPtr<USoundBase> PowerUpSound;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void RestoreLights();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Blackout")
	TObjectPtr<UPostProcessComponent> ExposureLock;

	TArray<TPair<TWeakObjectPtr<ULightComponentBase>, float>> SavedIntensities;
	FTimerHandle StopTimer;
};
