#include "Director/Directive.h"
#include "Director/ShootDirectorSubsystem.h"

void UDirective::OnBegin(UShootDirectorSubsystem* Director)
{
}

void UDirective::OnEnd(UShootDirectorSubsystem* Director)
{
}

bool UDirective::IsKillRespected(const FKillInfo& Kill) const
{
	return true;
}

void UDirective::OnKill(UShootDirectorSubsystem* Director, const FKillInfo& Kill)
{
}

bool UDirective::IsAllowedIn(EUniverse Universe) const
{
	return AllowedUniverses.Num() == 0 || AllowedUniverses.Contains(Universe);
}
