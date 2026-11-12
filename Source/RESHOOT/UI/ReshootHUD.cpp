#include "UI/ReshootHUD.h"
#include "Core/HealthComponent.h"
#include "Core/ReshootCharacter.h"
#include "Core/ReshootEnemy.h"
#include "Core/ReshootGameState.h"
#include "Core/ReshootWeapon.h"
#include "Director/AudienceRating.h"
#include "Director/Directive.h"
#include "Director/DirectorVoice.h"
#include "Director/ShootDirectorSubsystem.h"
#include "Medieval/MedievalSword.h"
#include "RESHOOT.h"

#include "CanvasItem.h"
#include "Engine/Canvas.h"
#include "Engine/Font.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

AReshootHUD::AReshootHUD()
{
	static ConstructorHelpers::FObjectFinder<UFont> RobotoFont(TEXT("/Engine/EngineFonts/Roboto.Roboto"));
	if (RobotoFont.Succeeded())
	{
		Font = RobotoFont.Object;
	}
}

void AReshootHUD::BeginPlay()
{
	Super::BeginPlay();
	TryBind();

	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<AReshootEnemy> It(World); It; ++It)
		{
			BindEnemy(*It);
		}
		ActorSpawnedHandle = World->AddOnActorSpawnedHandler(FOnActorSpawned::FDelegate::CreateUObject(this, &AReshootHUD::HandleActorSpawned));
	}
}

void AReshootHUD::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->RemoveOnActorSpawnedHandler(ActorSpawnedHandle);
		World->GetTimerManager().ClearAllTimersForObject(this);
	}
	Super::EndPlay(EndPlayReason);
}

double AReshootHUD::Now() const
{
	return GetWorld() ? GetWorld()->GetRealTimeSeconds() : 0.0;
}

// ---------------------------------------------------------------------------
// Écoute des delegates

void AReshootHUD::TryBind()
{
	if (!bBoundGlobal)
	{
		if (const AReshootGameState* GameState = GetWorld() ? GetWorld()->GetGameState<AReshootGameState>() : nullptr)
		{
			if (UAudienceRating* Rating = GameState->GetAudienceRating())
			{
				Rating->OnRatingChanged.AddUniqueDynamic(this, &AReshootHUD::HandleRatingChanged);
			}
			if (UDirectorVoice* Voice = GameState->GetDirectorVoice())
			{
				Voice->OnSubtitle.AddUniqueDynamic(this, &AReshootHUD::HandleSubtitle);
			}
			bBoundGlobal = true;
		}
	}

	// Le pion du joueur change à chaque reprise : on se rebranche sur sa vie si besoin.
	const AReshootCharacter* Player = Cast<AReshootCharacter>(GetOwningPawn());
	UHealthComponent* Health = Player ? Player->GetHealth() : nullptr;
	if (Health != BoundPlayerHealth)
	{
		if (BoundPlayerHealth)
		{
			BoundPlayerHealth->OnHealthChanged.RemoveDynamic(this, &AReshootHUD::HandlePlayerHealthChanged);
		}
		BoundPlayerHealth = Health;
		if (BoundPlayerHealth)
		{
			BoundPlayerHealth->OnHealthChanged.AddUniqueDynamic(this, &AReshootHUD::HandlePlayerHealthChanged);
		}
	}
}

void AReshootHUD::HandleActorSpawned(AActor* Actor)
{
	BindEnemy(Actor);
}

void AReshootHUD::BindEnemy(AActor* Actor)
{
	if (const AReshootEnemy* Enemy = Cast<AReshootEnemy>(Actor))
	{
		if (UHealthComponent* Health = Enemy->GetHealth())
		{
			Health->OnHealthChanged.AddUniqueDynamic(this, &AReshootHUD::HandleEnemyHealthChanged);
		}
	}
}

void AReshootHUD::HandleRatingChanged(float NewRating, float Delta)
{
	if (Delta > 0.f)
	{
		RatingPulseTime = Now();
		FloatingGains.Add({ FString::Printf(TEXT("+ %d"), FMath::RoundToInt(Delta)), Now() });
	}
}

void AReshootHUD::HandleSubtitle(FName LineId, const FText& Subtitle, float Duration)
{
	SubtitleText = Subtitle;
	SubtitleEndTime = Now() + FMath::Max(Duration, 1.f);
}

void AReshootHUD::HandlePlayerHealthChanged(UHealthComponent* HealthComponent, float NewHealth, float Delta)
{
	if (Delta < 0.f)
	{
		DamageFlashTime = Now();
	}
}

