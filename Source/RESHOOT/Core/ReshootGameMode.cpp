#include "Core/ReshootGameMode.h"
#include "Core/ReshootCharacter.h"
#include "Core/ReshootGameState.h"
#include "Director/ShootDirectorSubsystem.h"
#include "Director/ShootScript.h"

AReshootGameMode::AReshootGameMode()
{
	DefaultPawnClass = AReshootCharacter::StaticClass();
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
