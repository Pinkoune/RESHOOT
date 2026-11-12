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

	/**
	 * Vrai si l'élimination respecte la directive (+ DirectiveKillRating), faux si elle l'ignore (+ IgnoredDirectiveKillRating).
	 * Les directives restrictives (bouclier, arc, rechargement interdits) renvoient toujours vrai.
	 */
	UFUNCTION(BlueprintCallable, Category = "Directive")
	virtual bool IsKillRespected(const FKillInfo& Kill) const;

	/** Appelé à chaque élimination tant que la directive est active (« Pas de coupure ! » : recharge l'arme). */
	virtual void OnKill(UShootDirectorSubsystem* Director, const FKillInfo& Kill);

	/** Univers où la directive peut être tirée. Vide = tous. */
	UFUNCTION(BlueprintPure, Category = "Directive")
	bool IsAllowedIn(EUniverse Universe) const;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Directive")
	TArray<EUniverse> AllowedUniverses;

	/** Texte affiché par le HUD. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Directive")
	FText Title;

	/** Réplique jouée à l'annonce (ligne de DT_DirectorLines). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Directive")
	FName VoiceLineId;
};
