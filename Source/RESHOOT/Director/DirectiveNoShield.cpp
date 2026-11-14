#include "Director/DirectiveNoShield.h"
#include "Director/ShootDirectorSubsystem.h"
#include "Core/ReshootCharacter.h"
#include "Medieval/MedievalSword.h"

#include "Engine/World.h"
#include "TimerManager.h"

void UDirectiveNoShield::OnBegin(UShootDirectorSubsystem* Director)
{
	Super::OnBegin(Director);
	if (AReshootCharacter* Player = GetPlayer())
	{
		Player->SetWeaponActionAllowed(EWeaponAction::Secondary, false);
	}
	if (UWorld* World = Director ? Director->GetWorld() : nullptr)
	{
		World->GetTimerManager().SetTimer(EnforceTimer, FTimerDelegate::CreateUObject(this, &UDirectiveNoShield::EnforceShieldDown), 0.05f, true);
	}
}

void UDirectiveNoShield::OnEnd(UShootDirectorSubsystem* Director)
{
	if (UWorld* World = Director ? Director->GetWorld() : nullptr)
	{
		World->GetTimerManager().ClearTimer(EnforceTimer);
	}
	Super::OnEnd(Director);
}

void UDirectiveNoShield::EnforceShieldDown()
{
	const AReshootCharacter* Player = GetPlayer();
	if (AMedievalSword* Sword = Player ? Cast<AMedievalSword>(Player->GetCurrentWeapon()) : nullptr)
	{
		if (Sword->IsShieldRaised())
		{
			Sword->StopSecondary();
		}
	}
}
