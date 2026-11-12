#include "Core/ReshootGameState.h"
#include "Director/AudienceRating.h"
#include "Director/DirectorVoice.h"

AReshootGameState::AReshootGameState()
{
	AudienceRating = CreateDefaultSubobject<UAudienceRating>(TEXT("AudienceRating"));
	DirectorVoice = CreateDefaultSubobject<UDirectorVoice>(TEXT("DirectorVoice"));
}
