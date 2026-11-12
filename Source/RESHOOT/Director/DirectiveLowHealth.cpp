#include "Director/DirectiveLowHealth.h"

float UDirectiveLowHealth::EvaluateKill(const FKillInfo& Kill) const
{
	return Judge(Kill.PlayerHealthPct <= MaxHealthPct);
}
