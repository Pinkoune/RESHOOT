// RESHOOT — déroulé de la partie : scènes (une par univers), prises, vagues. Données uniquement.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Core/ReshootTypes.h"
#include "ShootScript.generated.h"

class AReshootEnemy;
class UDirective;
class UUniverseDefinition;
class UMaterialParameterCollection;

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

	/** Directive imposée au « CUT! » en pleine prise. Nulle = tirée au hasard dans RandomDirectives du script. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take")
	TObjectPtr<UDirective> Directive;

	/** Réplique jouée au lancement de la prise (ligne de DT_DirectorLines). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Take")
	FName ActionLineId;

	/** Commentaire du réalisateur quand la prise se termine (dernier figurant éliminé). */
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

	/** Tag du PlayerStart où le joueur est replacé à chaque prise. Vide = PlayerStart par défaut. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Scene")
	FName PlayerStartTag;

	/** Prise (index) pendant laquelle l'événement d'univers se déclenche. -1 = aucun. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Scene")
	int32 EventTakeIndex = 1;

	/** Délai après « ACTION! » avant l'annonce de l'événement. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Scene", meta = (ClampMin = "0"))
	float EventDelay = 15.f;
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

	/** Audimat par élimination sans directive active. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Scoring", meta = (ClampMin = "0"))
	float BaseKillRating = 10.f;

	/** Audimat par élimination qui respecte la directive (et pour toute élimination sous une directive restrictive). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Scoring", meta = (ClampMin = "0"))
	float DirectiveKillRating = 25.f;

	/** Audimat par élimination alors que la directive est ignorée. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Scoring", meta = (ClampMin = "0"))
	float IgnoredDirectiveKillRating = 5.f;

	/** Bonus de prise terminée sans mourir. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Scoring", meta = (ClampMin = "0"))
	float TakeClearedBonus = 40.f;

	/** Perte d'audimat à la mort du joueur (valeur positive, retirée). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Scoring", meta = (ClampMin = "0"))
	float DeathPenalty = 60.f;

	/** Le « CUT! » tombe au hasard entre CutDelayMin et CutDelayMax secondes après le clap. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Timing", meta = (ClampMin = "0"))
	float CutDelayMin = 20.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Timing", meta = (ClampMin = "0"))
	float CutDelayMax = 40.f;

	/** Durée du gel « CUT! » avant « Action! » (GDD : 4 s au plus). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Timing", meta = (ClampMin = "0"))
	float CutDuration = 3.f;

	/** Commentaires tirés au hasard au « CUT! » en pleine prise, avant l'annonce de la directive. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Timing")
	TArray<FName> MidTakeCutLineIds;

	/** Délai entre l'annonce d'un événement d'univers et son déclenchement. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Timing", meta = (ClampMin = "0"))
	float EventWarningTime = 3.f;

	/** Directives tirées au « CUT! » quand la prise n'en impose pas (filtrées par univers). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Directives")
	TArray<TObjectPtr<UDirective>> DirectivePool;

	/** Tag du PlayerStart de la zone de casting. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Casting")
	FName CastingPlayerStartTag = TEXT("Casting");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Casting")
	FName CastingIntroLineId = TEXT("CastingIntro");

	/** Sous-niveau du studio de casting : chargé au début du casting, déchargé quand la scène 1 commence. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Casting")
	FName CastingSubLevelName = TEXT("L_Casting");

	/** Réplique quand le tutoriel est fini, juste avant la scène 1. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Casting")
	FName CastingDoneLineId = TEXT("CastingDone");

	/** Arme donnée pendant le casting (celle de cet univers ; ni sous-niveau ni post-process). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Casting")
	EUniverse CastingUniverse = EUniverse::Noir;

	/** Délai entre la fin du tutoriel et la scène 1. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Casting", meta = (ClampMin = "0"))
	float CastingOutroDelay = 2.f;

	/** Tag du PlayerStart du plateau (départ des scènes quand ni la scène ni l'univers n'en imposent un). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Casting")
	FName StagePlayerStartTag = TEXT("Stage");

	// --- Répétition, puis mise en scène (GDD) : décor carton à la 1re prise, réaliste ensuite ---

	/** « On améliore la mise en scène ! », à la fin de la première prise de chaque scène. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Staging")
	FName StagingLineId = TEXT("Staging_Improve");

	/** Durée du balayage du rideau de lumière. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Staging", meta = (ClampMin = "0.1"))
	float StagingSweepDuration = 2.5f;

	/** Collection lue par le matériau maître M_SetDressing : StagingParameter va de 0 (carton) à 1 (réaliste). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Staging")
	TObjectPtr<UMaterialParameterCollection> StagingCollection;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Staging")
	FName StagingParameter = TEXT("StagingReveal");

	/** Acteurs visibles seulement en version carton (décor carton, studio, projecteurs). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Staging")
	FName PropActorTag = TEXT("SetDressing_Prop");

	/** Acteurs visibles seulement en version réaliste (décor réaliste, ciel, horizon). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Staging")
	FName RealActorTag = TEXT("SetDressing_Real");

	/** Délai avant de relancer la prise après la mort du joueur. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Timing", meta = (ClampMin = "0"))
	float RetakeDelay = 2.f;

	/** « On la refait » : une réplique tirée au hasard à chaque mort du joueur. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lines")
	TArray<FName> RetakeLineIds = { TEXT("Retake"), TEXT("Retake_02"), TEXT("Retake_03"), TEXT("Retake_04"), TEXT("Retake_05") };

	/** Annonce du final. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lines")
	FName FinaleIntroLineId = TEXT("Intro_Finale");

	/** Une réplique tirée au hasard à chaque bascule d'univers du final (pas à la première). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lines")
	TArray<FName> FinaleSwitchLineIds = { TEXT("Switch_01"), TEXT("Switch_02"), TEXT("Switch_03") };

	/** Fin de partie réussie. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lines")
	FName WrapLineId = TEXT("Wrap");

	/** Fin de partie ratée : jouée à la place de WrapLineId jusqu'à WrapBadMaxStars étoiles. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lines")
	FName WrapBadLineId = TEXT("Wrap_Bad");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lines", meta = (ClampMin = "0", ClampMax = "5"))
	int32 WrapBadMaxStars = 2;
};
