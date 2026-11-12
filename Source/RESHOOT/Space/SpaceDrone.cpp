#include "Space/SpaceDrone.h"

ASpaceDrone::ASpaceDrone(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Universe = EUniverse::Space;
}
