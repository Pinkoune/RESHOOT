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
#include "Director/ShootScript.h"
#include "Core/UniverseDefinition.h"
#include "Medieval/MedievalSword.h"
#include "RESHOOT.h"

#include "CanvasItem.h"
#include "Engine/Canvas.h"
#include "Engine/Font.h"
#include "Fonts/SlateFontInfo.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/SlateRenderer.h"
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
	if (!bBoundDirector)
	{
		if (UShootDirectorSubsystem* Director = GetWorld() ? GetWorld()->GetSubsystem<UShootDirectorSubsystem>() : nullptr)
		{
			Director->OnKillScored.AddUniqueDynamic(this, &AReshootHUD::HandleKillScored);
			bBoundDirector = true;
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
	}
	// Les éliminations arrivent par HandleKillScored ; ici les gros mouvements : bonus de prise ou d'événement, reprise.
	if (Delta >= 30.f)
	{
		FloatingGains.Add({ FString::Printf(TEXT("+%d  BONUS"), FMath::RoundToInt(Delta)), Now(), FLinearColor(0.45f, 1.f, 0.5f, 1.f) });
	}
	else if (Delta <= -30.f)
	{
		FloatingGains.Add({ FString::Printf(TEXT("%d  RETAKE"), FMath::RoundToInt(Delta)), Now(), RecColor });
	}
}

void AReshootHUD::HandleKillScored(const FKillInfo& Kill, float Points, bool bDirectiveRespected)
{
	const UShootDirectorSubsystem* Director = GetWorld()->GetSubsystem<UShootDirectorSubsystem>();
	const bool bHasDirective = Director && Director->GetCurrentDirective();
	const TCHAR* Label = !bHasDirective ? TEXT("KILL") : (bDirectiveRespected ? TEXT("DIRECTOR LOVES IT") : TEXT("IGNORED ORDER"));
	const FLinearColor Color = !bHasDirective ? FLinearColor::White : (bDirectiveRespected ? AccentColor : FLinearColor(0.65f, 0.65f, 0.65f, 1.f));
	FloatingGains.Add({ FString::Printf(TEXT("+%d  %s"), FMath::RoundToInt(Points), Label), Now(), Color });
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
	/** 0 -> 1 avec un léger dépassement (rebond) : pour les apparitions. */
	float EaseOutBack(float T)
	{
		T = FMath::Clamp(T, 0.f, 1.f);
		const float C1 = 1.9f, C3 = C1 + 1.f;
		return 1.f + C3 * FMath::Pow(T - 1.f, 3.f) + C1 * FMath::Pow(T - 1.f, 2.f);
	}

	/** Coup de poing d'échelle : 1 + Amp au départ, revient à 1 en Duration secondes. */
	float Punch(double Since, float Amp, float Duration)
	{
		if (Since < 0.0 || Since > Duration)
		{
			return 1.f;
		}
		const float T = static_cast<float>(Since) / Duration;
		return 1.f + Amp * (1.f - EaseOutBack(T));
	}
}

FSlateFontInfo AReshootHUD::MakeFont(float Scale) const
{
	const int32 Size = FMath::Max(6, FMath::RoundToInt(FontPixelSize * Scale * UIScale() * TextScale));
	FSlateFontInfo Info(Font, Size, FontTypeface);
	Info.OutlineSettings.OutlineSize = FMath::Max(1, FMath::RoundToInt(Size / 12.f));
	Info.OutlineSettings.OutlineColor = FLinearColor(0.f, 0.f, 0.f, 0.85f);
	return Info;
}

FVector2D AReshootHUD::MeasureText(const FString& Text, float Scale) const
{
	if (!Font || Text.IsEmpty() || !FSlateApplication::IsInitialized())
	{
		return FVector2D::ZeroVector;
	}
	const TSharedRef<FSlateFontMeasure> Measure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
	return Measure->Measure(Text, MakeFont(Scale));
}

