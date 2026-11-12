#include "Noir/NoirRevolver.h"
#include "Core/ReshootCharacter.h"
#include "RESHOOT.h"

#include "Camera/CameraComponent.h"
#include "Components/PointLightComponent.h"
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

void ANoirRevolver::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Changement d'arme pendant la visée : on rend le champ de vision normal au joueur.
	SetAiming(false);
	Super::EndPlay(EndPlayReason);
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

void ANoirRevolver::StartSecondary()
{
	Super::StartSecondary();
	if (IsEnabled())
	{
		SetAiming(true);
	}
}

void ANoirRevolver::StopSecondary()
{
	Super::StopSecondary();
	SetAiming(false);
}

void ANoirRevolver::SetAiming(bool bAim)
{
	if (bAiming == bAim)
	{
		return;
	}
	const AReshootCharacter* Character = GetOwnerCharacter();
	UCameraComponent* Camera = Character ? Character->GetFirstPersonCamera() : nullptr;
	if (!Camera)
	{
		return;
	}
	if (bAim)
	{
		DefaultFieldOfView = Camera->FieldOfView;
		Camera->SetFieldOfView(AimFieldOfView);
	}
	else
	{
		Camera->SetFieldOfView(DefaultFieldOfView);
	}
	bAiming = bAim;
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

	FVector Start, Direction;
	GetAim(Start, Direction);
	SpawnMuzzleFlash(this, Start + Direction * 60.f, MuzzleFlashIntensity, MuzzleFlashDuration);

	FHitResult Hit;
	const bool bHit = TraceFromAim(Range, Hit);

	if (bDrawShotTrace)
	{
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
	if (!IsEnabled() || bReloading || bReloadBlocked || CurrentAmmo >= MagazineSize)
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

void ANoirRevolver::RefillMagazine()
{
	GetWorldTimerManager().ClearTimer(ReloadTimer);
	bReloading = false;
	CurrentAmmo = MagazineSize;
}

void ANoirRevolver::SetReloadBlocked(bool bBlocked)
{
	bReloadBlocked = bBlocked;
	if (bBlocked && bReloading)
	{
		GetWorldTimerManager().ClearTimer(ReloadTimer);
		bReloading = false;
	}
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

void ANoirRevolver::SpawnMuzzleFlash(AActor* Owner, const FVector& Location, float Intensity, float Duration)
{
	if (!Owner || Intensity <= 0.f || Duration <= 0.f)
	{
		return;
	}
	UWorld* World = Owner->GetWorld();
	if (!World)
	{
		return;
	}

	UPointLightComponent* Flash = NewObject<UPointLightComponent>(Owner);
	Flash->SetMobility(EComponentMobility::Movable);
	Flash->SetIntensityUnits(ELightUnits::Candelas);
	Flash->SetIntensity(Intensity);
	Flash->SetAttenuationRadius(1500.f);
	Flash->SetLightColor(FLinearColor(1.f, 0.85f, 0.6f));
	Flash->SetCastShadows(false);
	Flash->SetWorldLocation(Location);
	Flash->RegisterComponent();

	TWeakObjectPtr<UPointLightComponent> WeakFlash = Flash;
	FTimerHandle Handle;
	World->GetTimerManager().SetTimer(Handle, FTimerDelegate::CreateLambda([WeakFlash]()
	{
		if (WeakFlash.IsValid())
		{
			WeakFlash->DestroyComponent();
		}
	}), Duration, false);
}
