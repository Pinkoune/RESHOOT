#include "Space/SpaceGravityEvent.h"
#include "Space/SpaceDrone.h"
#include "Core/ReshootCharacter.h"
#include "RESHOOT.h"

#include "Components/LightComponent.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

ASpaceGravityEvent::ASpaceGravityEvent()
{
	Duration = 10.f;
}

void ASpaceGravityEvent::StartEvent()
{
	if (bEventActive)
	{
		return;
	}
	Super::StartEvent();
	UE_LOG(LogReshoot, Log, TEXT("Gravity event: gravity ON for %.1fs"), Duration);

	ApplyGravity(true);
	if (GravityOnSound)
	{
		UGameplayStatics::PlaySound2D(this, GravityOnSound);
	}
	if (!AlarmLightTag.IsNone())
	{
		bAlarmLit = false;
		BlinkAlarm();
		GetWorldTimerManager().SetTimer(AlarmTimer, this, &ASpaceGravityEvent::BlinkAlarm, AlarmBlinkInterval, true);
	}
}

void ASpaceGravityEvent::StopEvent()
{
	if (!bEventActive)
	{
		Super::StopEvent();
		return;
	}
	UE_LOG(LogReshoot, Log, TEXT("Gravity event: gravity OFF"));

	ApplyGravity(false);

	GetWorldTimerManager().ClearTimer(AlarmTimer);
	bAlarmLit = true; // BlinkAlarm inverse : on éteint.
	BlinkAlarm();

	if (GravityOffSound)
	{
		UGameplayStatics::PlaySound2D(this, GravityOffSound);
	}
	Super::StopEvent();
}

void ASpaceGravityEvent::SetEventPaused(bool bPaused)
{
	Super::SetEventPaused(bPaused);
	if (bPaused)
	{
		GetWorldTimerManager().PauseTimer(AlarmTimer);
	}
	else
	{
		GetWorldTimerManager().UnPauseTimer(AlarmTimer);
	}
}

void ASpaceGravityEvent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (bEventActive && EndPlayReason == EEndPlayReason::Destroyed)
	{
		StopEvent();
	}
	GetWorldTimerManager().ClearTimer(AlarmTimer);
	Super::EndPlay(EndPlayReason);
}

void ASpaceGravityEvent::ApplyGravity(bool bGravityOn)
{
	if (AReshootCharacter* Player = Cast<AReshootCharacter>(UGameplayStatics::GetPlayerPawn(this, 0)))
	{
		Player->SetSpaceGravity(bGravityOn);
	}
	if (!bAffectDrones)
	{
		return;
	}
	for (TActorIterator<ASpaceDrone> It(GetWorld()); It; ++It)
	{
		if (!It->IsDead())
		{
			It->SetGravityEnabled(bGravityOn);
		}
	}
}

void ASpaceGravityEvent::BlinkAlarm()
{
	bAlarmLit = !bAlarmLit;
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		if (!It->ActorHasTag(AlarmLightTag))
		{
			continue;
		}
		TArray<ULightComponent*> Lights;
		It->GetComponents<ULightComponent>(Lights);
		for (ULightComponent* Light : Lights)
		{
			Light->SetVisibility(bAlarmLit);
		}
	}
}
