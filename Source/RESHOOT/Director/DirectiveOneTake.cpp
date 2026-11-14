#include "Director/DirectiveOneTake.h"
#include "Core/ReshootCharacter.h"
#include "Core/ReshootWeapon.h"
#include "Noir/NoirRevolver.h"

void UDirectiveOneTake::OnBegin(UShootDirectorSubsystem* Director)
{
	Super::OnBegin(Director);
	if (AReshootCharacter* Player = GetPlayer())
	{
		Player->SetWeaponActionAllowed(EWeaponAction::Reload, false);
	}
	// La recharge automatique du revolver (chargeur vide) passe par le revolver lui-même.
	SetRevolverReloadBlocked(true);
}

void UDirectiveOneTake::OnEnd(UShootDirectorSubsystem* Director)
{
	SetRevolverReloadBlocked(false);
	Super::OnEnd(Director);
}

void UDirectiveOneTake::OnKill(UShootDirectorSubsystem* Director, const FKillInfo& Kill)
{
	const AReshootCharacter* Player = GetPlayer();
	AReshootWeapon* Weapon = Player ? Player->GetCurrentWeapon() : nullptr;
	if (ANoirRevolver* Revolver = Cast<ANoirRevolver>(Weapon))
	{
		Revolver->RefillMagazine();
	}
	else if (Weapon)
	{
		Weapon->Refill();
	}
}

void UDirectiveOneTake::SetRevolverReloadBlocked(bool bBlocked) const
{
	const AReshootCharacter* Player = GetPlayer();
	if (ANoirRevolver* Revolver = Player ? Cast<ANoirRevolver>(Player->GetCurrentWeapon()) : nullptr)
	{
		Revolver->SetReloadBlocked(bBlocked);
	}
}
