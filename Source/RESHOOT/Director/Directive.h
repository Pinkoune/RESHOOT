// RESHOOT — une contrainte du réalisateur (« que des tirs à la tête ! », « ne t'arrête jamais ! »).
// Data Asset : une sous-classe C++ par type de directive, un asset par réglage. Pas de Tick.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Core/ReshootTypes.h"
#include "Directive.generated.h"

class UShootDirectorSubsystem;

UCLASS(BlueprintType, Blueprintable)
class RESHOOT_API UDirective : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Appelé quand la directive devient active (au « CUT! »). */
	virtual void OnBegin(UShootDirectorSubsystem* Director);

	/** Appelé quand la directive est remplacée ou que la prise se termine. */
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
};
