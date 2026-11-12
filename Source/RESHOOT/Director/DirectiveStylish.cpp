#include "Director/DirectiveStylish.h"
#include "Core/ReshootCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

bool UDirectiveStylish::IsKillRespected(const FKillInfo& Kill) const
{
	// Glissade, en l'air, ou boost en apesanteur.
	const AReshootCharacter* Player = GetPlayer();
	if (!Player)
	{
		return true;
	}
	const UCharacterMovementComponent* Move = Player->GetCharacterMovement();
	const bool bStylish = Player->IsSliding()
		|| Move->IsFalling()
		|| (Move->IsFlying() && Player->IsSprinting());
	return bStylish;
}