void AReshootHUD::HandleEnemyHealthChanged(UHealthComponent* HealthComponent, float NewHealth, float Delta)
{
	if (Delta < 0.f)
	{
		HitMarkerTime = Now();
	}
}

// ---------------------------------------------------------------------------
// Outils de dessin

float AReshootHUD::UIScale() const
{
	return Canvas ? Canvas->ClipY / 1080.f : 1.f;
}

namespace
{
	// La police Roboto du moteur est petite à l'échelle 1 : on l'agrandit pour une lecture à 1080p.
	constexpr float FontBoost = 1.7f;
}

FVector2D AReshootHUD::MeasureText(const FString& Text, float Scale) const
{
	float W = 0.f, H = 0.f;
	if (Canvas && Font)
	{
		const float S = Scale * UIScale() * TextScale * FontBoost;
		Canvas->TextSize(Font, Text, W, H, S, S);
	}
	return FVector2D(W, H);
}

void AReshootHUD::DrawTextAt(const FString& Text, float X, float Y, float Scale, const FLinearColor& Color, bool bCenterX, bool bShadow)
{
	if (!Canvas || !Font || Text.IsEmpty())
	{
		return;
	}
	FCanvasTextItem Item(FVector2D(X, Y), FText::FromString(Text), Font, Color);
	const float S = Scale * UIScale() * TextScale * FontBoost;
	Item.Scale = FVector2D(S, S);
	Item.bCentreX = bCenterX;
	if (bShadow)
	{
		Item.EnableShadow(FLinearColor(0.f, 0.f, 0.f, Color.A * 0.8f), FVector2D(2.f, 2.f));
	}
	Canvas->DrawItem(Item);
}

void AReshootHUD::DrawBar(float X, float Y, float W, float H, float Pct, const FLinearColor& Fill, const FLinearColor& Back)
{
	DrawRect(Back, X, Y, W, H);
	DrawRect(Fill, X, Y, W * FMath::Clamp(Pct, 0.f, 1.f), H);
}

// ---------------------------------------------------------------------------

void AReshootHUD::DrawHUD()
{
	Super::DrawHUD();
	if (!Canvas)
	{
		return;
	}
	TryBind();

	// Le HUD remplace l'affichage provisoire du combo médiéval.
	if (const AReshootCharacter* Player = Cast<AReshootCharacter>(GetOwningPawn()))
	{
		if (AMedievalSword* Sword = Cast<AMedievalSword>(Player->GetCurrentWeapon()))
		{
			Sword->bShowComboOnScreen = false;
		}
	}

	DrawCutBars();
	DrawDamageFlash();
	DrawTake();
	DrawDirective();
	DrawAudience();
	DrawHealth();
	DrawWeapon();
	DrawSubtitle();
	DrawReticle();
}

void AReshootHUD::DrawCutBars()
{
	const UShootDirectorSubsystem* Director = GetWorld()->GetSubsystem<UShootDirectorSubsystem>();
	if (!Director || Director->GetPhase() != EShootPhase::Cut)
	{
		return;
	}
	// « CUT! » : bandes noires et gros titre, comme un arrêt sur image.
	const float BarH = Canvas->ClipY * 0.11f;
	DrawRect(FLinearColor::Black, 0.f, 0.f, Canvas->ClipX, BarH);
	DrawRect(FLinearColor::Black, 0.f, Canvas->ClipY - BarH, Canvas->ClipX, BarH);
	DrawTextAt(TEXT("CUT !"), Canvas->ClipX * 0.5f, BarH + 20.f * UIScale(), 3.2f, FLinearColor::White, true);
}

void AReshootHUD::DrawDamageFlash()
{
	const double Age = Now() - DamageFlashTime;
	if (Age > DamageFlashDuration || DamageFlashDuration <= 0.f)
	{
		return;
	}
	const float A = 0.45f * (1.f - static_cast<float>(Age / DamageFlashDuration));
	const FLinearColor Red(0.85f, 0.f, 0.f, A);
	const float T = Canvas->ClipY * 0.06f;
	DrawRect(Red, 0.f, 0.f, Canvas->ClipX, T);
	DrawRect(Red, 0.f, Canvas->ClipY - T, Canvas->ClipX, T);
	DrawRect(Red, 0.f, T, T, Canvas->ClipY - 2.f * T);
	DrawRect(Red, Canvas->ClipX - T, T, T, Canvas->ClipY - 2.f * T);
}

