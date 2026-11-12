// RESHOOT — gangster du film noir. Reste à couvert (placé derrière une caisse), se tourne vers le joueur
// et tire à intervalle régulier quand il le voit. Pas de déplacement : version boîte grise.

#pragma once

#include "CoreMinimal.h"
#include "Core/ReshootEnemy.h"
#include "NoirGangster.generated.h"

class USoundBase;
class UStaticMeshComponent;

UCLASS(Blueprintable)
class RESHOOT_API ANoirGangster : public AReshootEnemy
{
	GENERATED_BODY()

public:
	ANoirGangster(const FObjectInitializer& ObjectInitializer);

	/** Dégâts d'une balle qui touche le joueur. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gangster", meta = (ClampMin = "0"))
	float Damage = 8.f;

	/** Secondes entre deux tirs. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gangster", meta = (ClampMin = "0.1"))
	float FireInterval = 1.6f;

	/** Délai aléatoire max avant le premier tir, pour que les gangsters ne tirent pas tous en même temps. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gangster", meta = (ClampMin = "0"))
	float FirstShotMaxDelay = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gangster", meta = (ClampMin = "0"))
	float Range = 4000.f;

	/** Probabilité de toucher quand le joueur est visible (0..1). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gangster", meta = (ClampMin = "0", ClampMax = "1"))
	float HitChance = 0.5f;

	/** Trait de debug à chaque tir (boîtes grises). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gangster")
	bool bDrawShotTrace = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gangster|Audio")
	TObjectPtr<USoundBase> FireSound;

	// --- Corps en boîtes grises (remplaçables dans un Blueprint) ---
	// Le corps et la tête bloquent le canal Visibility (tirs du joueur) ; la tête porte le tag "head" (tir à la tête).

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gangster|Visual")
	TObjectPtr<UStaticMeshComponent> Body;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gangster|Visual")
	TObjectPtr<UStaticMeshComponent> Head;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gangster|Visual")
	TObjectPtr<UStaticMeshComponent> HatBrim;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gangster|Visual")
	TObjectPtr<UStaticMeshComponent> HatCrown;

protected:
	virtual void BeginPlay() override;
	virtual void OnFreezeChanged(bool bFreeze) override;
	virtual void OnDeath(AActor* Killer, bool bHeadshot, bool bMelee) override;

	void Fire();

	FTimerHandle FireTimer;
};