void AReshootHUD::DrawTextAt(const FString& Text, float X, float Y, float Scale, const FLinearColor& Color, bool bCenterX, bool bShadow)
{
	if (!Canvas || !Font || Text.IsEmpty() || Color.A <= 0.01f)
	{
		return;
	}
	// Police vectorielle rendue à sa taille réelle (nette), contour noir ; ombre portée douce en plus.
	FSlateFontInfo Info = MakeFont(Scale);
	Info.OutlineSettings.OutlineColor.A *= Color.A;
	FCanvasTextItem Item(FVector2D(X, Y), FText::FromString(Text), Info, Color);
	Item.bCentreX = bCenterX;
	if (bShadow)
	{
		const float Off = FMath::Max(2.f, 3.f * Scale * UIScale());
		Item.EnableShadow(FLinearColor(0.f, 0.f, 0.f, Color.A * 0.45f), FVector2D(Off, Off));
	}
	Canvas->DrawItem(Item);
}

void AReshootHUD::DrawBar(float X, float Y, float W, float H, float Pct, const FLinearColor& Fill, const FLinearColor& Back)
{
	DrawRect(Back, X, Y, W, H);
	DrawRect(Fill, X, Y, W * FMath::Clamp(Pct, 0.f, 1.f), H);
}

void AReshootHUD::DrawPanel(float X, float Y, float W, float H, const FLinearColor& Stripe)
{
	DrawRect(FLinearColor(0.02f, 0.02f, 0.03f, 0.62f), X, Y, W, H);
	DrawRect(Stripe, X, Y, 5.f * UIScale(), H);
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

	TrackChanges();
	DrawCutBars();
	DrawDamageFlash();
	DrawTake();
	DrawDirective();
	DrawEventBanner();
	DrawAudience();
	DrawHealth();
	DrawWeapon();
	DrawSubtitle();
	DrawReticle();
	DrawSlate();
}

void AReshootHUD::TrackChanges()
{
	const UShootDirectorSubsystem* Director = GetWorld()->GetSubsystem<UShootDirectorSubsystem>();
	if (!Director)
	{
		return;
	}
	const double T = Now();
	if (Director->GetPhase() != LastPhase)
	{
		LastPhase = Director->GetPhase();
		PhaseChangeTime = T;
	}
	if (Director->GetCurrentUniverse() != LastUniverse)
	{
		LastUniverse = Director->GetCurrentUniverse();
		UniverseChangeTime = T;
	}
	if (Director->GetCurrentDirective() != LastDirective.Get())
	{
		LastDirective = Director->GetCurrentDirective();
		DirectiveChangeTime = T;
	}
	if (Director->GetEventHudState() != LastEventState)
	{
		LastEventState = Director->GetEventHudState();
		EventStateTime = T;
	}
	// Audimat affiché : il roule vers la vraie valeur au lieu de sauter.
	const AReshootGameState* GameState = GetWorld()->GetGameState<AReshootGameState>();
	const UAudienceRating* Rating = GameState ? GameState->GetAudienceRating() : nullptr;
	const float Target = Rating ? Rating->GetRating() : 0.f;
	const float Dt = static_cast<float>(FMath::Clamp(T - LastDrawTime, 0.0, 0.1));
	ShownRating = FMath::FInterpTo(ShownRating, Target, Dt, 8.f);
	LastDrawTime = T;
}

