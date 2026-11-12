#include "Director/DirectiveKeepMoving.h"

bool UDirectiveKeepMoving::IsKillRespected(const FKillInfo& Kill) const
{
	return Kill.bPlayerMoving;
}
