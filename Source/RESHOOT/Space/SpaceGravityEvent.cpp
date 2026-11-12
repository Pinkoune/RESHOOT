#include "Space/SpaceGravityEvent.h"

// Lot d'Argon : comportement à écrire dans ce fichier (voir docs/briefs/argon-space-gravity.md).

ASpaceGravityEvent::ASpaceGravityEvent()
{
	Duration = 10.f;
}

void ASpaceGravityEvent::StartEvent()
{
	Super::StartEvent();
}

void ASpaceGravityEvent::StopEvent()
{
	Super::StopEvent();
}

void ASpaceGravityEvent::SetEventPaused(bool bPaused)
{
	Super::SetEventPaused(bPaused);
}

void ASpaceGravityEvent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void ASpaceGravityEvent::ApplyGravity(bool bGravityOn)
{
}

void ASpaceGravityEvent::BlinkAlarm()
{
}