void AReshootHUD::DrawSlate()
{
	const UShootDirectorSubsystem* Director = GetWorld()->GetSubsystem<UShootDirectorSubsystem>();
	if (!Director)
	{
		return;
	}
	const double T = Now();
	const EShootPhase Phase = Director->GetPhase();
	// Grand clap à l'intro d'une scène ; petit clap à chaque bascule d'univers du final.
	const bool bSceneSlate = Phase == EShootPhase::SceneIntro;
	const bool bFinaleSlate = Phase == EShootPhase::Finale && T - UniverseChangeTime < 1.6;
	if (!bSceneSlate && !bFinaleSlate)
	{
		return;
	}
	const double Since = bSceneSlate ? T - PhaseChangeTime : T - UniverseChangeTime;
	const float Hold = bSceneSlate ? 2.8f : 1.2f;
	const float Size = bSceneSlate ? 1.f : 0.6f;

	const UUniverseDefinition* Definition = Director->GetUniverseDefinition(Director->GetCurrentUniverse());
	const FString Universe = Definition && !Definition->DisplayName.IsEmpty() ? Definition->DisplayName.ToString().ToUpper() : FString(TEXT("RESHOOT"));
	const FString Top = bSceneSlate ? FString::Printf(TEXT("SCENE %d"), FMath::Max(Director->GetSceneIndex(), 0) + 1) : FString(TEXT("DIRECTOR'S CUT"));

	const float S = UIScale() * Size;
	const float W = 620.f * S, H = 300.f * S, StickH = 64.f * S;
	// Entrée par le bas avec rebond, sortie vers le bas.
	const float In = EaseOutBack(static_cast<float>(Since) / 0.35f);
	const float Out = FMath::Clamp(static_cast<float>(Since - Hold) / 0.3f, 0.f, 1.f);
	const float Cx = Canvas->ClipX * 0.5f;
	const float BaseY = Canvas->ClipY * (bSceneSlate ? 0.5f : 0.3f) - H * 0.5f;
	const float Y = BaseY + (1.f - In) * Canvas->ClipY * 0.7f + Out * Out * Canvas->ClipY * 0.8f;
	const float X = Cx - W * 0.5f;

	// Ardoise.
	DrawRect(FLinearColor(0.02f, 0.02f, 0.02f, 0.96f), X, Y, W, H);
	DrawRect(FLinearColor(0.9f, 0.9f, 0.9f, 1.f), X, Y + H - 4.f * S, W, 4.f * S);
	DrawRect(FLinearColor(0.9f, 0.9f, 0.9f, 1.f), X + W * 0.5f - 2.f * S, Y + H * 0.58f, 4.f * S, H * 0.42f);

	// Claquette : ouverte à l'arrivée, elle se rabat à 0,45 s (flash blanc).
	const float CloseT = FMath::Clamp(static_cast<float>(Since - 0.25) / 0.2f, 0.f, 1.f);
	const float Angle = -28.f * (1.f - CloseT * CloseT);
	auto Stick = [&](float SY, float Rot)
	{
		FCanvasTileItem Bar(FVector2D(X, SY), FVector2D(W, StickH), FLinearColor(0.95f, 0.95f, 0.95f, 1.f));
		Bar.Rotation = FRotator(0.f, Rot, 0.f);
		Bar.PivotPoint = FVector2D(0.f, 1.f);
		Bar.BlendMode = SE_BLEND_Translucent;
		Canvas->DrawItem(Bar);
		// Bandes noires en biais (approchées par des tuiles tournées).
		for (int32 i = 0; i < 7; ++i)
		{
			const float Along = (40.f + i * 88.f) * S;
			const float Rad = FMath::DegreesToRadians(Rot);
			const FVector2D Pivot(X, SY + StickH);
			const FVector2D Local(Along, -StickH);
			const FVector2D P = Pivot + FVector2D(Local.X * FMath::Cos(Rad) - Local.Y * FMath::Sin(Rad), Local.X * FMath::Sin(Rad) + Local.Y * FMath::Cos(Rad));
			FCanvasTileItem Stripe(P, FVector2D(36.f * S, StickH * 1.05f), FLinearColor(0.05f, 0.05f, 0.05f, 1.f));
			Stripe.Rotation = FRotator(0.f, Rot + 25.f, 0.f);
			Stripe.PivotPoint = FVector2D(0.f, 0.f);
			Stripe.BlendMode = SE_BLEND_Translucent;
			Canvas->DrawItem(Stripe);
		}
	};
	Stick(Y - StickH, Angle);

	// Textes de l'ardoise.
	const float Pop = Punch(Since - 0.45, 0.25f, 0.3f);
	DrawTextAt(Top, Cx, Y + 26.f * S, 2.2f * Size * Pop, FLinearColor::White, true);
	DrawTextAt(Universe, Cx, Y + 110.f * S, 1.9f * Size * Pop, AccentColor, true);
	if (bSceneSlate)
	{
		DrawTextAt(TEXT("TAKE 1"), X + W * 0.25f, Y + H * 0.66f, 1.1f, FLinearColor::White, true);
		DrawTextAt(TEXT("DIR. THE DIRECTOR"), X + W * 0.75f, Y + H * 0.66f, 0.8f, FLinearColor(0.75f, 0.75f, 0.75f, 1.f), true);
	}

	// Flash du clap.
	const double FlashAge = Since - 0.45;
	if (FlashAge >= 0.0 && FlashAge < 0.18)
	{
		DrawRect(FLinearColor(1.f, 1.f, 1.f, 0.35f * (1.f - static_cast<float>(FlashAge / 0.18))), 0.f, 0.f, Canvas->ClipX, Canvas->ClipY);
	}
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
	const double Since = Now() - PhaseChangeTime;
	const float Slam = Punch(Since, 1.6f, 0.22f);
	const float Shake = Since < 0.3 ? (1.f - static_cast<float>(Since / 0.3)) * 10.f * UIScale() : 0.f;
	const float SX = FMath::Sin(Now() * 90.0) * Shake, SY = FMath::Cos(Now() * 77.0) * Shake;
	DrawTextAt(TEXT("CUT!"), Canvas->ClipX * 0.5f + SX, BarH + 14.f * UIScale() + SY, 3.6f * Slam, FLinearColor(1.f, 0.92f, 0.85f, 1.f), true);
	if (Since < 0.12)
	{
		DrawRect(FLinearColor(1.f, 1.f, 1.f, 0.25f * (1.f - static_cast<float>(Since / 0.12))), 0.f, 0.f, Canvas->ClipX, Canvas->ClipY);
	}
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
	const float X = 32.f * S, Y = 28.f * S;
	const UShootDirectorSubsystem* Director = GetWorld()->GetSubsystem<UShootDirectorSubsystem>();
	const int32 Scene = Director ? FMath::Max(Director->GetSceneIndex(), 0) + 1 : 1;
	const EShootPhase Phase = Director ? Director->GetPhase() : EShootPhase::TakeRunning;
	// À l'intro d'une scène, le réalisateur garde encore l'index de prise de la scène précédente.
	const bool bNewScene = Phase == EShootPhase::SceneIntro || Phase == EShootPhase::SceneTransition;
	const int32 Take = (Director && !bNewScene) ? FMath::Max(Director->GetTakeIndex(), 0) + 1 : 1;

	FString Label;
	if (Phase == EShootPhase::Casting)
	{
		Label = TEXT("CASTING");
	}
	else if (Phase == EShootPhase::Finale)
	{
		Label = TEXT("DIRECTOR'S CUT");
	}
	else
	{
		Label = FString::Printf(TEXT("SCENE %d  ·  TAKE %d"), Scene, Take);
	}
	const FVector2D LabelSize = MeasureText(Label, 1.05f);
	DrawPanel(X, Y, LabelSize.X + 110.f * S, 50.f * S, RecColor);

	// Voyant REC : clignote pendant que la caméra tourne, fixe et gris sinon.
	const bool bRolling = (Phase == EShootPhase::TakeRunning || Phase == EShootPhase::Finale);
	const bool bBlinkOn = FMath::Fmod(Now(), 1.0) < 0.6;
	const float Dot = 14.f * S;
	DrawRect(bRolling ? (bBlinkOn ? RecColor : RecColor * 0.35f) : FLinearColor(0.35f, 0.35f, 0.35f, 1.f), X + 20.f * S, Y + 18.f * S, Dot, Dot);
	DrawTextAt(TEXT("REC"), X + 42.f * S, Y + 11.f * S, 0.85f, bRolling ? FLinearColor::White : FLinearColor(0.6f, 0.6f, 0.6f, 1.f));
	DrawTextAt(Label, X + 92.f * S, Y + 9.f * S, 1.05f, AccentColor);
}

