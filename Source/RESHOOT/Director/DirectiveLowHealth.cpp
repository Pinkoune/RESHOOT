#include "Director/DirectiveLowHealth.h"

bool UDirectiveLowHealth::IsKillRespected(const FKillInfo& Kill) const
{
	return Kill.PlayerHealthPct <= MaxHealthPct;
}
