#include "Core/ReshootGameMode.h"
#include "Core/ReshootCharacter.h"
#include "Core/ReshootGameState.h"
#include "Director/ShootDirectorSubsystem.h"
#include "Director/ShootScript.h"

#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

AReshootGameMode::AReshootGameMode()
{
	// Pion joueur : BP_ReshootCharacter (assets du joueur, dont les sons de réacteur) s'il existe, sinon la classe C++.
	// Les game modes Blueprint qui héritent de celui-ci (BP_StageGameMode...) le reprennent sans rien régler.
	static ConstructorHelpers::FClassFinder<AReshootCharacter> PlayerBlueprint(TEXT("/Game/RESHOOT/Core/Blueprints/BP_ReshootCharacter"));
	DefaultPawnClass = PlayerBlueprint.Succeeded() ? PlayerBlueprint.Class.Get() : AReshootCharacter::StaticClass();
	GameStateClass = AReshootGameState::StaticClass();
}

void AReshootGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (UShootDirectorSubsystem* Director = GetWorld()->GetSubsystem<UShootDirectorSubsystem>())
	{
		Director->SetShootScript(ShootScript);
	}
	const bool bPlayOption = UGameplayStatics::HasOption(OptionsString, TEXT("Play"));
	const bool bSkipOption = UGameplayStatics::HasOption(OptionsString, TEXT("SkipCasting"));
	if (bAutoStartShoot && bShowMainMenu && !bPlayOption)
	{
		bMainMenuPending = true;	// le HUD affiche le menu et appellera StartFromMainMenu()
	}
	else if (bAutoStartShoot || bPlayOption)
	{
		StartShootFlow(bSkipOption);
	}
}

void AReshootGameMode::StartFromMainMenu()
{
	if (bMainMenuPending)
	{
		bMainMenuPending = false;
		StartShootFlow(false);
	}
}

void AReshootGameMode::StartShootFlow(bool bForceSkipCasting)
{
	if (UShootDirectorSubsystem* Director = GetWorld()->GetSubsystem<UShootDirectorSubsystem>())
	{
		if (bSkipCasting || bForceSkipCasting)
		{
			Director->StartShoot();
		}
		else
		{
			Director->StartCasting();
		}
	}
}
