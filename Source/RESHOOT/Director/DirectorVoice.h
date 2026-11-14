// RESHOOT — répliques du réalisateur : joue le son et pousse le sous-titre. Composant du GameState.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataTable.h"
#include "DirectorVoice.generated.h"

class USoundBase;
class UAudioComponent;

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

	/**
	 * Joue la réplique LineId (son 2D) et diffuse OnSubtitle. GDD : une réplique n'en coupe jamais une autre ;
	 * si une réplique est en cours, celle-ci attend son tour, ou est abandonnée si elle attendrait plus de MaxQueueWait.
	 * Retourne le temps jusqu'à la fin de cette réplique, attente comprise (0 si introuvable ou abandonnée).
	 */
	UFUNCTION(BlueprintCallable, Category = "Director Voice")
	float PlayLine(FName LineId);

	/** Coupe la réplique en cours et vide la file (fin de partie, changement de carte). */
	UFUNCTION(BlueprintCallable, Category = "Director Voice")
	void ClearLines();

	/** Attente maximale d'une réplique derrière les autres avant d'être abandonnée (plus d'actualité). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Director Voice", meta = (ClampMin = "0"))
	float MaxQueueWait = 4.f;

	/** Le HUD s'abonne ici pour afficher les sous-titres. */
	UPROPERTY(BlueprintAssignable, Category = "Director Voice")
	FOnDirectorSubtitle OnSubtitle;

	/** DT_DirectorLines (lignes de type FDirectorLine). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Director Voice")
	TObjectPtr<UDataTable> LinesTable;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	const FDirectorLine* FindLine(FName LineId) const;
	float GetLineDuration(const FDirectorLine& Line) const;
	/** Joue tout de suite (le canal est libre). */
	void StartLine(FName LineId);
	void PlayNextQueued();
	double Now() const;

	/** Répliques en attente, dans l'ordre. */
	TArray<FName> QueuedLines;

	/** Fin de la dernière réplique jouée ou programmée. */
	double ChannelFreeTime = 0.0;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> CurrentSound;

	FTimerHandle QueueTimer;
};
