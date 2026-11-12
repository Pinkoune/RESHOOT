// RESHOOT — une contrainte du réalisateur (« que des tirs à la tête ! », « ne t'arrête jamais ! »).
// Data Asset : une sous-classe C++ par type de directive, un asset par réglage. Pas de Tick.
// Un kill qui respecte la directive vaut BonusMultiplier, sinon PenaltyMultiplier (Judge).

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Core/ReshootTypes.h"
#include "Directive.generated.h"

class UShootDirectorSubsystem;
class AReshootCharacter;

UCLASS(BlueprintType, Blueprintable)
class RESHOOT_API UDirective : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Appelé quand la directive est annoncée (« CUT! » en pleine prise). */
	virtual void OnBegin(UShootDirectorSubsystem* Director);

	/** Appelé quand la directive est remplacée ou que la partie se termine. */
	virtual void OnEnd(UShootDirectorSubsystem* Director);

	/** Multiplicateur d'audimat pour ce kill. 1 = neutre, 0 = kill qui ne rapporte rien. */
	UFUNCTION(BlueprintCallable, Category = "Directive")
	virtual float EvaluateKill(const FKillInfo& Kill) const;

	/** Texte affiché par le HUD. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Directive")
	FText Title;

	/** Réplique jouée à l'annonce (ligne de DT_DirectorLines). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Directive")
	FName VoiceLineId;

	/** Multiplicateur d'audimat d'un kill qui respecte la directive. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Directive", meta = (ClampMin = "0"))
	float BonusMultiplier = 2.5f;

	/** Multiplicateur d'audimat d'un kill qui ne la respecte pas. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Directive", meta = (ClampMin = "0"))
	float PenaltyMultiplier = 0.5f;

protected:
	float Judge(bool bRespected) const { return bRespected ? BonusMultiplier : PenaltyMultiplier; }

	AReshootCharacter* GetPlayer() const { return PlayerPtr.Get(); }
	UShootDirectorSubsystem* GetDirector() const { return DirectorPtr.Get(); }

private:
	TWeakObjectPtr<AReshootCharacter> PlayerPtr;
	TWeakObjectPtr<UShootDirectorSubsystem> DirectorPtr;
};
