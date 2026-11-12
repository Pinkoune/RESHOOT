#include "Director/Directive.h"
#include "Director/ShootDirectorSubsystem.h"
#include "Core/ReshootCharacter.h"

#include "Kismet/GameplayStatics.h"

void UDirective::OnBegin(UShootDirectorSubsystem* Director)
{
	DirectorPtr = Director;
	PlayerPtr = Director ? Cast<AReshootCharacter>(UGameplayStatics::GetPlayerPawn(Director, 0)) : nullptr;
}

void UDirective::OnEnd(UShootDirectorSubsystem* Director)
{
	// Les directives restrictives (bouclier, arc, recharge, laser) passent par le socle : on lève tout en sortant.
	if (AReshootCharacter* Player = PlayerPtr.Get())
	{
		Player->ClearWeaponRestrictions();
	}
	PlayerPtr.Reset();
	DirectorPtr.Reset();
}

bool UDirective::IsKillRespected(const FKillInfo& Kill) const
{
	return true;
}

void UDirective::OnKill(UShootDirectorSubsystem* Director, const FKillInfo& Kill)
{
}

bool UDirective::IsAllowedIn(EUniverse Universe) const
{
	return AllowedUniverses.Num() == 0 || AllowedUniverses.Contains(Universe);
}