void AReshootHUD::DrawTake()
{
	const float S = UIScale();
	const float X = 40.f * S, Y = 32.f * S;
	const UShootDirectorSubsystem* Director = GetWorld()->GetSubsystem<UShootDirectorSubsystem>();
	const int32 Scene = Director ? FMath::Max(Director->GetSceneIndex(), 0) + 1 : 1;
	const int32 Take = Director ? FMath::Max(Director->GetTakeIndex(), 0) + 1 : 1;
	const EShootPhase Phase = Director ? Director->GetPhase() : EShootPhase::TakeRunning;

	// Voyant REC : clignote pendant que la caméra tourne, fixe et gris sinon.
	const bool bRolling = (Phase == EShootPhase::TakeRunning || Phase == EShootPhase::Finale || Phase == EShootPhase::Menu);
	const bool bBlinkOn = FMath::Fmod(Now(), 1.0) < 0.6;
	const float Dot = 18.f * S;
	DrawRect(bRolling ? (bBlinkOn ? RecColor : RecColor * 0.35f) : FLinearColor(0.35f, 0.35f, 0.35f, 1.f), X, Y + 6.f * S, Dot, Dot);
	DrawTextAt(TEXT("REC"), X + Dot + 10.f * S, Y, 1.f, bRolling ? FLinearColor::White : FLinearColor(0.6f, 0.6f, 0.6f, 1.f));
	DrawTextAt(FString::Printf(TEXT("SCÈNE %d   -   PRISE %d"), Scene, Take), X, Y + 36.f * S, 1.1f, AccentColor);
}

void AReshootHUD::DrawDirective()
{
	const UShootDirectorSubsystem* Director = GetWorld()->GetSubsystem<UShootDirectorSubsystem>();
	const UDirective* Directive = Director ? Director->GetCurrentDirective() : nullptr;
	if (!Directive || Directive->Title.IsEmpty())
	{
		return;
	}
	const float S = UIScale();
	const FString Text = Directive->Title.ToString();
	const FVector2D Size = MeasureText(Text, 1.4f);
	const float Cx = Canvas->ClipX * 0.5f, Y = 30.f * S;
	DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.55f), Cx - Size.X * 0.5f - 18.f * S, Y - 6.f * S, Size.X + 36.f * S, Size.Y + 12.f * S);
	DrawTextAt(Text, Cx, Y, 1.4f, AccentColor, true);
}

void AReshootHUD::DrawAudience()
{
	const float S = UIScale();
	const float W = 380.f * S, H = 20.f * S;
	const float X = Canvas->ClipX - W - 40.f * S, Y = 40.f * S;

	const AReshootGameState* GameState = GetWorld()->GetGameState<AReshootGameState>();
	const UAudienceRating* Rating = GameState ? GameState->GetAudienceRating() : nullptr;
	const float Pct = Rating ? Rating->GetRatingPct() : 0.f;
	const float Value = Rating ? Rating->GetRating() : 0.f;
	const float MaxValue = Rating ? Rating->MaxRating : 1000.f;

	// Pulse à chaque gain.
	const float PulseAge = static_cast<float>(Now() - RatingPulseTime);
	const float Pulse = PulseAge < 0.35f ? 1.f - PulseAge / 0.35f : 0.f;
	const float PH = H * (1.f + 0.6f * Pulse);

	DrawTextAt(TEXT("AUDIMAT"), X, Y - 30.f * S, 0.9f, FLinearColor::White);
	const FString Count = FString::Printf(TEXT("%d / %d"), FMath::RoundToInt(Value), FMath::RoundToInt(MaxValue));
	DrawTextAt(Count, X + W - MeasureText(Count, 0.9f).X, Y - 30.f * S, 0.9f, FLinearColor(0.8f, 0.8f, 0.8f, 1.f));
	DrawBar(X, Y - (PH - H) * 0.5f, W, PH, Pct, FMath::Lerp(AccentColor, FLinearColor::White, Pulse), FLinearColor(0.f, 0.f, 0.f, 0.6f));

	// Paliers d'étoiles.
	if (Rating)
	{
		const int32 Stars = Rating->GetStars();
		for (int32 i = 0; i < Rating->StarThresholds.Num(); ++i)
		{
			const float TX = X + W * FMath::Clamp(Rating->StarThresholds[i] / MaxValue, 0.f, 1.f);
			DrawRect(FLinearColor(1.f, 1.f, 1.f, 0.7f), TX - 1.f, Y - 4.f * S, 2.f, H + 8.f * S);
			const float Box = 14.f * S;
			DrawRect(i < Stars ? AccentColor : FLinearColor(0.25f, 0.25f, 0.25f, 0.9f), X + i * (Box + 6.f * S), Y + H + 10.f * S, Box, Box);
		}
	}

	// « + 25 » qui s'envolent vers la jauge.
	const double T = Now();
	FloatingGains.RemoveAll([T](const FFloatingGain& G) { return T - G.StartTime > 1.2; });
	for (const FFloatingGain& Gain : FloatingGains)
	{
		const float Age = static_cast<float>(T - Gain.StartTime) / 1.2f;
		DrawTextAt(Gain.Text, X + W * Pct, Y + 60.f * S - 50.f * S * Age, 1.2f, FLinearColor(AccentColor.R, AccentColor.G, AccentColor.B, 1.f - Age), true);
	}

	// Combo médiéval sous l'audimat.
	const AReshootCharacter* Player = Cast<AReshootCharacter>(GetOwningPawn());
	const AMedievalSword* Sword = Player ? Cast<AMedievalSword>(Player->GetCurrentWeapon()) : nullptr;
	if (Sword)
	{
		const float CY = Y + H + 40.f * S;
		DrawTextAt(FString::Printf(TEXT("SCORE  %d"), Sword->GetComboScore()), X, CY, 1.f, FLinearColor::White);
		if (Sword->GetCombo() > 0)
		{
			const int32 Mult = Sword->GetComboMultiplier();
			const FLinearColor C = Mult >= 4 ? FLinearColor(1.f, 0.3f, 0.15f, 1.f) : (Mult >= 2 ? AccentColor : FLinearColor::White);
			DrawTextAt(FString::Printf(TEXT("COMBO %d   x%d"), Sword->GetCombo(), Mult), X, CY + 30.f * S, 1.3f, C);
			DrawBar(X, CY + 70.f * S, W * 0.6f, 6.f * S, Sword->GetComboTimeLeftPct(), C, FLinearColor(0.f, 0.f, 0.f, 0.5f));
		}
	}
}

