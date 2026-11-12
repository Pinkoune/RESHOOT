#include "Director/ShootScript.h"
#include "Director/Directive.h"
#include "Core/UniverseDefinition.h"
#include "Core/ReshootEnemy.h"

UUniverseDefinition* UShootScript::FindUniverseDefinition(EUniverse Universe) const
{
	if (Universe == EUniverse::None)
	{
		return nullptr;
	}
	for (const FSceneDef& Scene : Scenes)
	{
		if (Scene.UniverseDefinition && Scene.UniverseDefinition->Universe == Universe)
		{
			return Scene.UniverseDefinition;
		}
	}
	for (UUniverseDefinition* Definition : FinaleUniverses)
	{
		if (Definition && Definition->Universe == Universe)
		{
			return Definition;
		}
	}
	return nullptr;
}
