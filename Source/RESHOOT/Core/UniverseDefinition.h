// RESHOOT — Data Asset qui décrit un univers. Un par univers, dans Content/Reshoot/<Univers>/.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/EngineTypes.h"
#include "Core/ReshootTypes.h"
#include "UniverseDefinition.generated.h"

class AReshootWeapon;
class AReshootEnemy;
class AUniverseEvent;
class UMaterialInterface;

UCLASS(BlueprintType)
class RESHOOT_API UUniverseDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Universe")
	EUniverse Universe = EUniverse::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Universe")
	FText DisplayName;

	/** Arme donnée au joueur dans cet univers. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Universe")
	TSubclassOf<AReshootWeapon> WeaponClass;

	/** Figurants de cet univers ; AWaveSpawner tire au hasard dedans. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Universe")
	TArray<TSubclassOf<AReshootEnemy>> EnemyClasses;

	/** Sous-niveau d'habillage chargé en streaming (ex. L_Medieval). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Universe")
	FName SubLevelName;

	/** Mode de déplacement du joueur : MOVE_Walking (Médiéval, Noir) ou MOVE_Flying (Spatial). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
	TEnumAsByte<EMovementMode> MovementMode = MOVE_Walking;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
	float GravityScale = 1.f;

	/** Décélération de freinage du mode de déplacement (faible en Spatial pour l'inertie). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement", meta = (ClampMin = "0"))
	float BrakingDeceleration = 2048.f;

	/** Post-process propre à l'univers (noir et blanc pour Noir). Peut être nul. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visuals")
	TObjectPtr<UMaterialInterface> PostProcessMaterial;

	/** Événement d'univers déclenché pendant les prises. Peut être nul. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Universe")
	TSubclassOf<AUniverseEvent> EventClass;
};
