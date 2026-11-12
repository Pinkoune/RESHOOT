// RESHOOT — flèche du médiéval, tirée par l'arc du joueur et par l'archer.
// Projectile visible. Tirée par le joueur : blesse les figurants. Tirée par un figurant : blesse seulement le joueur.
// Se fige au « CUT! » et repart à « ACTION! ».

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MedievalArrow.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UProjectileMovementComponent;
class USoundBase;

UCLASS(Blueprintable)
class RESHOOT_API AMedievalArrow : public AActor
{
	GENERATED_BODY()

public:
	AMedievalArrow();

	/** Lance la flèche. À appeler juste après le spawn (Owner et Instigator = le tireur). */
	UFUNCTION(BlueprintCallable, Category = "Arrow")
	void Launch(const FVector& Direction, float Speed, float InDamage, float GravityScale);

	UFUNCTION(BlueprintPure, Category = "Arrow")
	float GetDamage() const { return Damage; }

	/** Part haute de la capsule (0..1 depuis le centre vers le sommet) qui compte comme tête. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arrow", meta = (ClampMin = "0", ClampMax = "1"))
	float HeadZoneFraction = 0.65f;

	/** Durée pendant laquelle la flèche reste plantée dans le décor. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arrow", meta = (ClampMin = "0"))
	float StuckLifeSpan = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arrow")
	TObjectPtr<USoundBase> ImpactSound;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arrow")
	TObjectPtr<USphereComponent> Collision;

	/** Boîte grise ; à remplacer dans le Blueprint. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arrow")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arrow")
	TObjectPtr<UProjectileMovementComponent> Movement;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Arrow")
	float Damage = 40.f;

private:
	UFUNCTION()
	void HandleHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	UFUNCTION()
	void HandleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void HandleCut();

	UFUNCTION()
	void HandleAction();

	bool CanDamage(const AActor* Target) const;
	bool IsHeadHit(const AActor* Target, const FHitResult& Hit) const;
	void StopAndStick(USceneComponent* AttachTo);

	bool bDone = false;
};
