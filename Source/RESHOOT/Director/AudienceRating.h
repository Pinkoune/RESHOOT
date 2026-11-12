// RESHOOT — l'audimat (score), composant du GameState.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AudienceRating.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnRatingChanged, float, NewRating, float, Delta);

UCLASS(ClassGroup = (Reshoot), meta = (BlueprintSpawnableComponent))
class RESHOOT_API UAudienceRating : public UActorComponent
{
	GENERATED_BODY()

public:
	UAudienceRating();

	/** Ajoute (ou retire si négatif) de l'audimat. Borné entre 0 et MaxRating. */
	UFUNCTION(BlueprintCallable, Category = "Audience")
	void Add(float Amount);

	UFUNCTION(BlueprintCallable, Category = "Audience")
	void ResetRating();

	UFUNCTION(BlueprintPure, Category = "Audience")
	float GetRating() const { return Rating; }

	/** Entre 0 et 1. */
	UFUNCTION(BlueprintPure, Category = "Audience")
	float GetRatingPct() const;

	/** Nombre d'étoiles (0 à StarThresholds.Num()). */
	UFUNCTION(BlueprintPure, Category = "Audience")
	int32 GetStars() const;

	UPROPERTY(BlueprintAssignable, Category = "Audience")
	FOnRatingChanged OnRatingChanged;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audience", meta = (ClampMin = "1"))
	float MaxRating = 1000.f;

	/** Audimat minimal pour chaque étoile, croissant. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audience")
	TArray<float> StarThresholds = { 100.f, 300.f, 500.f, 700.f, 900.f };

private:
	UPROPERTY(VisibleInstanceOnly, Category = "Audience")
	float Rating = 0.f;
};
