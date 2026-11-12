#include "Medieval/MedievalArrow.h"
#include "Medieval/MedievalSword.h"
#include "Core/HealthComponent.h"
#include "Core/ReshootCharacter.h"
#include "Core/ReshootEnemy.h"
#include "Director/ShootDirectorSubsystem.h"
#include "RESHOOT.h"

#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

AMedievalArrow::AMedievalArrow()
{
	PrimaryActorTick.bCanEverTick = false;
	InitialLifeSpan = 8.f;

	// Décor = bloque (la flèche se plante). Pions = chevauchement (on choisit qui on blesse, les autres sont traversés).
	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	Collision->InitSphereRadius(6.f);
	Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Collision->SetCollisionObjectType(ECC_WorldDynamic);
	Collision->SetCollisionResponseToAllChannels(ECR_Ignore);
	Collision->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	Collision->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	Collision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Collision->SetGenerateOverlapEvents(true);
	Collision->CanCharacterStepUpOn = ECB_No;
	SetRootComponent(Collision);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Collision);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCastShadow(false);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderMesh.Succeeded())
	{
		Mesh->SetStaticMesh(CylinderMesh.Object);
	}
	// Cylindre couché sur l'axe X, pointe à l'avant de la sphère.
	Mesh->SetRelativeRotation(FRotator(-90.f, 0.f, 0.f));
	Mesh->SetRelativeLocation(FVector(-25.f, 0.f, 0.f));
	Mesh->SetRelativeScale3D(FVector(0.04f, 0.04f, 0.7f));

	Movement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Movement"));
	Movement->UpdatedComponent = Collision;
	Movement->InitialSpeed = 0.f;
	Movement->MaxSpeed = 0.f; // Pas de limite.
	Movement->bRotationFollowsVelocity = true;
	Movement->bShouldBounce = false;
	Movement->ProjectileGravityScale = 0.f;
	Movement->bAutoActivate = false;
}

void AMedievalArrow::BeginPlay()
{
	Super::BeginPlay();

	if (AActor* Shooter = GetInstigator())
	{
		Collision->IgnoreActorWhenMoving(Shooter, true);
	}
	if (GetOwner() && GetOwner() != GetInstigator())
	{
		Collision->IgnoreActorWhenMoving(GetOwner(), true);
	}

	Collision->OnComponentHit.AddDynamic(this, &AMedievalArrow::HandleHit);
	Collision->OnComponentBeginOverlap.AddDynamic(this, &AMedievalArrow::HandleOverlap);

	if (UShootDirectorSubsystem* Director = GetWorld()->GetSubsystem<UShootDirectorSubsystem>())
	{
		Director->OnCut.AddDynamic(this, &AMedievalArrow::HandleCut);
		Director->OnAction.AddDynamic(this, &AMedievalArrow::HandleAction);
		if (Director->GetPhase() == EShootPhase::Cut)
		{
			HandleCut();
		}
	}
}

void AMedievalArrow::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		if (UShootDirectorSubsystem* Director = World->GetSubsystem<UShootDirectorSubsystem>())
		{
			Director->OnCut.RemoveDynamic(this, &AMedievalArrow::HandleCut);
			Director->OnAction.RemoveDynamic(this, &AMedievalArrow::HandleAction);
		}
	}
	Super::EndPlay(EndPlayReason);
}

void AMedievalArrow::Launch(const FVector& Direction, float Speed, float InDamage, float GravityScale)
{
	Damage = InDamage;
	Movement->ProjectileGravityScale = GravityScale;
	Movement->Velocity = Direction.GetSafeNormal() * Speed;
	Movement->Activate(true);
}

bool AMedievalArrow::CanDamage(const AActor* Target) const
{
	if (!Target || Target == GetInstigator() || Target == GetOwner())
	{
		return false;
	}
	const UHealthComponent* Health = Target->FindComponentByClass<UHealthComponent>();
	if (!Health || Health->IsDead())
	{
		return false;
	}
	// Flèche d'un figurant : seul le joueur est blessé (pas de tir ami entre figurants).
	if (Cast<AReshootEnemy>(GetInstigator()))
	{
		return Target->IsA<AReshootCharacter>();
	}
	return true;
}

bool AMedievalArrow::IsHeadHit(const AActor* Target, const FHitResult& Hit) const
{
	if (Hit.BoneName == TEXT("head"))
	{
		return true;
	}
	if (const UPrimitiveComponent* Comp = Hit.GetComponent())
	{
		if (Comp->ComponentHasTag(TEXT("head")))
		{
			return true;
		}
	}
	// Figurants en boîte grise : le haut de la capsule compte comme tête.
	if (const ACharacter* Character = Cast<ACharacter>(Target))
	{
		const UCapsuleComponent* Capsule = Character->GetCapsuleComponent();
		const FVector Point = Hit.ImpactPoint.IsZero() ? GetActorLocation() : FVector(Hit.ImpactPoint);
		const float HeadZ = Capsule->GetComponentLocation().Z + Capsule->GetScaledCapsuleHalfHeight() * HeadZoneFraction;
		return Point.Z >= HeadZ;
	}
	return false;
}

void AMedievalArrow::HandleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (bDone || !CanDamage(OtherActor))
	{
		return;
	}
	bDone = true;

	FHitResult Hit = SweepResult;
	if (!bFromSweep)
	{
		Hit.ImpactPoint = GetActorLocation();
	}
	const bool bHeadshot = IsHeadHit(OtherActor, Hit);

	if (UHealthComponent* Health = OtherActor->FindComponentByClass<UHealthComponent>())
	{
		const float Applied = Health->ApplyDamage(Damage, GetInstigator() ? static_cast<AActor*>(GetInstigator()) : GetOwner(), bHeadshot, false);

		// Flèche du joueur qui touche : prolonge son combo médiéval.
		if (Applied > 0.f)
		{
			if (const AReshootCharacter* Shooter = Cast<AReshootCharacter>(GetInstigator()))
			{
				if (AMedievalSword* Sword = Cast<AMedievalSword>(Shooter->GetCurrentWeapon()))
				{
					Sword->RegisterHit(Health->IsDead(), bHeadshot, false);
				}
			}
		}
	}
	if (ImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, GetActorLocation());
	}
	Destroy();
}

void AMedievalArrow::HandleHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (bDone)
	{
		return;
	}
	bDone = true;

	if (ImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, Hit.ImpactPoint);
	}
	StopAndStick(OtherComp);
}

void AMedievalArrow::StopAndStick(USceneComponent* AttachTo)
{
	Movement->StopMovementImmediately();
	Movement->Deactivate();
	Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (AttachTo)
	{
		AttachToComponent(AttachTo, FAttachmentTransformRules::KeepWorldTransform);
	}
	SetLifeSpan(FMath::Max(StuckLifeSpan, 0.01f));
}

void AMedievalArrow::HandleCut()
{
	CustomTimeDilation = 0.f;
}

void AMedievalArrow::HandleAction()
{
	CustomTimeDilation = 1.f;
}