void AReshootHUD::DrawHealth()
{
	const AReshootCharacter* Player = Cast<AReshootCharacter>(GetOwningPawn());
	const UHealthComponent* Health = Player ? Player->GetHealth() : nullptr;
	if (!Health)
	{
		return;
	}
	const float S = UIScale();
	const float W = 360.f * S, H = 22.f * S;
	const float X = 40.f * S, Y = Canvas->ClipY - 60.f * S;
	const float Pct = Health->GetHealthPct();
	const FLinearColor Fill = Pct > 0.5f ? FLinearColor(0.9f, 0.9f, 0.9f, 1.f) : (Pct > 0.25f ? AccentColor : RecColor);
	DrawTextAt(FString::Printf(TEXT("VIE  %d"), FMath::RoundToInt(Health->GetHealth())), X, Y - 34.f * S, 1.f, FLinearColor::White);
	DrawBar(X, Y, W, H, Pct, Fill, FLinearColor(0.f, 0.f, 0.f, 0.6f));
}

void AReshootHUD::DrawWeapon()
{
	const AReshootCharacter* Player = Cast<AReshootCharacter>(GetOwningPawn());
	const AReshootWeapon* Weapon = Player ? Player->GetCurrentWeapon() : nullptr;
	if (!Weapon)
	{
		return;
	}
	const float S = UIScale();
	const float Right = Canvas->ClipX - 40.f * S;
	const float Y = Canvas->ClipY - 100.f * S;

	const FString Name = Weapon->DisplayName.ToString();
	const FVector2D NameSize = MeasureText(Name, 1.4f);
	DrawTextAt(Name, Right - NameSize.X, Y, 1.4f, FLinearColor::White);

	if (Weapon->GetMaxAmmo() >= 0)
	{
		const FString Ammo = FString::Printf(TEXT("%d / %d"), Weapon->GetAmmo(), Weapon->GetMaxAmmo());
		const FVector2D AmmoSize = MeasureText(Ammo, 1.2f);
		DrawTextAt(Ammo, Right - AmmoSize.X, Y + 44.f * S, 1.2f, Weapon->GetAmmo() > 0 ? AccentColor : RecColor);
	}

	if (const AMedievalSword* Sword = Cast<AMedievalSword>(Weapon))
	{
		// Rappel des touches, mode actif en surbrillance.
		const EMedievalWeaponMode Mode = Sword->GetMode();
		const TCHAR* Labels[] = { TEXT("1 ÉPÉE"), TEXT("2 ARC"), TEXT("3 POINGS") };
		float X = Right;
		for (int32 i = 2; i >= 0; --i)
		{
			const FVector2D Sz = MeasureText(Labels[i], 0.8f);
			X -= Sz.X;
			const bool bActive = static_cast<int32>(Mode) == i;
			DrawTextAt(Labels[i], X, Y + 44.f * S, 0.8f, bActive ? AccentColor : FLinearColor(0.55f, 0.55f, 0.55f, 1.f));
			X -= 22.f * S;
		}
		if (Mode == EMedievalWeaponMode::Bow && Sword->GetBowCharge() > 0.f)
		{
			DrawBar(Right - 220.f * S, Y - 22.f * S, 220.f * S, 8.f * S, Sword->GetBowCharge(), AccentColor, FLinearColor(0.f, 0.f, 0.f, 0.6f));
		}
		if (Sword->IsShieldRaised())
		{
			const FString Guard = Mode == EMedievalWeaponMode::Fists ? TEXT("GARDE") : TEXT("BOUCLIER LEVÉ");
			const FVector2D Sz = MeasureText(Guard, 0.8f);
			DrawTextAt(Guard, Right - Sz.X, Y - 30.f * S, 0.8f, FLinearColor(0.6f, 0.8f, 1.f, 1.f));
		}
	}
}

