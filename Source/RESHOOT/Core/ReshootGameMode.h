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

protected:
	virtual void BeginPlay() override;
};
