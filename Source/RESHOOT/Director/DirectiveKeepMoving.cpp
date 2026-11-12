#include "Director/DirectiveKeepMoving.h"

float UDirectiveKeepMoving::EvaluateKill(const FKillInfo& Kill) const
{
	return Judge(Kill.bPlayerMoving);
}