void AReshootHUD::DrawSubtitle()
{
	if (Now() > SubtitleEndTime || SubtitleText.IsEmpty())
	{
		return;
	}
	const float S = UIScale();
	const FString Text = SubtitleText.ToString();
	const FVector2D Size = MeasureText(Text, 1.25f);
	const float Cx = Canvas->ClipX * 0.5f;
	const float Y = Canvas->ClipY - 170.f * S;
	DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.6f), Cx - Size.X * 0.5f - 16.f * S, Y - 8.f * S, Size.X + 32.f * S, Size.Y + 16.f * S);
	DrawTextAt(Text, Cx, Y, 1.25f, FLinearColor(1.f, 0.95f, 0.8f, 1.f), true);
}

void AReshootHUD::DrawReticle()
{
	const float S = UIScale();
	const float Cx = Canvas->ClipX * 0.5f, Cy = Canvas->ClipY * 0.5f;
	const FLinearColor C(1.f, 1.f, 1.f, 0.9f);

	const AReshootCharacter* Player = Cast<AReshootCharacter>(GetOwningPawn());
	const AMedievalSword* Sword = Player ? Cast<AMedievalSword>(Player->GetCurrentWeapon()) : nullptr;

	if (Sword && Sword->GetMode() == EMedievalWeaponMode::Bow)
	{
		// Arc : croix dont l'écart se resserre avec la charge.
		const float Gap = FMath::Lerp(18.f, 4.f, Sword->GetBowCharge()) * S;
		const float L = 10.f * S;
		DrawLine(Cx - Gap - L, Cy, Cx - Gap, Cy, C, 2.f);
		DrawLine(Cx + Gap, Cy, Cx + Gap + L, Cy, C, 2.f);
		DrawLine(Cx, Cy - Gap - L, Cx, Cy - Gap, C, 2.f);
		DrawLine(Cx, Cy + Gap, Cx, Cy + Gap + L, C, 2.f);
	}
	else if (Sword)
	{
		// Corps à corps : crochets qui montrent la largeur du coup.
		const float R = (Sword->GetMode() == EMedievalWeaponMode::Fists ? 12.f : 20.f) * S;
		const float L = 7.f * S;
		DrawLine(Cx - R, Cy - L, Cx - R, Cy + L, C, 2.f);
		DrawLine(Cx + R, Cy - L, Cx + R, Cy + L, C, 2.f);
		DrawRect(C, Cx - 1.5f * S, Cy - 1.5f * S, 3.f * S, 3.f * S);
	}
	else
	{
		// Armes à feu : un point.
		DrawRect(C, Cx - 2.5f * S, Cy - 2.5f * S, 5.f * S, 5.f * S);
	}

	// Marqueur de touche.
	if (Now() - HitMarkerTime < HitMarkerDuration)
	{
		const float A = 8.f * S, B = 16.f * S;
		const FLinearColor H(1.f, 0.35f, 0.25f, 1.f);
		DrawLine(Cx - B, Cy - B, Cx - A, Cy - A, H, 2.5f);
		DrawLine(Cx + B, Cy - B, Cx + A, Cy - A, H, 2.5f);
		DrawLine(Cx - B, Cy + B, Cx - A, Cy + A, H, 2.5f);
		DrawLine(Cx + B, Cy + B, Cx + A, Cy + A, H, 2.5f);
	}
}
