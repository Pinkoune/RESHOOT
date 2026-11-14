#include "Director/DirectiveLaserCost.h"
#include "Core/ReshootCharacter.h"

void UDirectiveLaserCost::OnBegin(UShootDirectorSubsystem* Director)
{
	Super::OnBegin(Director);
	if (AReshootCharacter* Player = GetPlayer())
	{
		// Levé par UDirective::OnEnd (ClearWeaponRestrictions).
		Player->SetWeaponCostMultiplier(CostMultiplier);
	}
}
