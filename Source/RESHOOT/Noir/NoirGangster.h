// RESHOOT — gangster du film noir. Rejoint un couvert (acteur tagué CoverTag, ex. une caisse), reste baissé derrière,
// se relève toutes les BurstInterval secondes pour tirer une rafale vers le joueur, puis se rebaisse.
// Si le joueur s'approche à moins de FleeDistance, il court vers un autre couvert. Déplacement par AIController
// (navmesh si disponible, sinon il reste où il est).

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
	float Damage = 15.f;

	/** Secondes entre deux rafales. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gangster", meta = (ClampMin = "0.5"))
	float BurstInterval = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gangster", meta = (ClampMin = "1"))
	int32 BurstCount = 3;

	/** Secondes entre deux balles d'une rafale. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gangster", meta = (ClampMin = "0.05"))
	float ShotSpacing = 0.15f;

	/** Temps entre le moment où il se relève et la première balle (laisse au joueur le temps de réagir). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gangster", meta = (ClampMin = "0"))
	float PeekDelay = 0.4f;

	/** Temps à découvert après la dernière balle. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gangster", meta = (ClampMin = "0"))
	float StayExposedTime = 0.4f;

	/** Délai aléatoire max avant la première rafale, pour que les gangsters ne tirent pas tous en même temps. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gangster", meta = (ClampMin = "0"))
	float FirstShotMaxDelay = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gangster", meta = (ClampMin = "0"))
	float Range = 4000.f;

	/** Probabilité de toucher un joueur immobile (0..1). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gangster", meta = (ClampMin = "0", ClampMax = "1"))
	float HitChance = 0.35f;

	/** Probabilité de toucher un joueur en mouvement (0..1) : bouger doit payer. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gangster", meta = (ClampMin = "0", ClampMax = "1"))
	float MovingHitChance = 0.15f;

	/** Profondeur dont il se baisse derrière son couvert (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gangster|Cover", meta = (ClampMin = "0"))
	float DuckDepth = 55.f;

	/** Tag des acteurs qui servent de couvert. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gangster|Cover")
	FName CoverTag = TEXT("NoirCover");

	/** Distance entre le centre du couvert et la position du gangster, côté opposé au joueur. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gangster|Cover", meta = (ClampMin = "0"))
	float CoverOffset = 130.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gangster|Cover", meta = (ClampMin = "0"))
	float CoverSearchRadius = 3000.f;

	/** Le joueur plus près que ça : il change de couvert. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gangster|Cover", meta = (ClampMin = "0"))
	float FleeDistance = 600.f;

	/** Trait de debug à chaque tir (boîtes grises). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gangster")
	bool bDrawShotTrace = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gangster|Audio")
	TObjectPtr<USoundBase> FireSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gangster|Flash", meta = (ClampMin = "0"))
	float MuzzleFlashIntensity = 3000.f;

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

	UFUNCTION(BlueprintPure, Category = "Gangster")
	bool IsDucked() const { return bDucked; }

	/** Couvert actuel (peut être nul). */
	UFUNCTION(BlueprintPure, Category = "Gangster")
	AActor* GetCover() const { return CurrentCover.Get(); }

protected:
	virtual void BeginPlay() override;
	virtual void OnFreezeChanged(bool bFreeze) override;
	virtual void OnDeath(AActor* Killer, bool bHeadshot, bool bMelee) override;

	/** Toutes les BurstInterval : change de couvert si besoin, sinon se relève pour tirer. */
	void RunCycle();
	void FireOneShot();
	void FinishBurst();
	void SetDucked(bool bDuck);

	/** Choisit un couvert libre et s'y rend. bFlee : préfère un couvert loin du joueur. Retourne vrai si un déplacement est lancé. */
	bool MoveToCover(bool bFlee);
	FVector GetCoverSpot(const AActor* Cover) const;
	bool CanSeePlayer() const;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Gangster")
	bool bDucked = false;

	TWeakObjectPtr<AActor> CurrentCover;
	int32 ShotsLeft = 0;

	FTimerHandle CycleTimer;
	FTimerHandle ShotTimer;
	FTimerHandle DuckTimer;
};
