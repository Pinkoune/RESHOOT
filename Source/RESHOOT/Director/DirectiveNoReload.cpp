#include "Director/DirectiveNoReload.h"
#include "Director/ShootDirectorSubsystem.h"
#include "Core/ReshootCharacter.h"
#include "Core/ReshootWeapon.h"

#include "Engine/World.h"
#include "TimerManager.h"

void UDirectiveNoReload::OnBegin(UShootDirectorSubsystem* Director)
{
	Super::OnBegin(Director);
	bReloaded = false;
	WatchedWeapon.Reset();
	LastAmmo = -1;
	CheckAmmo();
	if (UWorld* World = Director ? Director->GetWorld() : nullptr)
	{
		World->GetTimerManager().SetTimer(AmmoTimer, FTimerDelegate::CreateUObject(this, &UDirectiveNoReload::CheckAmmo), AmmoCheckInterval, true);
	}
}

void UDirectiveNoReload::OnEnd(UShootDirectorSubsystem* Director)
{
	if (UWorld* World = Director ? Director->GetWorld() : nullptr)
	{
		World->GetTimerManager().ClearTimer(AmmoTimer);
	}
	Super::OnEnd(Director);
}

void UDirectiveNoReload::CheckAmmo()
{
	const AReshootCharacter* Player = GetPlayer();
	AReshootWeapon* Weapon = Player ? Player->GetCurrentWeapon() : nullptr;
	if (!Weapon || Weapon->GetMaxAmmo() < 0)
	{
		WatchedWeapon.Reset();
		LastAmmo = -1;
		return;
	}
	const int32 Ammo = Weapon->GetAmmo();
	// Même arme et munitions qui remontent = recharge. Un changement d'arme (d'univers) n'en est pas une.
	if (WatchedWeapon.Get() == Weapon && Ammo > LastAmmo)
	{
		bReloaded = true;
	}
	WatchedWeapon = Weapon;
	LastAmmo = Ammo;
}

float UDirectiveNoReload::EvaluateKill(const FKillInfo& Kill) const
{
	const AReshootCharacter* Player = GetPlayer();
	const AReshootWeapon* Weapon = Player ? Player->GetCurrentWeapon() : nullptr;
	if (!Weapon || Weapon->GetMaxAmmo() < 0)
	{
		return 1.f;
	}
	return Judge(!bReloaded);
}
