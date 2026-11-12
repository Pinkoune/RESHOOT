// RESHOOT — chevalier figurant. Court vers le joueur (plus vite que sa marche, moins vite que son sprint)
// et frappe au contact après une courte préparation. Navmesh si présent, sinon ligne droite.
// À la mort, son corps en boîte grise bascule en physique et s'effondre de façon exagérée.

#pragma once

#include "CoreMinimal.h"
#include "Core/ReshootEnemy.h"
#include "MedievalKnight.generated.h"

class UStaticMeshComponent;
class USoundBase;

UCLASS(Blueprintable)
class RESHOOT_API AMedievalKnight : public AReshootEnemy
{
	GENERATED_BODY()

public:
	AMedievalKnight(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Knight", meta = (ClampMin = "0"))
	float RunSpeed = 700.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Knight", meta = (ClampMin = "0"))
	float AttackDamage = 20.f;

	/** Distance (bord de capsule à bord de capsule, en cm) à laquelle il frappe. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Knight", meta = (ClampMin = "0"))
	float AttackRange = 120.f;

	/** Secondes entre deux coups. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Knight", meta = (ClampMin = "0.1"))
	float AttackInterval = 1.f;

	/** Préparation avant l'impact : le joueur peut reculer ou lever le bouclier. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Knight", meta = (ClampMin = "0"))
	float AttackWindup = 0.35f;

	/** Fréquence de mise à jour de la poursuite. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Knight", meta = (ClampMin = "0.05"))
	float ThinkInterval = 0.2f;

	/** Utilise le navmesh s'il existe ; sinon (ou en cas d'échec) fonce en ligne droite. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Knight")
	bool bUsePathfinding = true;

	/** Force de l'effondrement à la mort. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Knight", meta = (ClampMin = "0"))
	float DeathImpulse = 600.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Knight")
	bool bDrawDebug = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Knight|Audio")
	TObjectPtr<USoundBase> AttackSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Knight|Audio")
	TObjectPtr<USoundBase> DeathSound;

protected:
	virtual void BeginPlay() override;
	virtual void OnFreezeChanged(bool bFreeze) override;
	virtual void OnDeath(AActor* Killer, bool bHeadshot, bool bMelee) override;

	/** Corps en boîte grise ; à cacher dans le Blueprint quand un vrai mesh est assigné. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Knight")
	TObjectPtr<UStaticMeshComponent> BodyMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Knight")
	TObjectPtr<UStaticMeshComponent> HelmetMesh;

private:
	void Think();
	void FinishAttack();
	float GetGapToPlayer() const;
	void FacePlayer();

	bool bAttacking = false;
	double LastAttackTime = -1000.0;
	FTimerHandle ThinkTimer;
	FTimerHandle AttackTimer;
};
