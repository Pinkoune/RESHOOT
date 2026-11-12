// RESHOOT — déroulé de la partie : scènes (une par univers), prises, vagues. Données uniquement.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Core/ReshootTypes.h"
#include "ShootScript.generated.h"

class AReshootEnemy;
class UDirective;
class UUniverseDefinition;

/** Une vague de figurants. */
USTRUCT(BlueprintType)
struct FWaveDef
{
	GENERATED_BODY()

	/** Nombre de figurants à faire apparaître. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wave", meta = (ClampMin = "0"))
	int32 Count = 5;

	/** Classes tirées au hasard. Vide = EnemyClasses du UUniverseDefinition courant. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wave")
	TArray<TSubclassOf<AReshootEnemy>> EnemyClassesOverride;

	/** Délai entre deux apparitions, en secondes. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wave", meta = (ClampMin = "0"))
	float SpawnInterval = 0.4f;

	/** Délai avant la vague, en secondes. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wave", meta = (ClampMin = "0"))
	float StartDelay = 0.f;
};

/** Une prise : des vagues, et éventuellement une directive imposée au « CUT! » qui l'ouvre. */
USTRUCT(BlueprintType)
struct FTakeDef
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take")
	TArray<FWaveDef> Waves;

	/** Directive imposée pour cette prise. Nulle = aucune contrainte. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take")
	TObjectPtr<UDirective> Directive;

	/** Réplique jouée au lancement de la prise (ligne de DT_DirectorLines). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take")
	FName ActionLineId;

	/** Réplique jouée au « CUT! » qui termine la prise. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take")
	FName CutLineId;
};

/** Une scène = un univers et ses prises. */
USTRUCT(BlueprintType)
struct FSceneDef
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Scene")
	TObjectPtr<UUniverseDefinition> UniverseDefinition;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Scene")
	FName IntroLineId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Scene")
	TArray<FTakeDef> Takes;
};

UCLASS(BlueprintType)
class RESHOOT_API UShootScript : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Retourne la définition d'un univers trouvée dans Scenes, puis dans FinaleUniverses. */
	UFUNCTION(BlueprintPure, Category = "Shoot Script")
	UUniverseDefinition* FindUniverseDefinition(EUniverse Universe) const;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shoot Script")
	TArray<FSceneDef> Scenes;

	/** Univers alternés pendant le final (dans l'ordre). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Finale")
	TArray<TObjectPtr<UUniverseDefinition>> FinaleUniverses;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Finale", meta = (ClampMin = "1"))
	float FinaleSwitchInterval = 20.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Finale", meta = (ClampMin = "0"))
	float FinaleDuration = 90.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Finale")
	TArray<FWaveDef> FinaleWaves;

	/** Audimat de base gagné par kill, avant multiplicateur de directive. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Scoring", meta = (ClampMin = "0"))
	float BaseKillRating = 10.f;

	/** Durée du gel « CUT! » avant « Action! ». */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Timing", meta = (ClampMin = "0"))
	float CutDuration = 3.f;

	/** Délai avant de relancer la prise après la mort du joueur. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Timing", meta = (ClampMin = "0"))
	float RetakeDelay = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lines")
	FName RetakeLineId = TEXT("Retake");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lines")
	FName WrapLineId = TEXT("Wrap");
};
