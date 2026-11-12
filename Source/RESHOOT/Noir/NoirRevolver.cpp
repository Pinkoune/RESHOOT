#include "Noir/NoirRevolver.h"
#include "RESHOOT.h"

#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

ANoirRevolver::ANoirRevolver()
{
	DisplayName = NSLOCTEXT("Reshoot", "NoirRevolverName", "Revolver");
}

void ANoirRevolver::BeginPlay()
{
	Super::BeginPlay();
	CurrentAmmo = MagazineSize;
}

void ANoirRevolver::StartPrimary()
{
	Super::StartPrimary();
	Fire();
}

void ANoirRevolver::StopPrimary()
{
	Super::StopPrimary();
}

void ANoirRevolver::Fire()
{
	if (!IsEnabled() || bReloading)
	{
		return;
	}

	const double Now = GetWorld()->GetTimeSeconds();
	if (Now - LastFireTime < FireInterval)
	{
		return;
	}

	if (CurrentAmmo <= 0)
	{
		if (EmptySound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, EmptySound, GetActorLocation());
		}
		Reload();
		return;
	}

	LastFireTime = Now;
	--CurrentAmmo;

	if (FireSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, FireSound, GetActorLocation());
	}

	FHitResult Hit;
	const bool bHit = TraceFromAim(Range, Hit);

	if (bDrawShotTrace)
	{
		FVector Start, Direction;
		GetAim(Start, Direction);
		const FVector End = bHit ? Hit.ImpactPoint : Start + Direction * Range;
		DrawDebugLine(GetWorld(), Start + Direction * 30.f, End, FColor::White, false, 0.15f, 0, 1.f);
	}

	if (bHit)
	{
		DealDamage(Hit.GetActor(), Damage, IsHeadshot(Hit), false);
	}

	// Chargeur vide : recharge automatique.
	if (CurrentAmmo <= 0)
	{
		Reload();
	}
}

void ANoirRevolver::Reload()
{
	if (!IsEnabled() || bReloading || CurrentAmmo >= MagazineSize)
	{
		return;
	}

	bReloading = true;
	if (ReloadSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ReloadSound, GetActorLocation());
	}
	GetWorldTimerManager().SetTimer(ReloadTimer, this, &ANoirRevolver::FinishReload, FMath::Max(ReloadTime, 0.01f), false);
}

void ANoirRevolver::FinishReload()
{
	bReloading = false;
	CurrentAmmo = MagazineSize;
}

void ANoirRevolver::SetEnabled(bool bInEnabled)
{
	Super::SetEnabled(bInEnabled);

	// Gel ou changement d'univers pendant une recharge : on l'annule, elle sera relancée par R ou au prochain tir à vide.
	if (!bInEnabled && bReloading)
	{
		GetWorldTimerManager().ClearTimer(ReloadTimer);
		bReloading = false;
	}
}
