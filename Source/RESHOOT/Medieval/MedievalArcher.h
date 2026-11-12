// RESHOOT — archer figurant. Immobile sur une position haute (à poser à la main ou via un point d'apparition),
// se tourne vers le joueur et tire une flèche lente et visible à intervalle régulier quand il le voit.

#pragma once

#include "CoreMinimal.h"
#include "Core/ReshootEnemy.h"
#include "MedievalArcher.generated.h"

class AMedievalArrow;
class UStaticMeshComponent;
class USoundBase;

UCLASS(Blueprintable)
class RESHOOT_API AMedievalArcher : public AReshootEnemy
{
	GENERATED_BODY()

public:
	AMedievalArcher(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Archer")
	TSubclassOf<AMedievalArrow> ArrowClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Archer", meta = (ClampMin = "0"))
	float ArrowDamage = 25.f;

	/** Lente pour être esquivable ou parée au bouclier. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Archer", meta = (ClampMin = "0"))
	float ArrowSpeed = 1200.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Archer", meta = (ClampMin = "0.1"))
	float FireInterval = 2.5f;

	/** Délai aléatoire max avant la première flèche, pour désynchroniser les archers. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Archer", meta = (ClampMin = "0"))
	float FirstShotMaxDelay = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Archer", meta = (ClampMin = "0"))
	float Range = 5000.f;

	/** Vise là où le joueur sera (0 = vise sa position actuelle, 1 = anticipation complète). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Archer", meta = (ClampMin = "0", ClampMax = "1"))
	float LeadFactor = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Archer|Audio")
	TObjectPtr<USoundBase> FireSound;

protected:
	virtual void BeginPlay() override;
	virtual void OnFreezeChanged(bool bFreeze) override;
	virtual void OnDeath(AActor* Killer, bool bHeadshot, bool bMelee) override;

	/** Corps en boîte grise ; à cacher dans le Blueprint quand un vrai mesh est assigné. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Archer")
	TObjectPtr<UStaticMeshComponent> BodyMesh;

private:
	void Fire();

	FTimerHandle FireTimer;
};
