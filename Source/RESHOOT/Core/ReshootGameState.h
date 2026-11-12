// RESHOOT — GameState : porte l'audimat et la voix du réalisateur.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "ReshootGameState.generated.h"

class UAudienceRating;
class UDirectorVoice;

UCLASS(Blueprintable)
class RESHOOT_API AReshootGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	AReshootGameState();

	UFUNCTION(BlueprintPure, Category = "Reshoot")
	UAudienceRating* GetAudienceRating() const { return AudienceRating; }

	UFUNCTION(BlueprintPure, Category = "Reshoot")
	UDirectorVoice* GetDirectorVoice() const { return DirectorVoice; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UAudienceRating> AudienceRating;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UDirectorVoice> DirectorVoice;
};
