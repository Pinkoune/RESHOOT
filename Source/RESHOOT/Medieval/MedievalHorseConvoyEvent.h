// RESHOOT — événement du médiéval : un convoi de chevaux de bois, poussé sur rails par deux techniciens,
// traverse le plateau et renverse tout sur son passage (joueur blessé et projeté, figurants fauchés).
// Les rails sont posés en permanence le long de l'axe Y local de l'acteur ; le convoi part d'un bout et va à l'autre,
// puis repart dans l'autre sens à l'événement suivant. Se fige au « CUT! ».

#pragma once

#include "CoreMinimal.h"
#include "Core/UniverseEvent.h"
#include "MedievalHorseConvoyEvent.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class UInstancedStaticMeshComponent;
class USoundBase;

UCLASS(Blueprintable)
class RESHOOT_API AMedievalHorseConvoyEvent : public AUniverseEvent
{
	GENERATED_BODY()

public:
	AMedievalHorseConvoyEvent();

	virtual void StartEvent() override;
	virtual void StopEvent() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void OnConstruction(const FTransform& Transform) override;

	/** Demi-longueur des rails (cm), de part et d'autre de l'acteur, le long de son axe Y. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Convoy", meta = (ClampMin = "100"))
	float RailHalfLength = 2800.f;

	/** Vitesse du convoi (cm/s). Plus rapide que la marche du joueur : il faut s'écarter, pas le distancer. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Convoy", meta = (ClampMin = "0"))
	float Speed = 1100.f;

	/** Temps pendant lequel le convoi trépigne en bout de rail avant de partir (en plus de l'annonce du réalisateur). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Convoy", meta = (ClampMin = "0"))
	float WarningDelay = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Convoy", meta = (ClampMin = "0"))
	float PlayerDamage = 25.f;

	/** Dégâts aux figurants : assez pour les faucher. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Convoy", meta = (ClampMin = "0"))
	float EnemyDamage = 500.f;

	/** Vitesse donnée à ce qui est renversé (cm/s), sur le côté et vers le haut. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Convoy", meta = (ClampMin = "0"))
	float KnockbackSpeed = 1100.f;

	/** Test sans réalisateur : relance l'événement toutes les N secondes (0 = jamais, le réalisateur s'en charge). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Convoy|Test", meta = (ClampMin = "0"))
	float AutoStartInterval = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Convoy|Audio")
	TObjectPtr<USoundBase> WarningSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Convoy|Audio")
	TObjectPtr<USoundBase> RollingSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Convoy|Audio")
	TObjectPtr<USoundBase> ImpactSound;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Convoy")
	TObjectPtr<USceneComponent> Root;

	/** Tout ce qui roule : chevaux, plateformes, techniciens, zone de choc. Se déplace le long des rails. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Convoy")
	TObjectPtr<USceneComponent> Convoy;

	/** Zone qui renverse ce qu'elle touche. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Convoy")
	TObjectPtr<UBoxComponent> Hitbox;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Convoy")
	TObjectPtr<UStaticMeshComponent> RailLeft;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Convoy")
	TObjectPtr<UStaticMeshComponent> RailRight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Convoy")
	TObjectPtr<UInstancedStaticMeshComponent> Sleepers;

	/** Pièces en boîte grise du convoi (chevaux de bois, roues, techniciens). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Convoy")
	TArray<TObjectPtr<UStaticMeshComponent>> ConvoyParts;

private:
	void BeginRolling();
	void PlaceConvoyAtStart();
	void HitPawnsInPath();
	void UpdateRailsAndSleepers();

	UFUNCTION()
	void HandleCut();

	UFUNCTION()
	void HandleAction();

	/** +1 : de -Y vers +Y ; -1 : retour. Alterne à chaque passage. */
	float Direction = 1.f;
	bool bRolling = false;
	float WarningElapsed = 0.f;
	float ConvoyY = 0.f;

	UPROPERTY(Transient)
	TSet<TObjectPtr<AActor>> HitThisPass;

	FTimerHandle WarningTimer;
	FTimerHandle AutoStartTimer;
};
