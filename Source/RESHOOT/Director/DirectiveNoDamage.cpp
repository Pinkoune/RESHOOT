#include "Director/DirectiveNoDamage.h"
#include "Core/ReshootCharacter.h"
#include "Core/HealthComponent.h"

UHealthComponent* UDirectiveNoDamage::GetPlayerHealth() const
{
	const AReshootCharacter* Player = GetPlayer();
	return Player ? Player->GetHealth() : nullptr;
}

void UDirectiveNoDamage::OnBegin(UShootDirectorSubsystem* Director)
{
	Super::OnBegin(Director);
	bHit = false;
	if (UHealthComponent* Health = GetPlayerHealth())
	{
		Health->OnHealthChanged.AddUniqueDynamic(this, &UDirectiveNoDamage::HandleHealthChanged);
	}
}

void UDirectiveNoDamage::OnEnd(UShootDirectorSubsystem* Director)
{
	if (UHealthComponent* Health = GetPlayerHealth())
	{
		Health->OnHealthChanged.RemoveDynamic(this, &UDirectiveNoDamage::HandleHealthChanged);
	}
	Super::OnEnd(Director);
}

void UDirectiveNoDamage::HandleHealthChanged(UHealthComponent* HealthComponent, float NewHealth, float Delta)
{
	if (Delta < 0.f)
	{
		bHit = true;
	}
}

float UDirectiveNoDamage::EvaluateKill(const FKillInfo& Kill) const
{
	return Judge(!bHit);
}
