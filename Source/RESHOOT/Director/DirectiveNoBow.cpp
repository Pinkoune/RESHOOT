#include "Director/DirectiveNoBow.h"
#include "Director/ShootDirectorSubsystem.h"
#include "Core/ReshootCharacter.h"
#include "Medieval/MedievalSword.h"

#include "Engine/World.h"
#include "TimerManager.h"

void UDirectiveNoBow::OnBegin(UShootDirectorSubsystem* Director)
{
	Super::OnBegin(Director);
	if (AReshootCharacter* Player = GetPlayer())
	{
		Player->SetWeaponActionAllowed(EWeaponAction::SwitchMode, false);
	}
	EnforceNoBow();
	if (UWorld* World = Director ? Director->GetWorld() : nullptr)
	{
		World->GetTimerManager().SetTimer(EnforceTimer, FTimerDelegate::CreateUObject(this, &UDirectiveNoBow::EnforceNoBow), 0.05f, true);
	}
}

void UDirectiveNoBow::OnEnd(UShootDirectorSubsystem* Director)
{
	if (UWorld* World = Director ? Director->GetWorld() : nullptr)
	{
		World->GetTimerManager().ClearTimer(EnforceTimer);
	}
	Super::OnEnd(Director);
}

void UDirectiveNoBow::EnforceNoBow()
{
	const AReshootCharacter* Player = GetPlayer();
	if (AMedievalSword* Sword = Player ? Cast<AMedievalSword>(Player->GetCurrentWeapon()) : nullptr)
	{
		if (Sword->GetMedievalMode() == EMedievalWeaponMode::Bow)
		{
			Sword->SetMode(EMedievalWeaponMode::Sword);
		}
	}
}
