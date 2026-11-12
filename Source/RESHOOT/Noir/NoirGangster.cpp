#include "Noir/NoirGangster.h"
#include "Core/HealthComponent.h"
#include "Core/ReshootCharacter.h"
#include "RESHOOT.h"

#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	/** Forme simple attachée à la capsule. Bloque seulement le canal Visibility si bShootable. */
	UStaticMeshComponent* MakePart(ANoirGangster* Owner, USceneComponent* Parent, FName Name, UStaticMesh* Mesh,
		const FVector& Location, const FVector& Scale, bool bShootable)
	{
		UStaticMeshComponent* Part = Owner->CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Part->SetupAttachment(Parent);
		Part->SetStaticMesh(Mesh);
		Part->SetRelativeLocation(Location);
		Part->SetRelativeScale3D(Scale);
		Part->SetCanEverAffectNavigation(false);
		if (bShootable)
		{
			Part->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			Part->SetCollisionResponseToAllChannels(ECR_Ignore);
			Part->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
		}
		else
		{
			Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
		return Part;
	}
}

ANoirGangster::ANoirGangster(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Universe = EUniverse::Noir;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));

	// Capsule de 176 cm (demi-hauteur 88). Formes de base : 100 cm de diamètre et de haut, pivot au centre.
	USceneComponent* Root = GetCapsuleComponent();
	Body = MakePart(this, Root, TEXT("Body"), CylinderMesh.Object, FVector(0.f, 0.f, -20.f), FVector(0.6f, 0.6f, 1.36f), true);
	Head = MakePart(this, Root, TEXT("Head"), SphereMesh.Object, FVector(0.f, 0.f, 66.f), FVector(0.4f), true);
	Head->ComponentTags.Add(TEXT("head"));
	HatBrim = MakePart(this, Root, TEXT("HatBrim"), CylinderMesh.Object, FVector(0.f, 0.f, 82.f), FVector(0.7f, 0.7f, 0.04f), false);
	HatCrown = MakePart(this, Root, TEXT("HatCrown"), CylinderMesh.Object, FVector(0.f, 0.f, 92.f), FVector(0.38f, 0.38f, 0.2f), false);
}

void ANoirGangster::BeginPlay()
{
	Super::BeginPlay();

	const float FirstDelay = FireInterval + FMath::FRandRange(0.f, FirstShotMaxDelay);
	GetWorldTimerManager().SetTimer(FireTimer, this, &ANoirGangster::Fire, FireInterval, true, FirstDelay);

	// Spawné pendant un CUT! : le timer démarre en pause.
	if (IsFrozen())
	{
		GetWorldTimerManager().PauseTimer(FireTimer);
	}
}

void ANoirGangster::OnFreezeChanged(bool bFreeze)
{
	Super::OnFreezeChanged(bFreeze);

	if (bFreeze)
	{
		GetWorldTimerManager().PauseTimer(FireTimer);
	}
	else
	{
		GetWorldTimerManager().UnPauseTimer(FireTimer);
	}
}

void ANoirGangster::OnDeath(AActor* Killer, bool bHeadshot, bool bMelee)
{
	GetWorldTimerManager().ClearTimer(FireTimer);
	// Le corps ne doit plus arrêter les balles pendant qu'il disparaît.
	Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Head->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Super::OnDeath(Killer, bHeadshot, bMelee);
}

void ANoirGangster::Fire()
{
	if (IsFrozen() || IsDead())
	{
		return;
	}

	AReshootCharacter* Player = GetTargetPlayer();
	UHealthComponent* PlayerHealth = Player ? Player->GetHealth() : nullptr;
	if (!PlayerHealth || PlayerHealth->IsDead())
	{
		return;
	}

	const FVector Start = GetPawnViewLocation();
	const FVector Target = Player->GetActorLocation();
	if (FVector::DistSquared(Start, Target) > FMath::Square(Range))
	{
		return;
	}

	// Ligne de vue : rien entre le gangster et le joueur.
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(NoirGangsterSight), true, this);
	const bool bBlocked = GetWorld()->LineTraceSingleByChannel(Hit, Start, Target, ECC_Visibility, Params) && Hit.GetActor() != Player;
	if (bBlocked)
	{
		return;
	}

	// Se tourne vers le joueur (lacet seulement).
	FRotator Facing = (Target - GetActorLocation()).Rotation();
	Facing.Pitch = 0.f;
	Facing.Roll = 0.f;
	SetActorRotation(Facing);

	if (FireSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, FireSound, Start);
	}

	const bool bHits = FMath::FRand() < HitChance;

	if (bDrawShotTrace)
	{
		const FVector Miss = Target + FMath::VRand() * 150.f;
		DrawDebugLine(GetWorld(), Start, bHits ? Target : Miss, FColor::Red, false, 0.15f, 0, 1.f);
	}

	if (bHits)
	{
		PlayerHealth->ApplyDamage(Damage, this);
	}
}
