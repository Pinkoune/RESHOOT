#include "Core/ReshootWeapon.h"
#include "Core/ReshootCharacter.h"
#include "Core/HealthComponent.h"
#include "RESHOOT.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"

namespace ReshootTrace
{
	/** Le profil « Pawn » ignore le canal Visibility : on trace par type d'objet pour toucher monde et personnages. */
	static FCollisionObjectQueryParams HitObjectTypes()
	{
		FCollisionObjectQueryParams Types;
		Types.AddObjectTypesToQuery(ECC_WorldStatic);
		Types.AddObjectTypesToQuery(ECC_WorldDynamic);
		Types.AddObjectTypesToQuery(ECC_Pawn);
		Types.AddObjectTypesToQuery(ECC_PhysicsBody);
		Types.AddObjectTypesToQuery(ECC_Destructible);
		return Types;
	}
}

AReshootWeapon::AReshootWeapon()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Root);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCastShadow(false);
}

void AReshootWeapon::BeginPlay()
{
	Super::BeginPlay();
}

void AReshootWeapon::StartPrimary()
{
	bPrimaryHeld = true;
}

void AReshootWeapon::StopPrimary()
{
	bPrimaryHeld = false;
}

void AReshootWeapon::StartSecondary()
{
	if (!bSecondaryHeld)
	{
		bSecondaryHeld = true;
		BroadcastWeaponEvent(EWeaponEvent::SecondaryStarted);
	}
}

void AReshootWeapon::StopSecondary()
{
	if (bSecondaryHeld)
	{
		bSecondaryHeld = false;
		BroadcastWeaponEvent(EWeaponEvent::SecondaryStopped);
	}
}

void AReshootWeapon::Reload()
{
}

void AReshootWeapon::SetEnabled(bool bInEnabled)
{
	bEnabled = bInEnabled;
	if (!bEnabled)
	{
		StopPrimary();
		StopSecondary();
	}
	if (bHideWhenDisabled)
	{
		SetActorHiddenInGame(!bEnabled);
	}
}

AReshootCharacter* AReshootWeapon::GetOwnerCharacter() const
{
	return Cast<AReshootCharacter>(GetOwner());
}

void AReshootWeapon::GetAim(FVector& OutStart, FVector& OutDirection) const
{
	if (const AReshootCharacter* Character = GetOwnerCharacter())
	{
		if (const UCameraComponent* Camera = Character->GetFirstPersonCamera())
		{
			OutStart = Camera->GetComponentLocation();
			OutDirection = Camera->GetForwardVector();
			return;
		}
	}
	OutStart = GetActorLocation();
	OutDirection = GetActorForwardVector();
}

bool AReshootWeapon::TraceFromAim(float Range, FHitResult& OutHit) const
{
	FVector Start, Direction;
	GetAim(Start, Direction);

	FCollisionQueryParams Params(SCENE_QUERY_STAT(ReshootWeaponTrace), true, this);
	Params.AddIgnoredActor(GetOwner());
	return GetWorld()->LineTraceSingleByObjectType(OutHit, Start, Start + Direction * Range, ReshootTrace::HitObjectTypes(), Params);
}

bool AReshootWeapon::IsHeadshot(const FHitResult& Hit) const
{
	if (HeadBoneOrTag.IsNone())
	{
		return false;
	}
	if (Hit.BoneName == HeadBoneOrTag)
	{
		return true;
	}
	const UPrimitiveComponent* HitComponent = Hit.GetComponent();
	if (HitComponent && HitComponent->ComponentHasTag(HeadBoneOrTag))
	{
		return true;
	}
	// Les tirs s'arrêtent sur la capsule des personnages (elle englobe le squelette) :
	// le quart supérieur de la capsule compte comme la tête.
	if (const UCapsuleComponent* Capsule = Cast<UCapsuleComponent>(HitComponent))
	{
		const float HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
		return Hit.ImpactPoint.Z >= Capsule->GetComponentLocation().Z + HalfHeight * 0.5f;
	}
	return false;
}

float AReshootWeapon::DealDamage(AActor* Target, float Damage, bool bHeadshot, bool bMelee)
{
	if (!Target)
	{
		return 0.f;
	}
	UHealthComponent* Health = Target->FindComponentByClass<UHealthComponent>();
	if (!Health)
	{
		return 0.f;
	}
	const float Applied = Health->ApplyDamage(Damage, GetOwner(), bHeadshot, bMelee);
	if (Applied > 0.f)
	{
		BroadcastWeaponEvent(EWeaponEvent::HitConfirmed);
	}
	return Applied;
}

// ---------------------------------------------------------------------------
// Modes, restrictions, événements
// ---------------------------------------------------------------------------

void AReshootWeapon::SelectMode(int32 Index)
{
	// Une seule arme / un seul mode par défaut. Les sous-classes à plusieurs modes surchargent
	// et appellent BroadcastWeaponEvent(EWeaponEvent::ModeChanged).
}

void AReshootWeapon::CycleMode(int32 Direction)
{
	const int32 Count = GetModeCount();
	if (Count <= 1 || Direction == 0)
	{
		return;
	}
	const int32 Next = ((GetMode() + (Direction > 0 ? 1 : -1)) % Count + Count) % Count;
	SelectMode(Next);
}

float AReshootWeapon::GetCostMultiplier() const
{
	const AReshootCharacter* Character = GetOwnerCharacter();
	return Character ? Character->GetWeaponCostMultiplier() : 1.f;
}

bool AReshootWeapon::IsActionAllowed(EWeaponAction Action) const
{
	const AReshootCharacter* Character = GetOwnerCharacter();
	return !Character || Character->IsWeaponActionAllowed(Action);
}

void AReshootWeapon::BroadcastWeaponEvent(EWeaponEvent Event)
{
	OnWeaponEvent.Broadcast(this, Event);
}