void AReshootHUD::DrawDirective()
{
	const UShootDirectorSubsystem* Director = GetWorld()->GetSubsystem<UShootDirectorSubsystem>();
	const UDirective* Directive = Director ? Director->GetCurrentDirective() : nullptr;
	const bool bNewScene = Director && (Director->GetPhase() == EShootPhase::SceneIntro || Director->GetPhase() == EShootPhase::SceneTransition);
	if (!Directive || Directive->Title.IsEmpty() || bNewScene)
	{
		return;
	}
	const float S = UIScale();
	const FString Text = Directive->Title.ToString();
	const FString Sub = TEXT("DIRECTOR'S ORDER  ·  OBEY IT FOR +25 PER KILL");
	const FVector2D Size = MeasureText(Text, 1.45f);
	const FVector2D SubSize = MeasureText(Sub, 0.7f);
	const float W = FMath::Max(Size.X, SubSize.X) + 60.f * S;
	const float Cx = Canvas->ClipX * 0.5f, Y = 26.f * S;
	DrawPanel(Cx - W * 0.5f, Y, W, Size.Y + SubSize.Y + 24.f * S, AccentColor);
	const float Pop = Punch(Now() - DirectiveChangeTime, 0.6f, 0.35f);
	DrawTextAt(Sub, Cx, Y + 8.f * S, 0.7f, FLinearColor(0.75f, 0.75f, 0.75f, 1.f), true);
	DrawTextAt(Text, Cx, Y + 10.f * S + SubSize.Y, 1.45f * Pop, AccentColor, true);
}

