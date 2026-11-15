// RESHOOT — GameMode : branche le joueur, le GameState et le script de tournage.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ReshootGameMode.generated.h"

class UShootScript;

UCLASS(Blueprintable)
class RESHOOT_API AReshootGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AReshootGameMode();

	/** Déroulé de la partie, transmis à UShootDirectorSubsystem au BeginPlay. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reshoot")
	TObjectPtr<UShootScript> ShootScript;

	/** Lance StartShoot() dès le BeginPlay (test sans menu). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reshoot")
	bool bAutoStartShoot = false;

	/** Avec bAutoStartShoot : saute le casting et lance directement la scène 1. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reshoot")
	bool bSkipCasting = true;

	/** Avec bAutoStartShoot : le tournage attend le menu principal du HUD (« ACTION! »). Une carte rouverte avec
	    l'option ?Play (Recommencer, Rejouer) saute le menu ; ?SkipCasting saute aussi le casting. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reshoot")
	bool bShowMainMenu = false;

	/** Vrai tant que le menu principal attend le joueur. */
	UFUNCTION(BlueprintPure, Category = "Reshoot")
	bool IsMainMenuPending() const { return bMainMenuPending; }

	/** Appelé par le menu principal : lance le casting (ou la scène 1 si bSkipCasting). */
	UFUNCTION(BlueprintCallable, Category = "Reshoot")
	void StartFromMainMenu();

protected:
	virtual void BeginPlay() override;

private:
	void StartShootFlow(bool bForceSkipCasting);

	bool bMainMenuPending = false;
};
