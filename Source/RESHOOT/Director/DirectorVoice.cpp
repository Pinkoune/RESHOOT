#include "Director/DirectorVoice.h"
#include "RESHOOT.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

UDirectorVoice::UDirectorVoice()
{
	PrimaryComponentTick.bCanEverTick = false;
}

float UDirectorVoice::PlayLine(FName LineId)
{
	if (LineId.IsNone() || !LinesTable)
	{
		return 0.f;
	}

	const FDirectorLine* Line = LinesTable->FindRow<FDirectorLine>(LineId, TEXT("UDirectorVoice::PlayLine"), false);
	if (!Line)
	{
		UE_LOG(LogReshoot, Warning, TEXT("Director line '%s' not found in %s"), *LineId.ToString(), *GetNameSafe(LinesTable));
		return 0.f;
	}

	float Duration = Line->SubtitleDuration;
	if (Line->Sound)
	{
		UGameplayStatics::PlaySound2D(this, Line->Sound);
		Duration = FMath::Max(Duration, Line->Sound->GetDuration());
	}
	OnSubtitle.Broadcast(LineId, Line->Subtitle, Duration);
	return Duration;
}