void AReshootHUD::DrawEventBanner()
{
	const UShootDirectorSubsystem* Director = GetWorld()->GetSubsystem<UShootDirectorSubsystem>();
	const int32 State = Director ? Director->GetEventHudState() : 0;
	const UUniverseDefinition* Definition = Director ? Director->GetEventHudDefinition() : nullptr;
	// Pendant un CUT!, l'événement est figé : le bandeau revient avec « Action! ».
	if (State == 0 || !Definition || (State == 2 && Director->GetPhase() == EShootPhase::Cut))
	{
		return;
	}
	const float S = UIScale();
	const UShootScript* Script = Director->GetShootScript();
	const int32 Bonus = FMath::RoundToInt(Script ? Script->EventSurvivedBonus : 100.f);
	const FString Title = Definition->EventTitle.IsEmpty() ? FString(TEXT("PRODUCER'S SURPRISE")) : Definition->EventTitle.ToString().ToUpper();
	const FString Hint = Definition->EventHint.ToString();
	const float TimeLeft = Director->GetEventHudTimeLeft();

	FString Line1, Line2;
	FLinearColor Color = FLinearColor(0.4f, 0.8f, 1.f, 1.f);
	if (State == 1)
	{
		Line1 = FString::Printf(TEXT("INCOMING: %s  %.0f"), *Title, FMath::CeilToFloat(TimeLeft));
		Line2 = FString::Printf(TEXT("%s  ·  Survive it: +%d audience"), *Hint, Bonus);
	}
	else if (State == 2)
	{
		Line1 = FString::Printf(TEXT("%s  ·  %.0f s"), *Title, FMath::CeilToFloat(TimeLeft));
		Line2 = FString::Printf(TEXT("%s  ·  Survive it: +%d audience"), *Hint, Bonus);
	}
	else if (State == 3)
	{
		Line1 = FString::Printf(TEXT("%s SURVIVED!"), *Title);
		Line2 = FString::Printf(TEXT("+%d audience"), FMath::RoundToInt(Director->GetEventHudBonus()));
		Color = FLinearColor(0.45f, 1.f, 0.5f, 1.f);
	}
	else
	{
		Line1 = FString::Printf(TEXT("%s FAILED"), *Title);
		Line2 = TEXT("No bonus this time");
		Color = RecColor;
	}
	const bool bBlink = State == 1 && FMath::Fmod(Now(), 0.5) < 0.25;
	const FVector2D S1 = MeasureText(Line1, 1.3f);
	const FVector2D S2 = MeasureText(Line2, 0.85f);
	const float W = FMath::Max(S1.X, S2.X) + 60.f * S;
	const float Cx = Canvas->ClipX * 0.5f;
	const float Y = Canvas->ClipY * 0.2f;
	DrawPanel(Cx - W * 0.5f, Y, W, S1.Y + S2.Y + 26.f * S, Color);
	DrawTextAt(Line1, Cx, Y + 8.f * S, 1.3f * Punch(Now() - EventStateTime, 0.5f, 0.35f), bBlink ? FLinearColor::White : Color, true);
	DrawTextAt(Line2, Cx, Y + 12.f * S + S1.Y, 0.85f, FLinearColor(0.9f, 0.9f, 0.9f, 1.f), true);
	if (State == 2)
	{
		const float Total = 10.f;
		DrawBar(Cx - W * 0.5f + 5.f * S, Y + S1.Y + S2.Y + 22.f * S, W - 5.f * S, 4.f * S, TimeLeft / Total, Color, FLinearColor(0.f, 0.f, 0.f, 0.5f));
	}
}

