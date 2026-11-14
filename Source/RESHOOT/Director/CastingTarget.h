// RESHOOT — cible en carton du casting (GDD : trois cibles pour apprendre le tir). Prévient le réalisateur quand elle tombe.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CastingTarget.generated.h"

class UStaticMeshComponent;
class UHealthComponent;

UCLASS(Blueprintable)
class RESHOOT_API ACastingTarget : public AActor
{
	GENERATED_BODY()

public:
	ACastingTarget();

	/** Relève la cible (début du casting). */
	UFUNCTION(BlueprintCallable, Category = "Casting")
	void ResetTarget();

	UFUNCTION(BlueprintPure, Category = "Casting")
	bool IsDown() const { return bDown; }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Casting")
	TObjectPtr<UStaticMeshComponent> Board;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Casting")
	TObjectPtr<UStaticMeshComponent> Bullseye;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Casting")
	TObjectPtr<UHealthComponent> Health;

	/** Angle de la cible couchée (tangage, degrés). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Casting")
	float DownPitch = -80.f;

private:
	UFUNCTION()
	void HandleDeath(UHealthComponent* DeadComponent, AActor* Killer, bool bHeadshot, bool bMelee);

	bool bDown = false;
	FRotator StandingRotation = FRotator::ZeroRotator;
};
