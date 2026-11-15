#include "Core/ReshootGameMode.h"
#include "Core/ReshootCharacter.h"
#include "Core/ReshootGameState.h"
#include "Director/ShootDirectorSubsystem.h"
#include "Director/ShootScript.h"

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
		if (bAutoStartShoot)
		{
			if (bSkipCasting)
			{
				Director->StartShoot();
			}
			else
			{
				Director->StartCasting();
			}
		}
	}
}
