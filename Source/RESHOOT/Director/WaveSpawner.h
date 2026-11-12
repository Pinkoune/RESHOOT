// RESHOOT — fait apparaître les figurants d'une vague. Un par niveau (placé dans L_Stage).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/ReshootTypes.h"
#include "Director/ShootScript.h"
#include "WaveSpawner.generated.h"

class AReshootEnemy;
class AWaveSpawner;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWaveCleared, AWaveSpawner*, Spawner);

UCLASS(Blueprintable)
class RESHOOT_API AWaveSpawner : public AActor
{
	GENERATED_BODY()

public:
	AWaveSpawner();

	/** Lance une vague. Remplace la vague en attente d'apparition s'il y en a une. */
	UFUNCTION(BlueprintCallable, Category = "Waves")
	void SpawnWave(const FWaveDef& Wave);

	/** Détruit tous les figurants vivants sans notifier de kill (reprise, fin de prise). */
	UFUNCTION(BlueprintCallable, Category = "Waves")
	void KillAll();

	/** Remplace chaque figurant vivant par un figurant de l'univers donné, au même endroit (final). */
	UFUNCTION(BlueprintCallable, Category = "Waves")
	void ReplaceAllForUniverse(EUniverse Universe);

	/** Gèle ou dégèle tous les figurants vivants et suspend les apparitions. */
	UFUNCTION(BlueprintCallable, Category = "Waves")
	void FreezeAll(bool bFreeze);

	UFUNCTION(BlueprintPure, Category = "Waves")
	int32 GetAliveCount() const;

	/** Vrai tant que des figurants restent à faire apparaître. */
	UFUNCTION(BlueprintPure, Category = "Waves")
	bool IsSpawning() const { return PendingSpawns > 0; }

	/** Diffusé quand la vague est entièrement apparue puis entièrement tuée. */
	UPROPERTY(BlueprintAssignable, Category = "Waves")
	FOnWaveCleared OnWaveCleared;

	/** Points d'apparition (Target Points, etc.). Vide = autour du spawner dans SpawnRadius. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Waves")
	TArray<TObjectPtr<AActor>> SpawnPoints;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Waves", meta = (ClampMin = "0"))
	float SpawnRadius = 800.f;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void SpawnNext();
	AReshootEnemy* SpawnEnemy(TSubclassOf<AReshootEnemy> EnemyClass, const FTransform& Transform);
	TSubclassOf<AReshootEnemy> PickEnemyClass(EUniverse Universe) const;
	FTransform PickSpawnTransform() const;

	UFUNCTION()
	void HandleEnemyDied(AReshootEnemy* Enemy);

	FWaveDef CurrentWave;
	int32 PendingSpawns = 0;
	bool bFrozen = false;
	FTimerHandle SpawnTimer;
	TArray<TWeakObjectPtr<AReshootEnemy>> AliveEnemies;
};
