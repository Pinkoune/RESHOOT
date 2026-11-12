#include "Core/UniverseEvent.h"
#include "TimerManager.h"
#include "Engine/World.h"

AUniverseEvent::AUniverseEvent()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AUniverseEvent::StartEvent()
{
	bEventActive = true;
	if (Duration > 0.f)
	{
		GetWorldTimerManager().SetTimer(DurationTimer, this, &AUniverseEvent::StopEvent, Duration, false);
	}
	OnEventStateChanged.Broadcast(this, true);
}

void AUniverseEvent::StopEvent()
{
	GetWorldTimerManager().ClearTimer(DurationTimer);
	if (!bEventActive)
	{
		return;
	}
	bEventActive = false;
	OnEventStateChanged.Broadcast(this, false);
}

void AUniverseEvent::SetEventPaused(bool bPaused)
{
	if (bPaused)
	{
		GetWorldTimerManager().PauseTimer(DurationTimer);
	}
	else
	{
		GetWorldTimerManager().UnPauseTimer(DurationTimer);
	}
}