void AReshootHUD::DrawAudience()
{
	const UShootDirectorSubsystem* Director = GetWorld()->GetSubsystem<UShootDirectorSubsystem>();
	if (Director && Director->GetPhase() == EShootPhase::Casting)
	{
		return;	// pas encore de public pendant le casting
	}
	const float S = UIScale();
	const float W = 360.f * S, H = 14.f * S;
	const float PanelX = Canvas->ClipX - W - 64.f * S, PanelY = 28.f * S;
	const float X = PanelX + 20.f * S, Y = PanelY + 46.f * S;

	const AReshootGameState* GameState = GetWorld()->GetGameState<AReshootGameState>();
	const UAudienceRating* Rating = GameState ? GameState->GetAudienceRating() : nullptr;
	const float Pct = Rating ? FMath::Clamp(ShownRating / FMath::Max(Rating->MaxRating, 1.f), 0.f, 1.f) : 0.f;
	const float Value = ShownRating;
	const bool bBored = Director && Director->IsAudienceBored();

	// Pulse à chaque gain, rouge quand le public décroche.
	const float PulseAge = static_cast<float>(Now() - RatingPulseTime);
	const float Pulse = PulseAge < 0.35f ? 1.f - PulseAge / 0.35f : 0.f;
	const float PH = H * (1.f + 0.6f * Pulse);
	const bool bBlink = FMath::Fmod(Now(), 0.6) < 0.3;
	const FLinearColor BarColor = bBored ? (bBlink ? RecColor : RecColor * 0.6f) : FMath::Lerp(AccentColor, FLinearColor::White, Pulse);

	DrawPanel(PanelX, PanelY, W + 40.f * S, 118.f * S, bBored ? RecColor : AccentColor);
	DrawTextAt(TEXT("AUDIENCE"), X, PanelY + 10.f * S, 0.95f, FLinearColor::White);
	const FString Count = FString::Printf(TEXT("%d"), FMath::RoundToInt(Value));
	const float CountPop = 1.f + 0.35f * Pulse;
	DrawTextAt(Count, X + W - MeasureText(Count, 0.95f * CountPop).X, PanelY + 10.f * S, 0.95f * CountPop, BarColor);
	DrawBar(X, Y - (PH - H) * 0.5f, W, PH, Pct, BarColor, FLinearColor(1.f, 1.f, 1.f, 0.12f));

	// Paliers d'étoiles : la note du film à la fin.
	if (Rating)
	{
		const int32 Stars = Rating->GetStars();
		for (int32 i = 0; i < Rating->StarThresholds.Num(); ++i)
		{
			const float TX = X + W * FMath::Clamp(Rating->StarThresholds[i] / Rating->MaxRating, 0.f, 1.f);
			DrawRect(FLinearColor(1.f, 1.f, 1.f, 0.5f), TX - 1.f, Y - 3.f * S, 2.f, H + 6.f * S);
			DrawTextAt(TEXT("*"), TX + 2.f * S, Y + H, 0.8f, i < Stars ? AccentColor : FLinearColor(0.35f, 0.35f, 0.35f, 1.f));
		}
	}

	// Ce qui fait monter ou descendre la jauge, dit en une ligne.
	const UDirective* Directive = Director ? Director->GetCurrentDirective() : nullptr;
	const FString Hint = bBored ? TEXT("BORED! KILL TO WIN THEM BACK")
		: (Directive ? TEXT("FOLLOW THE ORDER FOR MORE POINTS") : TEXT("KILLS KEEP THEM WATCHING"));
	DrawTextAt(Hint, X, Y + H + 24.f * S, 0.72f, bBored ? RecColor : FLinearColor(0.7f, 0.7f, 0.7f, 1.f));

	// « +25 » qui montent depuis la jauge, avec leur raison.
	const double T = Now();
	FloatingGains.RemoveAll([T](const FFloatingGain& G) { return T - G.StartTime > 1.4; });
	float Stack = 0.f;
	for (int32 i = FloatingGains.Num() - 1; i >= 0; --i)
	{
		const FFloatingGain& Gain = FloatingGains[i];
		const float Age = static_cast<float>(T - Gain.StartTime) / 1.4f;
		const FLinearColor C(Gain.Color.R, Gain.Color.G, Gain.Color.B, 1.f - Age * Age);
		const float GainScale = Punch(T - Gain.StartTime, 0.7f, 0.25f);
		DrawTextAt(Gain.Text, X + W - MeasureText(Gain.Text, GainScale).X, PanelY + 132.f * S + Stack - 20.f * S * Age, GainScale, C);
		Stack += 30.f * S;
	}

	// Combo médiéval sous l'audimat.
	const AReshootCharacter* Player = Cast<AReshootCharacter>(GetOwningPawn());
	const AMedievalSword* Sword = Player ? Cast<AMedievalSword>(Player->GetCurrentWeapon()) : nullptr;
	if (Sword && Sword->GetCombo() > 0)
	{
		const float CY = PanelY + 132.f * S + Stack + 10.f * S;
		const int32 Mult = Sword->GetComboMultiplier();
		const FLinearColor C = Mult >= 4 ? FLinearColor(1.f, 0.3f, 0.15f, 1.f) : (Mult >= 2 ? AccentColor : FLinearColor::White);
		DrawTextAt(FString::Printf(TEXT("COMBO %d   x%d"), Sword->GetCombo(), Mult), X, CY, 1.2f, C);
		DrawBar(X, CY + 36.f * S, W * 0.6f, 5.f * S, Sword->GetComboTimeLeftPct(), C, FLinearColor(0.f, 0.f, 0.f, 0.5f));
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
	const float W = 320.f * S, H = 12.f * S;
	const float PanelX = 32.f * S, PanelY = Canvas->ClipY - 104.f * S;
	const float X = PanelX + 20.f * S, Y = PanelY + 62.f * S;
	const float Pct = Health->GetHealthPct();
	const FLinearColor Fill = Pct > 0.5f ? FLinearColor(0.92f, 0.92f, 0.92f, 1.f) : (Pct > 0.25f ? AccentColor : RecColor);
	DrawPanel(PanelX, PanelY, W + 40.f * S, 86.f * S, Fill);
	DrawTextAt(TEXT("HEALTH"), X, PanelY + 10.f * S, 0.8f, FLinearColor(0.7f, 0.7f, 0.7f, 1.f));
	const FString Value = FString::Printf(TEXT("%d"), FMath::RoundToInt(Health->GetHealth()));
	const float HitPop = Punch(Now() - DamageFlashTime, 0.3f, 0.25f);
	DrawTextAt(Value, X + W - MeasureText(Value, 1.3f * HitPop).X, PanelY + 4.f * S, 1.3f * HitPop, Pct > 0.25f ? FLinearColor::White : RecColor);
	DrawBar(X, Y, W, H, Pct, Fill, FLinearColor(1.f, 1.f, 1.f, 0.12f));
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
		const EMedievalWeaponMode Mode = Sword->GetMedievalMode();
		const TCHAR* Labels[] = { TEXT("1 SWORD"), TEXT("2 BOW"), TEXT("3 FISTS") };
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
			const FString Guard = Mode == EMedievalWeaponMode::Fists ? TEXT("GUARD") : TEXT("SHIELD UP");
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

	if (Sword && Sword->GetMedievalMode() == EMedievalWeaponMode::Bow)
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
		const float R = (Sword->GetMedievalMode() == EMedievalWeaponMode::Fists ? 12.f : 20.f) * S;
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
