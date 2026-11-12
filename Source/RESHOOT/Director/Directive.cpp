#include "Director/Directive.h"
#include "Director/ShootDirectorSubsystem.h"

void UDirective::OnBegin(UShootDirectorSubsystem* Director)
{
}

void UDirective::OnEnd(UShootDirectorSubsystem* Director)
{
}

float UDirective::EvaluateKill(const FKillInfo& Kill) const
{
	return 1.f;
}
