#include "Core/HealthComponent.h"
#include "RESHOOT.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"

UHealthComponent::UHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UHealthComponent::BeginPlay()
{
	Super::BeginPlay();
	Health = MaxHealth;
	bDead = false;
}

float UHealthComponent::ApplyDamage(float Damage, AActor* DamageInstigator, bool bHeadshot, bool bMelee)
{
	if (bDead || bInvulnerable || Damage <= 0.f)
	{
		return 0.f;
	}

	float FinalDamage = Damage;
	if (bHeadshot)
	{
		FinalDamage *= HeadshotMultiplier;
	}
	if (bFrontalBlock && IsInFrontCone(DamageInstigator))
	{
		FinalDamage *= FrontalBlockMultiplier;
	}

	const float OldHealth = Health;
	Health = FMath::Clamp(Health - FinalDamage, 0.f, MaxHealth);
	const float Applied = OldHealth - Health;

	if (Applied > 0.f)
	{
		OnHealthChanged.Broadcast(this, Health, -Applied);
	}

	if (Health <= 0.f && !bDead)
	{
		bDead = true;
		UE_LOG(LogReshoot, Verbose, TEXT("%s killed by %s"), *GetNameSafe(GetOwner()), *GetNameSafe(DamageInstigator));
		OnDeath.Broadcast(this, DamageInstigator, bHeadshot, bMelee);
	}

	return Applied;
}

void UHealthComponent::Heal(float Amount)
{
	if (bDead || Amount <= 0.f)
	{
		return;
	}
	const float OldHealth = Health;
	Health = FMath::Min(Health + Amount, MaxHealth);
	if (Health != OldHealth)
	{
		OnHealthChanged.Broadcast(this, Health, Health - OldHealth);
	}
}

void UHealthComponent::ResetHealth()
{
	const float OldHealth = Health;
	bDead = false;
	Health = MaxHealth;
	OnHealthChanged.Broadcast(this, Health, Health - OldHealth);
}

float UHealthComponent::GetHealthPct() const
{
	return MaxHealth > 0.f ? Health / MaxHealth : 0.f;
}

void UHealthComponent::SetFrontalBlock(bool bEnabled, float DamageMultiplier)
{
	bFrontalBlock = bEnabled;
	FrontalBlockMultiplier = FMath::Max(0.f, DamageMultiplier);
}

bool UHealthComponent::IsInFrontCone(const AActor* Other) const
{
	const AActor* Owner = GetOwner();
	if (!Owner || !Other)
	{
		return false;
	}

	// Pour un pion, le regard est la rotation du contrôleur (vue FPS) ; sinon l'orientation de l'acteur.
	FVector Forward = Owner->GetActorForwardVector();
	if (const APawn* Pawn = Cast<APawn>(Owner))
	{
		Forward = Pawn->GetControlRotation().Vector();
	}

	FVector ToOther = Other->GetActorLocation() - Owner->GetActorLocation();
	Forward.Z = 0.f;
	ToOther.Z = 0.f;
	if (!Forward.Normalize() || !ToOther.Normalize())
	{
		return false;
	}

	const float CosHalfAngle = FMath::Cos(FMath::DegreesToRadians(FrontalBlockHalfAngle));
	return FVector::DotProduct(Forward, ToOther) >= CosHalfAngle;
}
