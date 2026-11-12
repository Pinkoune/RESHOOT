#include "Director/Directive.h"
#include "Director/ShootDirectorSubsystem.h"
#include "Core/ReshootCharacter.h"

#include "Kismet/GameplayStatics.h"

void UDirective::OnBegin(UShootDirectorSubsystem* Director)
{
	DirectorPtr = Director;
	PlayerPtr = Director ? Cast<AReshootCharacter>(UGameplayStatics::GetPlayerPawn(Director, 0)) : nullptr;
}

void UDirective::OnEnd(UShootDirectorSubsystem* Director)
{
	PlayerPtr.Reset();
	DirectorPtr.Reset();
}

float UDirective::EvaluateKill(const FKillInfo& Kill) const
{
	return 1.f;
}
