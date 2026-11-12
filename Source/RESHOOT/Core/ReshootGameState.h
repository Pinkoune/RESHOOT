// RESHOOT — GameState : porte l'audimat et la voix du réalisateur.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "Core/ReshootTypes.h"
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

	/** Nom saisi au casting, affiché sur l'affiche de fin. */
	UFUNCTION(BlueprintCallable, Category = "Reshoot")
	void SetPlayerName(const FString& InName) { PlayerName = InName; }

	UFUNCTION(BlueprintPure, Category = "Reshoot")
	const FString& GetPlayerName() const { return PlayerName; }

	/** Bilan pour l'écran de fin. */
	UFUNCTION(BlueprintPure, Category = "Reshoot")
	const FShootStats& GetShootStats() const { return Stats; }

	/** Écrit par UShootDirectorSubsystem seulement. */
	FShootStats& GetMutableShootStats() { return Stats; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UAudienceRating> AudienceRating;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UDirectorVoice> DirectorVoice;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reshoot")
	FString PlayerName = TEXT("Figurant");

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Reshoot")
	FShootStats Stats;
};
