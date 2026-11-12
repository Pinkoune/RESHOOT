// RESHOOT — répliques du réalisateur : joue le son et pousse le sous-titre. Composant du GameState.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataTable.h"
#include "DirectorVoice.generated.h"

class USoundBase;

/** Ligne de DT_DirectorLines. Le nom de la ligne (Row Name) est le LineId. */
USTRUCT(BlueprintType)
struct FDirectorLine : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Line")
	TObjectPtr<USoundBase> Sound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Line", meta = (MultiLine = true))
	FText Subtitle;

	/** Durée d'affichage minimale du sous-titre (la durée du son l'emporte si plus longue). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Line", meta = (ClampMin = "0"))
	float SubtitleDuration = 2.5f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnDirectorSubtitle, FName, LineId, const FText&, Subtitle, float, Duration);

UCLASS(ClassGroup = (Reshoot), meta = (BlueprintSpawnableComponent))
class RESHOOT_API UDirectorVoice : public UActorComponent
{
	GENERATED_BODY()

public:
	UDirectorVoice();

	/** Joue la réplique LineId (son 2D) et diffuse OnSubtitle. Retourne la durée (0 si introuvable). */
	UFUNCTION(BlueprintCallable, Category = "Director Voice")
	float PlayLine(FName LineId);

	/** Le HUD s'abonne ici pour afficher les sous-titres. */
	UPROPERTY(BlueprintAssignable, Category = "Director Voice")
	FOnDirectorSubtitle OnSubtitle;

	/** DT_DirectorLines (lignes de type FDirectorLine). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Director Voice")
	TObjectPtr<UDataTable> LinesTable;
};
