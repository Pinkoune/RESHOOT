#include "Core/UniverseEvent.h"

AUniverseEvent::AUniverseEvent()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AUniverseEvent::StartEvent()
{
	bEventActive = true;
}

void AUniverseEvent::StopEvent()
{
	bEventActive = false;
}
