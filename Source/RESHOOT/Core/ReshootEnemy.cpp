#include "Core/ReshootEnemy.h"
#include "Core/HealthComponent.h"
#include "Core/ReshootCharacter.h"
#include "Director/ShootDirectorSubsystem.h"
#include "RESHOOT.h"

#include "AIController.h"
#include "BrainComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"

AReshootEnemy::AReshootEnemy(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = false;
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));
}

void AReshootEnemy::BeginPlay()
{
	Super::BeginPlay();
	HealthComponent->OnDeath.AddDynamic(this, &AReshootEnemy::HandleDeath);
}

AReshootCharacter* AReshootEnemy::GetTargetPlayer() const
{
	return Cast<AReshootCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
}

bool AReshootEnemy::IsDead() const
{
	return HealthComponent && HealthComponent->IsDead();
}

void AReshootEnemy::Freeze(bool bFreeze)
{
	if (bFrozen == bFreeze)
	{
		return;
	}
	bFrozen = bFreeze;

	// Gèle tick, mouvement et animations de l'acteur et de ses composants.
	CustomTimeDilation = bFreeze ? 0.f : 1.f;

	if (bFreeze)
	{
		GetCharacterMovement()->StopMovementImmediately();
	}

	if (AAIController* AI = Cast<AAIController>(GetController()))
	{
		if (bFreeze)
		{
			AI->StopMovement();
		}
		if (UBrainComponent* Brain = AI->GetBrainComponent())
		{
			if (bFreeze)
			{
				Brain->PauseLogic(TEXT("Cut"));
			}
			else
			{
				Brain->ResumeLogic(TEXT("Cut"));
			}
		}
	}

	OnFreezeChanged(bFreeze);
}

void AReshootEnemy::OnFreezeChanged(bool bFreeze)
{
}

FKillInfo AReshootEnemy::BuildKillInfo(AActor* Killer, bool bHeadshot, bool bMelee) const
{
	FKillInfo Info;
	Info.Universe = Universe;
	Info.bHeadshot = bHeadshot;
	Info.bMelee = bMelee;
	if (const AReshootCharacter* Player = GetTargetPlayer())
	{
		Info.bPlayerMoving = Player->IsMoving();
		if (const UHealthComponent* PlayerHealth = Player->GetHealth())
		{
			Info.PlayerHealthPct = PlayerHealth->GetHealthPct();
		}
	}
	return Info;
}

void AReshootEnemy::HandleDeath(UHealthComponent* DeadComponent, AActor* Killer, bool bHeadshot, bool bMelee)
{
	if (UShootDirectorSubsystem* Director = GetWorld()->GetSubsystem<UShootDirectorSubsystem>())
	{
		Director->NotifyKill(BuildKillInfo(Killer, bHeadshot, bMelee));
	}
	OnDeath(Killer, bHeadshot, bMelee);
	OnEnemyDied.Broadcast(this);
}

void AReshootEnemy::OnDeath(AActor* Killer, bool bHeadshot, bool bMelee)
{
	if (AController* AI = GetController())
	{
		AI->UnPossess();
	}
	GetCharacterMovement()->DisableMovement();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetLifeSpan(FMath::Max(CorpseLifeSpan, 0.01f));
}
