#include "Medieval/MedievalAnimatedLook.h"
#include "Core/HealthComponent.h"
#include "RESHOOT.h"

#include "Animation/AnimSequenceBase.h"
#include "Animation/AnimationAsset.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "TimerManager.h"

namespace
{
	float AnimLength(const UAnimationAsset* Anim)
	{
		const UAnimSequenceBase* Sequence = Cast<UAnimSequenceBase>(Anim);
		return Sequence ? Sequence->GetPlayLength() : 0.f;
	}
}

UMedievalAnimatedLook::UMedievalAnimatedLook()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UMedievalAnimatedLook::BeginPlay()
{
	Super::BeginPlay();
	if (UHealthComponent* Health = GetOwner()->FindComponentByClass<UHealthComponent>())
	{
		Health->OnHealthChanged.AddDynamic(this, &UMedievalAnimatedLook::HandleHealthChanged);
	}
}

void UMedievalAnimatedLook::HandleHealthChanged(UHealthComponent* HealthComponent, float NewHealth, float Delta)
{
	if (Delta < 0.f && NewHealth > 0.f)
	{
		PlayHit();
	}
}

void UMedievalAnimatedLook::SetGrayBoxParts(const TArray<UPrimitiveComponent*>& Parts)
{
	GrayBoxParts.Reset();
	for (UPrimitiveComponent* Part : Parts)
	{
		if (Part)
		{
			GrayBoxParts.Add(Part);
		}
	}
}

void UMedievalAnimatedLook::SetReal(bool bReal)
{
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	USkeletalMeshComponent* Body = Character ? Character->GetMesh() : nullptr;
	if (!Body || bDead)
	{
		return;
	}

	const bool bShowReal = bReal && RealMesh;
	if (bShowReal == bShowingReal)
	{
		return;
	}
	bShowingReal = bShowReal;

	for (UPrimitiveComponent* Part : GrayBoxParts)
	{
		if (Part)
		{
			Part->SetVisibility(!bShowReal, true);
		}
	}

	if (bShowReal)
	{
		Body->SetSkeletalMesh(RealMesh);
		Body->SetRelativeTransform(RealMeshTransform);
		Body->SetVisibility(true, true);
		Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		if (HeldWeapon)
		{
			if (!HeldWeaponComponent)
			{
				HeldWeaponComponent = NewObject<UStaticMeshComponent>(GetOwner(), TEXT("HeldWeapon"));
				HeldWeaponComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				HeldWeaponComponent->RegisterComponent();
			}
			HeldWeaponComponent->SetStaticMesh(HeldWeapon);
			HeldWeaponComponent->AttachToComponent(Body, FAttachmentTransformRules::SnapToTargetNotIncludingScale, HeldWeaponSocket);
			HeldWeaponComponent->SetRelativeTransform(HeldWeaponTransform);
			HeldWeaponComponent->SetVisibility(true);
		}

		CurrentLoop = nullptr;
		OneShotEndTime = 0.0;
		UpdateLocomotion();
		GetWorld()->GetTimerManager().SetTimer(LocomotionTimer, this, &UMedievalAnimatedLook::UpdateLocomotion, 0.1f, true);
	}
	else
	{
		GetWorld()->GetTimerManager().ClearTimer(LocomotionTimer);
		Body->Stop();
		Body->SetVisibility(false, true);
		if (HeldWeaponComponent)
		{
			HeldWeaponComponent->SetVisibility(false);
		}
	}
}

void UMedievalAnimatedLook::UpdateLocomotion()
{
	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (!bShowingReal || bDead || !Character)
	{
		return;
	}
	// Une attaque ou un coup reçu est en cours : on la laisse finir.
	if (GetWorld()->GetTimeSeconds() < OneShotEndTime)
	{
		return;
	}

	const float Speed = Character->GetVelocity().Size2D();
	if (Speed > MoveThreshold && RunAnim)
	{
		PlayLoop(RunAnim, FMath::Clamp(Speed / RunAnimReferenceSpeed, 0.5f, 2.f));
	}
	else if (IdleAnim)
	{
		PlayLoop(IdleAnim, 1.f);
	}
}

void UMedievalAnimatedLook::PlayLoop(UAnimationAsset* Anim, float Rate)
{
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	USkeletalMeshComponent* Body = Character ? Character->GetMesh() : nullptr;
	if (!Body || !Anim)
	{
		return;
	}
	if (CurrentLoop != Anim)
	{
		CurrentLoop = Anim;
		Body->PlayAnimation(Anim, true);
	}
	Body->SetPlayRate(Rate);
}

void UMedievalAnimatedLook::PlayOneShot(UAnimationAsset* Anim, float Rate)
{
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	USkeletalMeshComponent* Body = Character ? Character->GetMesh() : nullptr;
	if (!bShowingReal || bDead || !Body || !Anim)
	{
		return;
	}
	CurrentLoop = nullptr;
	Body->PlayAnimation(Anim, false);
	Body->SetPlayRate(Rate);
	OneShotEndTime = GetWorld()->GetTimeSeconds() + AnimLength(Anim) / FMath::Max(Rate, 0.05f);
}

void UMedievalAnimatedLook::PlayAttack()
{
	PlayOneShot(AttackAnim, AttackPlayRate);
}

void UMedievalAnimatedLook::PlayHit()
{
	// Un coup reçu n'interrompt pas une attaque.
	if (GetWorld()->GetTimeSeconds() < OneShotEndTime)
	{
		return;
	}
	PlayOneShot(HitAnim, 1.f);
}

bool UMedievalAnimatedLook::PlayDeath(const FVector& AwayFromKiller, float Impulse)
{
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	USkeletalMeshComponent* Body = Character ? Character->GetMesh() : nullptr;
	if (!bShowingReal || bDead || !Body)
	{
		return false;
	}
	GetWorld()->GetTimerManager().ClearTimer(LocomotionTimer);

	if (DeathAnim)
	{
		CurrentLoop = nullptr;
		Body->PlayAnimation(DeathAnim, false);
		Body->SetPlayRate(1.f);
	}
	else if (Body->GetPhysicsAsset())
	{
		// Poupée de chiffon projetée loin du tueur.
		Body->SetCollisionProfileName(TEXT("Ragdoll"));
		Body->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
		Body->SetSimulatePhysics(true);
		Body->AddImpulse((AwayFromKiller + FVector(0.f, 0.f, 0.4f)) * Impulse, NAME_None, true);
	}
	bDead = true;
	return true;
}

void UMedievalAnimatedLook::SetFrozen(bool bFrozen)
{
	// Les animations sont gelées par CustomTimeDilation (AReshootEnemy::Freeze) ; on suspend le choix de la boucle
	// et on garde le temps restant de l'attaque en cours.
	FTimerManager& Timers = GetWorld()->GetTimerManager();
	const double Now = GetWorld()->GetTimeSeconds();
	if (bFrozen)
	{
		Timers.PauseTimer(LocomotionTimer);
		OneShotEndTime = FMath::Max(0.0, OneShotEndTime - Now); // temps restant
	}
	else
	{
		Timers.UnPauseTimer(LocomotionTimer);
		OneShotEndTime = OneShotEndTime > 0.0 ? Now + OneShotEndTime : 0.0;
	}
}

void UMedievalAnimatedLook::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(LocomotionTimer);
	}
	Super::EndPlay(EndPlayReason);
}
