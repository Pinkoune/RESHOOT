#include "Director/DirectiveFast.h"
#include "Director/ShootDirectorSubsystem.h"

#include "Engine/World.h"

void UDirectiveFast::OnBegin(UShootDirectorSubsystem* Director)
{
	Super::OnBegin(Director);
	LastKillTime = Now();
	if (Director)
	{
		// Le chrono repart à chaque « Action! » : le gel du CUT! ne compte pas.
		Director->OnAction.AddUniqueDynamic(this, &UDirectiveFast::HandleAction);
	}
}

void UDirectiveFast::OnEnd(UShootDirectorSubsystem* Director)
{
	if (Director)
	{
		Director->OnAction.RemoveDynamic(this, &UDirectiveFast::HandleAction);
	}
	Super::OnEnd(Director);
}

void UDirectiveFast::HandleAction()
{
	LastKillTime = Now();
}

double UDirectiveFast::Now() const
{
	const UShootDirectorSubsystem* Director = GetDirector();
	const UWorld* World = Director ? Director->GetWorld() : nullptr;
	return World ? World->GetTimeSeconds() : 0.0;
}

float UDirectiveFast::EvaluateKill(const FKillInfo& Kill) const
{
	const double Time = Now();
	const bool bFastEnough = Time - LastKillTime <= MaxSecondsBetweenKills;
	LastKillTime = Time;
	return Judge(bFastEnough);
}
