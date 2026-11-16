#include "UI/ReshootHUD.h"
#include "Core/HealthComponent.h"
#include "Core/ReshootCharacter.h"
#include "Core/ReshootEnemy.h"
#include "Core/ReshootGameMode.h"
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
#include "Engine/FontFace.h"
#include "Fonts/CompositeFont.h"
#include "Fonts/SlateFontInfo.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/SlateRenderer.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"
#include "AudioDevice.h"
#include "Components/AudioComponent.h"
#include "Engine/Engine.h"
#include "Engine/Texture2D.h"
#include "GameFramework/GameUserSettings.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/ConfigCacheIni.h"
#include "Sound/SoundBase.h"

AReshootHUD::AReshootHUD()
{
	static ConstructorHelpers::FObjectFinder<UFont> RobotoFont(TEXT("/Engine/EngineFonts/Roboto.Roboto"));
	if (RobotoFont.Succeeded())
	{
		Font = RobotoFont.Object;
	}
	// Titres en Bebas Neue (SIL OFL, SourceArt/UI/Fonts) : police d'affiche de cinéma, construite ici à partir de la
	// fonte importée. Sous-objet du HUD : la fonte est cuite avec le jeu. Typographie nommée « Bold » comme FontTypeface.
	static ConstructorHelpers::FObjectFinder<UFontFace> BebasFace(TEXT("/Game/RESHOOT/UI/Fonts/FF_BebasNeue.FF_BebasNeue"));
	if (BebasFace.Succeeded())
	{
		UFont* Display = CreateDefaultSubobject<UFont>(TEXT("DisplayFont"));
		Display->FontCacheType = EFontCacheType::Runtime;
		FTypefaceEntry& Entry = Display->GetMutableInternalCompositeFont().DefaultTypeface.Fonts.AddDefaulted_GetRef();
		Entry.Name = TEXT("Bold");
		Entry.Font = FFontData(BebasFace.Object);
		Font = Display;
	}

	// Bruitages des menus (Tools/synth_ui_sfx.py, importés par Tools/setup_ui_sounds.py).
	static ConstructorHelpers::FObjectFinder<USoundBase> ClapFinder(TEXT("/Game/RESHOOT/UI/Audio/SW_UI_Clap.SW_UI_Clap"));
	static ConstructorHelpers::FObjectFinder<USoundBase> HoverFinder(TEXT("/Game/RESHOOT/UI/Audio/SW_UI_Hover.SW_UI_Hover"));
	static ConstructorHelpers::FObjectFinder<USoundBase> ClickFinder(TEXT("/Game/RESHOOT/UI/Audio/SW_UI_Click.SW_UI_Click"));
	static ConstructorHelpers::FObjectFinder<USoundBase> WhooshFinder(TEXT("/Game/RESHOOT/UI/Audio/SW_UI_Whoosh.SW_UI_Whoosh"));
	static ConstructorHelpers::FObjectFinder<USoundBase> ProjectorFinder(TEXT("/Game/RESHOOT/UI/Audio/SW_UI_Projector.SW_UI_Projector"));
	ClapSound = ClapFinder.Object;
	HoverSound = HoverFinder.Object;
	ClickSound = ClickFinder.Object;
	WhooshSound = WhooshFinder.Object;
	ProjectorSound = ProjectorFinder.Object;
}

void AReshootHUD::BeginPlay()
{
	Super::BeginPlay();
	TryBind();
	VignetteTexture = MakeRadialTexture(true);
	GlowTexture = MakeRadialTexture(false);
	LoadSettings();
	ApplySettings();

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
	if (ProjectorComponent)
	{
		ProjectorComponent->Stop();
		ProjectorComponent = nullptr;
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

namespace
{
	/** Les phrases (sous-titres, consignes) gardent une police de lecture : Bebas n'a que des capitales. */
	bool IsSentence(const FString& Text)
	{
		for (const TCHAR Ch : Text)
		{
			if (FChar::IsLower(Ch))
			{
				return true;
			}
		}
		return false;
	}

	const UFont* BodyFont()
	{
		static TWeakObjectPtr<const UFont> Cached;
		if (!Cached.IsValid())
		{
			Cached = LoadObject<UFont>(nullptr, TEXT("/Engine/EngineFonts/Roboto.Roboto"));
		}
		return Cached.Get();
	}
}

FSlateFontInfo AReshootHUD::MakeFontFor(const FString& Text, float Scale) const
{
	FSlateFontInfo Info = MakeFont(Scale);
	if (IsSentence(Text))
	{
		if (const UFont* Body = BodyFont())
		{
			Info.FontObject = Body;
			Info.TypefaceFontName = TEXT("Bold");
			Info.Size = FMath::Max(6.f, Info.Size * 0.82f);
		}
	}
	else
	{
		Info.Size *= 1.18f;	// Bebas est étroite : un peu plus grande pour le même encombrement
	}
	Info.OutlineSettings.OutlineSize = FMath::Max(1, FMath::RoundToInt(Info.Size / 12.f));
	return Info;
}

FVector2D AReshootHUD::MeasureText(const FString& Text, float Scale) const
{
	if (!Font || Text.IsEmpty() || !FSlateApplication::IsInitialized())
	{
		return FVector2D::ZeroVector;
	}
	const TSharedRef<FSlateFontMeasure> Measure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
	return Measure->Measure(Text, MakeFontFor(Text, Scale));
}

void AReshootHUD::DrawTextAt(const FString& Text, float X, float Y, float Scale, const FLinearColor& Color, bool bCenterX, bool bShadow)
{
	if (!Canvas || !Font || Text.IsEmpty() || Color.A <= 0.01f)
	{
		return;
	}
	// Police vectorielle rendue à sa taille réelle (nette), contour noir ; ombre portée douce en plus.
	FSlateFontInfo Info = MakeFontFor(Text, Scale);
	Info.OutlineSettings.OutlineColor.A *= Color.A;
	FCanvasTextItem Item(FVector2D(X, Y), FText::FromString(Text), Info, Color);
	Item.bCentreX = bCenterX;
	if (bShadow)
	{
		const float Off = FMath::Clamp(3.f * Scale * UIScale(), 2.f, 5.f * UIScale());
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
	PollMenuInput();
	UpdateMenus();

	if (!IsGameplayHudHidden())
	{
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

	switch (Menu)
	{
	case EMenuScreen::Main:		DrawMainMenu(); break;
	case EMenuScreen::Pause:	DrawPauseMenu(); break;
	case EMenuScreen::Options:	DrawOptionsMenu(); break;
	case EMenuScreen::End:		DrawEndScreen(); DrawSubtitle(); break;
	default: break;
	}
	DrawLaunchTransition();

	// Nombre d'entrées dessinées ce frame : la navigation clavier du frame suivant tourne dessus.
	MenuCount = MenuBuilt;
	MenuBuilt = 0;
	if (MenuCount > 0)
	{
		MenuSel = FMath::Clamp(MenuSel, 0, MenuCount - 1);
	}
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

	// Entrée par le bas avec rebond, sortie vers le bas.
	const float In = EaseOutBack(static_cast<float>(Since) / 0.35f);
	const float Out = FMath::Clamp(static_cast<float>(Since - Hold) / 0.3f, 0.f, 1.f);
	const float Cy = Canvas->ClipY * (bSceneSlate ? 0.5f : 0.3f) + (1.f - In) * Canvas->ClipY * 0.7f + Out * Out * Canvas->ClipY * 0.8f;
	DrawClapperboard(Canvas->ClipX * 0.5f, Cy, Size, Since, Top, Universe,
		bSceneSlate ? FString(TEXT("TAKE 1")) : FString(), bSceneSlate ? FString(TEXT("DIR. THE DIRECTOR")) : FString(), true);
}

void AReshootHUD::DrawClapperboard(float Cx, float Cy, float Size, double Since, const FString& Top, const FString& Middle,
	const FString& BottomLeft, const FString& BottomRight, bool bFlash)
{
	const float S = UIScale() * Size;
	const float W = 620.f * S, H = 300.f * S, StickH = 64.f * S;
	const float X = Cx - W * 0.5f;
	const float Y = Cy - H * 0.5f;

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
	DrawTextAt(Middle, Cx, Y + 110.f * S, 1.9f * Size * Pop, AccentColor, true);
	DrawTextAt(BottomLeft, X + W * 0.25f, Y + H * 0.66f, 1.1f * Size, FLinearColor::White, true);
	DrawTextAt(BottomRight, X + W * 0.75f, Y + H * 0.66f, 0.8f * Size, FLinearColor(0.75f, 0.75f, 0.75f, 1.f), true);

	// Flash du clap.
	const double FlashAge = Since - 0.45;
	if (bFlash && FlashAge >= 0.0 && FlashAge < 0.18)
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
		const TCHAR* Labels[] = { TEXT("1 SWORD"), TEXT("2 BOW") };
		float X = Right;
		for (int32 i = 1; i >= 0; --i)
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

// ---------------------------------------------------------------------------
// Menus : principal (salle de projection), pause (entracte), options, écran de fin.
// Tout est dessiné au Canvas ; la souris et le clavier/manette sont lus sur le PlayerController.

namespace
{
	const TCHAR* SettingsSection = TEXT("Reshoot.Settings");

	float EaseOutCubic(float T)
	{
		T = FMath::Clamp(T, 0.f, 1.f);
		return 1.f - FMath::Pow(1.f - T, 3.f);
	}

	float Appear01(double Since, double Delay, float Duration)
	{
		return FMath::Clamp(static_cast<float>(Since - Delay) / Duration, 0.f, 1.f);
	}

	bool IsPlayablePhase(EShootPhase Phase)
	{
		return Phase == EShootPhase::Casting || Phase == EShootPhase::SceneIntro || Phase == EShootPhase::TakeRunning
			|| Phase == EShootPhase::Cut || Phase == EShootPhase::SceneTransition || Phase == EShootPhase::Finale;
	}

	void SetCVar(const TCHAR* Name, float Value)
	{
		if (IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(Name))
		{
			Var->Set(Value, ECVF_SetByCode);
		}
	}

	// Commandes de test (console) : pilotent les menus sans clavier ni souris.
	bool GSimSelect = false;
	bool GSimPause = false;
	int32 GSimMove = 0;
	bool GSimShowEnd = false;
	FAutoConsoleCommand CmdUISelect(TEXT("reshoot.UI.Select"), TEXT("Menus : valide l'entrée sélectionnée."), FConsoleCommandDelegate::CreateLambda([]() { GSimSelect = true; }));
	FAutoConsoleCommand CmdUINext(TEXT("reshoot.UI.Next"), TEXT("Menus : entrée suivante."), FConsoleCommandDelegate::CreateLambda([]() { GSimMove = 1; }));
	FAutoConsoleCommand CmdUIPause(TEXT("reshoot.UI.Pause"), TEXT("Menus : comme la touche P."), FConsoleCommandDelegate::CreateLambda([]() { GSimPause = true; }));
	FAutoConsoleCommand CmdUIShowEnd(TEXT("reshoot.UI.ShowEnd"), TEXT("Menus : affiche l'écran de fin (test)."), FConsoleCommandDelegate::CreateLambda([]() { GSimShowEnd = true; }));
}

bool AReshootHUD::IsGameplayHudHidden() const
{
	const UShootDirectorSubsystem* Director = GetWorld()->GetSubsystem<UShootDirectorSubsystem>();
	const EShootPhase Phase = Director ? Director->GetPhase() : EShootPhase::TakeRunning;
	return Phase == EShootPhase::Menu || Phase == EShootPhase::Wrap || Menu == EMenuScreen::Main
		|| (Menu == EMenuScreen::Options && OptionsBack == EMenuScreen::Main);
}

bool AReshootHUD::WasPressed(const FKey& Key) const
{
	return KeysPressed.Contains(Key);
}

bool AReshootHUD::AnyPressed(std::initializer_list<FKey> Keys) const
{
	for (const FKey& Key : Keys)
	{
		if (KeysPressed.Contains(Key))
		{
			return true;
		}
	}
	return false;
}

void AReshootHUD::PollMenuInput()
{
	KeysPressed.Reset();
	bSelectPressed = false;
	bClickPressed = false;
	APlayerController* PC = GetOwningPlayerController();
	if (!PC)
	{
		return;
	}
	// Fronts montants maison : fonctionne aussi jeu en pause et quelle que soit la config d'Enhanced Input.
	static const FKey Watched[] = {
		EKeys::Escape, EKeys::P, EKeys::Enter, EKeys::SpaceBar, EKeys::Up, EKeys::Down, EKeys::Left, EKeys::Right,
		EKeys::Z, EKeys::S, EKeys::Q, EKeys::D, EKeys::W, EKeys::A, EKeys::LeftMouseButton,
		EKeys::Gamepad_Special_Right, EKeys::Gamepad_FaceButton_Bottom, EKeys::Gamepad_FaceButton_Right,
		EKeys::Gamepad_DPad_Up, EKeys::Gamepad_DPad_Down, EKeys::Gamepad_DPad_Left, EKeys::Gamepad_DPad_Right };
	for (const FKey& Key : Watched)
	{
		const bool bDown = PC->IsInputKeyDown(Key);
		bool& bWas = KeyWasDown.FindOrAdd(Key);
		if (bDown && !bWas)
		{
			KeysPressed.Add(Key);
		}
		bWas = bDown;
	}
	if (GSimPause)
	{
		GSimPause = false;
		KeysPressed.Add(EKeys::P);
	}
	if (GSimMove != 0)
	{
		KeysPressed.Add(EKeys::Down);
		GSimMove = 0;
	}
	if (GSimShowEnd)
	{
		GSimShowEnd = false;
		OpenMenu(EMenuScreen::End);
	}
	bMouseDown = PC->IsInputKeyDown(EKeys::LeftMouseButton);
	float MX = 0.f, MY = 0.f;
	if (PC->GetMousePosition(MX, MY))
	{
		MousePos = FVector2D(MX, MY);
		if (FVector2D::DistSquared(MousePos, LastMousePos) > 4.f)
		{
			bMouseUsed = true;
		}
		LastMousePos = MousePos;
	}
	if (Menu == EMenuScreen::None)
	{
		return;
	}
	bClickPressed = WasPressed(EKeys::LeftMouseButton);
	bSelectPressed = AnyPressed({ EKeys::Enter, EKeys::SpaceBar, EKeys::Gamepad_FaceButton_Bottom }) || GSimSelect;
	GSimSelect = false;
	if (MenuCount > 0)
	{
		// Écran de fin : boutons côte à côte, gauche/droite aussi.
		const bool bHorizontal = Menu == EMenuScreen::End;
		const bool bPrev = AnyPressed({ EKeys::Up, EKeys::Z, EKeys::W, EKeys::Gamepad_DPad_Up })
			|| (bHorizontal && AnyPressed({ EKeys::Left, EKeys::Q, EKeys::A, EKeys::Gamepad_DPad_Left }));
		const bool bNext = AnyPressed({ EKeys::Down, EKeys::S, EKeys::Gamepad_DPad_Down })
			|| (bHorizontal && AnyPressed({ EKeys::Right, EKeys::D, EKeys::Gamepad_DPad_Right }));
		if (bPrev || bNext)
		{
			MenuSel = (MenuSel + (bNext ? 1 : -1) + MenuCount) % MenuCount;
			bMouseUsed = false;
		}
	}
}

void AReshootHUD::UpdateMenus()
{
	const UShootDirectorSubsystem* Director = GetWorld()->GetSubsystem<UShootDirectorSubsystem>();
	const EShootPhase Phase = Director ? Director->GetPhase() : EShootPhase::TakeRunning;
	const AReshootGameMode* GameMode = GetWorld()->GetAuthGameMode<AReshootGameMode>();
	// Menu principal tant que le GameMode l'attend et que le tournage n'a pas commencé (lancé par un autre chemin).
	const bool bMainPending = GameMode && GameMode->IsMainMenuPending() && Phase == EShootPhase::Menu;

	if (bMainPending && !bLaunchStarted && Menu != EMenuScreen::Main && Menu != EMenuScreen::Options)
	{
		OpenMenu(EMenuScreen::Main);
	}
	else if (!bMainPending && !bLaunchStarted && (Menu == EMenuScreen::Main || (Menu == EMenuScreen::Options && OptionsBack == EMenuScreen::Main)))
	{
		OpenMenu(EMenuScreen::None);
	}
	// Le joueur ne bouge pas derrière le menu.
	if (bMainPending)
	{
		if (AReshootCharacter* Player = Cast<AReshootCharacter>(GetOwningPawn()))
		{
			if (!Player->IsFrozen())
			{
				Player->SetFrozen(true);
			}
		}
	}
	// Clap de fin : l'écran de fin arrive juste après le « Coupez ! » final.
	if (Phase == EShootPhase::Wrap && Menu != EMenuScreen::End && Now() - PhaseChangeTime > 0.4)
	{
		OpenMenu(EMenuScreen::End);
	}
	// Pause : P (ou Échap hors éditeur, Start à la manette). Retour arrière dans les options.
	if (AnyPressed({ EKeys::Escape, EKeys::P, EKeys::Gamepad_Special_Right, EKeys::Gamepad_FaceButton_Right }))
	{
		const bool bBackOnly = WasPressed(EKeys::Gamepad_FaceButton_Right);
		if (Menu == EMenuScreen::None && !bBackOnly && IsPlayablePhase(Phase) && !bLaunchStarted)
		{
			PlayUISound(ClickSound);
			OpenMenu(EMenuScreen::Pause);
		}
		else if (Menu == EMenuScreen::Pause)
		{
			PlayUISound(ClickSound);
			OpenMenu(EMenuScreen::None);
		}
		else if (Menu == EMenuScreen::Options)
		{
			PlayUISound(ClickSound);
			OpenMenu(OptionsBack);
		}
	}
	// Curseur et entrées de jeu suivent l'état du menu (le PlayerController peut arriver après le premier frame).
	SetMenuInput(Menu != EMenuScreen::None);
	if (MenuSel != LastSel)
	{
		if (LastSel >= 0 && Menu != EMenuScreen::None)
		{
			PlayUISound(HoverSound, 0.7f);
		}
		LastSel = MenuSel;
		SelChangeTime = Now();
	}
}

void AReshootHUD::OpenMenu(EMenuScreen Screen)
{
	Menu = Screen;
	MenuOpenTime = Now();
	MenuSel = 0;
	LastSel = -1;
	bSelectPressed = false;
	bClickPressed = false;
	const bool bPause = Screen == EMenuScreen::Pause || (Screen == EMenuScreen::Options && OptionsBack == EMenuScreen::Pause);
	UGameplayStatics::SetGamePaused(this, bPause);
	if (Screen == EMenuScreen::End)
	{
		bEndSlamPlayed = false;
		EndStarsPlayed = 0;
	}
	if (Screen == EMenuScreen::Main && ProjectorSound && !ProjectorComponent)
	{
		ProjectorComponent = UGameplayStatics::CreateSound2D(this, ProjectorSound, ProjectorVolume, 1.f, 0.f, nullptr, false, false);
		if (ProjectorComponent)
		{
			ProjectorComponent->bIsUISound = true;
			ProjectorComponent->FadeIn(1.5f, 1.f);
		}
	}
	SetMenuInput(Screen != EMenuScreen::None);
}

void AReshootHUD::SetMenuInput(bool bOn)
{
	APlayerController* PC = GetOwningPlayerController();
	if (!PC || bOn == bMenuInput)
	{
		return;
	}
	bMenuInput = bOn;
	PC->bShowMouseCursor = bOn;
	PC->SetIgnoreLookInput(bOn);
	PC->SetIgnoreMoveInput(bOn);
	if (bOn)
	{
		FInputModeGameAndUI Mode;
		Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		Mode.SetHideCursorDuringCapture(false);
		PC->SetInputMode(Mode);
		bMouseUsed = false;
	}
	else
	{
		PC->SetInputMode(FInputModeGameOnly());
	}
}

void AReshootHUD::PlayUISound(USoundBase* Sound, float Volume, float Pitch)
{
	if (Sound)
	{
		UGameplayStatics::PlaySound2D(this, Sound, Volume * MenuSoundVolume, Pitch, 0.f, nullptr, nullptr, true);
	}
}

// ---------------------------------------------------------------------------
// Widgets immédiats

bool AReshootHUD::MenuButton(const FString& Label, float Cx, float Y, float Scale, float Appear)
{
	const int32 Index = MenuBuilt++;
	const float S = UIScale();
	const float A = FMath::Clamp(Appear * 1.4f, 0.f, 1.f);
	if (A <= 0.f)
	{
		return false;
	}
	const FVector2D Size = MeasureText(Label, Scale);
	const float W = FMath::Max(Size.X + 90.f * S, 360.f * S), H = Size.Y + 20.f * S;
	const float Slide = (1.f - EaseOutBack(Appear)) * -80.f * S;
	const float X = Cx - W * 0.5f + Slide;
	const bool bHover = A > 0.6f && MousePos.X >= X && MousePos.X <= X + W && MousePos.Y >= Y && MousePos.Y <= Y + H;
	if (bHover && bMouseUsed)
	{
		MenuSel = Index;
	}
	const bool bSel = MenuSel == Index;
	if (bSel)
	{
		// Carte sombre + liseré qui s'étire, petites bandes de claquette à droite.
		const float Grow = EaseOutCubic(static_cast<float>(Now() - SelChangeTime) / 0.18f);
		DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.6f * A), X, Y, W, H);
		DrawRect(AccentColor * FLinearColor(1.f, 1.f, 1.f, A), X, Y, 6.f * S, H);
		DrawRect(AccentColor * FLinearColor(1.f, 1.f, 1.f, 0.9f * A), X, Y + H - 3.f * S, W * Grow, 3.f * S);
		for (int32 i = 0; i < 3; ++i)
		{
			FCanvasTileItem Stripe(FVector2D(X + W - (22.f + i * 18.f) * S, Y + 6.f * S), FVector2D(8.f * S, H - 12.f * S),
				FLinearColor(1.f, 1.f, 1.f, 0.18f * A));
			Stripe.Rotation = FRotator(0.f, 20.f, 0.f);
			Stripe.BlendMode = SE_BLEND_Translucent;
			Canvas->DrawItem(Stripe);
		}
	}
	const float Pop = bSel ? Punch(Now() - SelChangeTime, 0.12f, 0.25f) : 1.f;
	const FLinearColor C = bSel ? AccentColor : FLinearColor(0.82f, 0.82f, 0.82f, 1.f);
	DrawTextAt(Label, Cx + Slide, Y + 10.f * S, Scale * Pop, FLinearColor(C.R, C.G, C.B, A), true);
	const bool bActivated = A >= 1.f && bSel && (bSelectPressed || (bClickPressed && bHover));
	if (bActivated)
	{
		bSelectPressed = false;
		bClickPressed = false;
		PlayUISound(ClickSound);
	}
	return bActivated;
}

bool AReshootHUD::OptionSlider(const FString& Label, float& Value, float Min, float Max, float Step, float Cx, float Y, float Appear)
{
	const int32 Index = MenuBuilt++;
	const float S = UIScale();
	const float A = FMath::Clamp(Appear * 1.4f, 0.f, 1.f);
	const float W = 720.f * S, H = 56.f * S;
	const float X = Cx - W * 0.5f + (1.f - EaseOutBack(Appear)) * -80.f * S;
	const bool bHover = A > 0.6f && MousePos.X >= X && MousePos.X <= X + W && MousePos.Y >= Y && MousePos.Y <= Y + H;
	if (bHover && bMouseUsed)
	{
		MenuSel = Index;
	}
	const bool bSel = MenuSel == Index;
	DrawRect(FLinearColor(0.f, 0.f, 0.f, (bSel ? 0.6f : 0.35f) * A), X, Y, W, H);
	if (bSel)
	{
		DrawRect(AccentColor * FLinearColor(1.f, 1.f, 1.f, A), X, Y, 6.f * S, H);
	}
	const FLinearColor LabelColor = bSel ? AccentColor : FLinearColor::White;
	DrawTextAt(Label, X + 26.f * S, Y + 13.f * S, 0.95f, FLinearColor(LabelColor.R, LabelColor.G, LabelColor.B, A));

	const float Old = Value;
	const float BX = X + W * 0.46f, BW = W * 0.36f, BH = 8.f * S, BY = Y + H * 0.5f - BH * 0.5f;
	if (bSel && A >= 1.f)
	{
		if (AnyPressed({ EKeys::Left, EKeys::Q, EKeys::A, EKeys::Gamepad_DPad_Left }))
		{
			Value -= Step;
		}
		if (AnyPressed({ EKeys::Right, EKeys::D, EKeys::Gamepad_DPad_Right }))
		{
			Value += Step;
		}
		if (bMouseDown && bHover && MousePos.X >= BX - 12.f * S && MousePos.X <= BX + BW + 12.f * S)
		{
			Value = Min + (Max - Min) * FMath::Clamp((MousePos.X - BX) / BW, 0.f, 1.f);
			Value = FMath::GridSnap(Value, Step * 0.5f);
		}
	}
	Value = FMath::Clamp(Value, Min, Max);
	const float Pct = (Value - Min) / FMath::Max(Max - Min, KINDA_SMALL_NUMBER);
	const FLinearColor Fill = bSel ? AccentColor : FLinearColor(0.8f, 0.8f, 0.8f, 1.f);
	DrawBar(BX, BY, BW, BH, Pct, FLinearColor(Fill.R, Fill.G, Fill.B, A), FLinearColor(1.f, 1.f, 1.f, 0.15f * A));
	const float Knob = 18.f * S * (bSel ? Punch(Now() - SelChangeTime, 0.3f, 0.25f) : 1.f);
	DrawRect(FLinearColor(1.f, 1.f, 1.f, A), BX + BW * Pct - Knob * 0.25f, BY + BH * 0.5f - Knob * 0.5f, Knob * 0.5f, Knob);
	const FString ValueText = Max <= 1.f ? FString::Printf(TEXT("%d%%"), FMath::RoundToInt(Value * 100.f)) : FString::Printf(TEXT("X%.1f"), Value);
	DrawTextAt(ValueText, X + W - 24.f * S - MeasureText(ValueText, 0.95f).X, Y + 13.f * S, 0.95f, FLinearColor(1.f, 1.f, 1.f, A));
	const bool bChanged = !FMath::IsNearlyEqual(Old, Value);
	if (bChanged && !bMouseDown)
	{
		PlayUISound(HoverSound, 0.8f, 0.8f + 0.5f * Pct);
	}
	return bChanged;
}

bool AReshootHUD::OptionToggle(const FString& Label, bool& bValue, float Cx, float Y, float Appear)
{
	const int32 Index = MenuBuilt++;
	const float S = UIScale();
	const float A = FMath::Clamp(Appear * 1.4f, 0.f, 1.f);
	const float W = 720.f * S, H = 56.f * S;
	const float X = Cx - W * 0.5f + (1.f - EaseOutBack(Appear)) * -80.f * S;
	const bool bHover = A > 0.6f && MousePos.X >= X && MousePos.X <= X + W && MousePos.Y >= Y && MousePos.Y <= Y + H;
	if (bHover && bMouseUsed)
	{
		MenuSel = Index;
	}
	const bool bSel = MenuSel == Index;
	DrawRect(FLinearColor(0.f, 0.f, 0.f, (bSel ? 0.6f : 0.35f) * A), X, Y, W, H);
	if (bSel)
	{
		DrawRect(AccentColor * FLinearColor(1.f, 1.f, 1.f, A), X, Y, 6.f * S, H);
	}
	const FLinearColor LabelColor = bSel ? AccentColor : FLinearColor::White;
	DrawTextAt(Label, X + 26.f * S, Y + 13.f * S, 0.95f, FLinearColor(LabelColor.R, LabelColor.G, LabelColor.B, A));
	bool bChanged = false;
	if (bSel && A >= 1.f && (bSelectPressed || (bClickPressed && bHover)
		|| AnyPressed({ EKeys::Left, EKeys::Right, EKeys::Q, EKeys::D, EKeys::A, EKeys::Gamepad_DPad_Left, EKeys::Gamepad_DPad_Right })))
	{
		bValue = !bValue;
		bChanged = true;
		bSelectPressed = false;
		bClickPressed = false;
		PlayUISound(ClickSound);
	}
	// Interrupteur ON / OFF.
	const float PW = 120.f * S, PH = 34.f * S, PX = X + W - 24.f * S - PW, PY = Y + (H - PH) * 0.5f;
	DrawRect(FLinearColor(1.f, 1.f, 1.f, 0.12f * A), PX, PY, PW, PH);
	const FLinearColor Knob = bValue ? AccentColor : FLinearColor(0.4f, 0.4f, 0.4f, 1.f);
	DrawRect(FLinearColor(Knob.R, Knob.G, Knob.B, A), bValue ? PX + PW * 0.5f : PX, PY, PW * 0.5f, PH);
	const float Lum = bValue ? 0.f : 1.f;
	DrawTextAt(bValue ? TEXT("ON") : TEXT("OFF"), bValue ? PX + PW * 0.75f : PX + PW * 0.25f, PY + 3.f * S, 0.8f, FLinearColor(Lum, Lum, Lum, A), true, false);
	return bChanged;
}

// ---------------------------------------------------------------------------
// Décor : pellicule, grain, iris, étoiles

UTexture2D* AReshootHUD::MakeRadialTexture(bool bVignette)
{
	const int32 N = 128;
	UTexture2D* Texture = UTexture2D::CreateTransient(N, N, PF_B8G8R8A8);
	if (!Texture || !Texture->GetPlatformData() || Texture->GetPlatformData()->Mips.Num() == 0)
	{
		return nullptr;
	}
	Texture->Filter = TF_Bilinear;
	Texture->AddressX = TA_Clamp;
	Texture->AddressY = TA_Clamp;
	FTexture2DMipMap& Mip = Texture->GetPlatformData()->Mips[0];
	FColor* Pixels = static_cast<FColor*>(Mip.BulkData.Lock(LOCK_READ_WRITE));
	for (int32 Y = 0; Y < N; ++Y)
	{
		for (int32 X = 0; X < N; ++X)
		{
			const float DX = (X + 0.5f) / N * 2.f - 1.f, DY = (Y + 0.5f) / N * 2.f - 1.f;
			const float R = FMath::Sqrt(DX * DX + DY * DY);
			float Alpha;
			if (bVignette)
			{
				// Noir sur les bords, transparent au centre (coins = 1).
				Alpha = FMath::Pow(FMath::Clamp((R / UE_SQRT_2 - 0.3f) / 0.7f, 0.f, 1.f), 1.5f);
			}
			else
			{
				// Halo blanc doux, plein au centre.
				Alpha = FMath::Square(1.f - FMath::SmoothStep(0.f, 1.f, FMath::Clamp(R, 0.f, 1.f)));
			}
			// Halo prémultiplié : la couleur suit l'alpha, il marche aussi en additif (qui ignore l'alpha).
			const uint8 A8 = static_cast<uint8>(FMath::Clamp(Alpha, 0.f, 1.f) * 255.f);
			const uint8 C = bVignette ? 0 : A8;
			Pixels[Y * N + X] = FColor(C, C, C, A8);
		}
	}
	Mip.BulkData.Unlock();
	Texture->UpdateResource();
	return Texture;
}

void AReshootHUD::DrawTex(UTexture2D* Texture, float X, float Y, float W, float H, const FLinearColor& Color, bool bAdditive)
{
	if (!Texture || !Texture->GetResource())
	{
		return;
	}
	FCanvasTileItem Item(FVector2D(X, Y), Texture->GetResource(), FVector2D(W, H), Color);
	Item.BlendMode = bAdditive ? SE_BLEND_Additive : SE_BLEND_Translucent;
	Canvas->DrawItem(Item);
}

void AReshootHUD::DrawGrain(float Alpha)
{
	if (Alpha <= 0.f)
	{
		return;
	}
	// Grain de pellicule : change 24 fois par seconde, comme une vraie bobine.
	const int32 Frame = FMath::FloorToInt(Now() * 24.0);
	FRandomStream Rng(Frame * 7919 + 13);
	const float S = UIScale();
	for (int32 i = 0; i < 70; ++i)
	{
		const float Size = Rng.FRandRange(1.f, 3.2f) * S;
		const float Lum = Rng.FRand() < 0.5f ? 1.f : 0.f;
		DrawRect(FLinearColor(Lum, Lum, Lum, Rng.FRandRange(0.05f, 0.22f) * Alpha), Rng.FRand() * Canvas->ClipX, Rng.FRand() * Canvas->ClipY, Size, Size);
	}
	// Rayures verticales et poussières, de temps en temps.
	if (Rng.FRand() < 0.35f)
	{
		DrawRect(FLinearColor(1.f, 0.95f, 0.85f, 0.10f * Alpha), Rng.FRand() * Canvas->ClipX, 0.f, FMath::Max(1.f, 1.5f * S), Canvas->ClipY);
	}
	if (Rng.FRand() < 0.15f)
	{
		const float D = Rng.FRandRange(4.f, 9.f) * S;
		DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.35f * Alpha), Rng.FRand() * Canvas->ClipX, Rng.FRand() * Canvas->ClipY, D, D * Rng.FRandRange(0.4f, 1.6f));
	}
}

void AReshootHUD::DrawFilmBackdrop(float Alpha, bool bStrips)
{
	if (Alpha <= 0.f)
	{
		return;
	}
	const float S = UIScale();
	const float W = Canvas->ClipX, H = Canvas->ClipY;
	// Lumière de projecteur qui vacille légèrement.
	const float Flicker = 0.025f * FMath::Sin(Now() * 37.0) + 0.015f * FMath::Sin(Now() * 91.0);
	DrawRect(FLinearColor(0.012f, 0.01f, 0.012f, (0.84f + Flicker) * Alpha), 0.f, 0.f, W, H);
	DrawTex(GlowTexture, W * 0.5f - H * 0.75f, -H * 0.55f, H * 1.5f, H * 1.4f, FLinearColor(1.f, 0.78f, 0.45f, (0.10f + Flicker) * Alpha), true);
	DrawTex(VignetteTexture, 0.f, 0.f, W, H, FLinearColor(1.f, 1.f, 1.f, Alpha), false);

	if (bStrips)
	{
		// Bandes de pellicule qui défilent à gauche et à droite.
		const float SW = 64.f * S, Pitch = 46.f * S;
		const float Offset = FMath::Fmod(static_cast<float>(Now()) * 90.f * S, Pitch);
		for (int32 Side = 0; Side < 2; ++Side)
		{
			const float X = Side == 0 ? 0.f : W - SW;
			DrawRect(FLinearColor(0.035f, 0.03f, 0.03f, 0.95f * Alpha), X, 0.f, SW, H);
			for (float Y = -Pitch + Offset; Y < H; Y += Pitch)
			{
				DrawRect(FLinearColor(0.85f, 0.78f, 0.62f, 0.22f * Alpha), X + SW * 0.3f, Y, SW * 0.4f, Pitch * 0.45f);
			}
			DrawRect(FLinearColor(1.f, 1.f, 1.f, 0.06f * Alpha), Side == 0 ? X + SW - 2.f * S : X, 0.f, 2.f * S, H);
		}
	}
	DrawGrain(Alpha);
}

void AReshootHUD::DrawIris(float Radius01, float Cx, float Cy)
{
	if (Radius01 >= 1.f)
	{
		return;
	}
	const float Diag = FMath::Sqrt(FMath::Square(Canvas->ClipX) + FMath::Square(Canvas->ClipY));
	const float R = Radius01 * Diag;
	if (R <= 1.f || !GlowTexture || !GlowTexture->GetResource())
	{
		DrawRect(FLinearColor::Black, 0.f, 0.f, Canvas->ClipX, Canvas->ClipY);
		return;
	}
	// Anneau noir entre le rayon de l'iris et bien au-delà de l'écran (centre de la texture de halo = blanc plein).
	const float Big = Diag * 2.f;
	const int32 Segments = 96;
	TArray<FCanvasUVTri> Tris;
	Tris.Reserve(Segments * 2);
	const FVector2D UV(0.5f, 0.5f);
	const FVector2D C(Cx, Cy);
	for (int32 i = 0; i < Segments; ++i)
	{
		const float A0 = 2.f * PI * i / Segments, A1 = 2.f * PI * (i + 1) / Segments;
		const FVector2D D0(FMath::Cos(A0), FMath::Sin(A0)), D1(FMath::Cos(A1), FMath::Sin(A1));
		const FVector2D I0 = C + D0 * R, I1 = C + D1 * R, O0 = C + D0 * Big, O1 = C + D1 * Big;
		FCanvasUVTri T1;
		T1.V0_Pos = I0; T1.V1_Pos = O0; T1.V2_Pos = O1;
		T1.V0_UV = T1.V1_UV = T1.V2_UV = UV;
		T1.V0_Color = T1.V1_Color = T1.V2_Color = FLinearColor::Black;
		Tris.Add(T1);
		FCanvasUVTri T2 = T1;
		T2.V0_Pos = I0; T2.V1_Pos = O1; T2.V2_Pos = I1;
		Tris.Add(T2);
	}
	FCanvasTriangleItem Item(Tris, GlowTexture->GetResource());
	Item.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(Item);
	// Liseré doré de l'iris.
	const FLinearColor Rim(1.f, 0.78f, 0.35f, 0.5f);
	for (int32 i = 0; i < Segments; ++i)
	{
		const float A0 = 2.f * PI * i / Segments, A1 = 2.f * PI * (i + 1) / Segments;
		DrawLine(Cx + FMath::Cos(A0) * R, Cy + FMath::Sin(A0) * R, Cx + FMath::Cos(A1) * R, Cy + FMath::Sin(A1) * R, Rim, 2.f);
	}
}

void AReshootHUD::DrawStar(float Cx, float Cy, float R, const FLinearColor& Color)
{
	if (!GlowTexture || !GlowTexture->GetResource() || R <= 0.5f)
	{
		return;
	}
	TArray<FCanvasUVTri> Tris;
	const FVector2D C(Cx, Cy), UV(0.5f, 0.5f);
	for (int32 i = 0; i < 10; ++i)
	{
		const float A0 = -PI * 0.5f + PI * i / 5.f, A1 = -PI * 0.5f + PI * (i + 1) / 5.f;
		const float R0 = (i % 2 == 0) ? R : R * 0.45f, R1 = (i % 2 == 0) ? R * 0.45f : R;
		FCanvasUVTri T;
		T.V0_Pos = C;
		T.V1_Pos = C + FVector2D(FMath::Cos(A0), FMath::Sin(A0)) * R0;
		T.V2_Pos = C + FVector2D(FMath::Cos(A1), FMath::Sin(A1)) * R1;
		T.V0_UV = T.V1_UV = T.V2_UV = UV;
		T.V0_Color = T.V1_Color = T.V2_Color = Color;
		Tris.Add(T);
	}
	FCanvasTriangleItem Item(Tris, GlowTexture->GetResource());
	Item.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(Item);
}

// ---------------------------------------------------------------------------
// Écrans

void AReshootHUD::DrawMainMenu()
{
	const float S = UIScale();
	const float Cx = Canvas->ClipX * 0.5f;
	const double Since = Now() - MenuOpenTime;
	// Au lancement, tout glisse et s'efface pendant que le clap monte.
	const float Out = bLaunchStarted ? EaseOutCubic(static_cast<float>(Now() - LaunchTime) / 0.35f) : 0.f;
	const float Fade = 1.f - Out;

	DrawFilmBackdrop(1.f, true);

	// Timecode et voyant REC, comme dans un viseur.
	const double Clock = FMath::Max(Since, 0.0);
	const FString Timecode = FString::Printf(TEXT("REC  %02d:%02d:%02d"), static_cast<int32>(FMath::FloorToInt(Clock / 60.0) % 60), static_cast<int32>(FMath::FloorToInt(Clock) % 60), static_cast<int32>(FMath::FloorToInt(Clock * 24.0) % 24));
	const bool bBlink = FMath::Fmod(Now(), 1.0) < 0.6;
	DrawRect((bBlink ? RecColor : RecColor * 0.3f) * FLinearColor(1.f, 1.f, 1.f, Fade), 100.f * S, 46.f * S, 14.f * S, 14.f * S);
	DrawTextAt(Timecode, 124.f * S, 38.f * S, 0.9f, FLinearColor(1.f, 1.f, 1.f, 0.8f * Fade));
	const FString TakeLabel = TEXT("SCENE 0  ·  TAKE 1");
	DrawTextAt(TakeLabel, Canvas->ClipX - 100.f * S - MeasureText(TakeLabel, 0.9f).X, 38.f * S, 0.9f, FLinearColor(1.f, 1.f, 1.f, 0.8f * Fade));

	// « THE DIRECTOR PRESENTS », puis le titre lettre par lettre.
	const float TitleY = Canvas->ClipY * 0.17f - Out * 60.f * S;
	DrawTextAt(TEXT("THE DIRECTOR PRESENTS"), Cx, TitleY - 34.f * S, 0.95f, FLinearColor(0.85f, 0.85f, 0.85f, Appear01(Since, 0.1, 0.5f) * Fade), true);
	const FString Title = TEXT("RESHOOT");
	const float TitleScale = 6.2f;
	TArray<float> Widths;
	float Total = 0.f;
	const float Gap = 10.f * S;
	for (int32 i = 0; i < Title.Len(); ++i)
	{
		Widths.Add(MeasureText(Title.Mid(i, 1), TitleScale).X);
		Total += Widths.Last() + Gap;
	}
	Total -= Gap;
	DrawTex(GlowTexture, Cx - Total * 0.8f, TitleY - 80.f * S, Total * 1.6f, 360.f * S, FLinearColor(1.f, 0.6f, 0.25f, 0.16f * Appear01(Since, 0.6, 0.6f) * Fade), true);
	float LX = Cx - Total * 0.5f;
	for (int32 i = 0; i < Title.Len(); ++i)
	{
		const float T = Appear01(Since, 0.25 + i * 0.07, 0.45f);
		const float Drop = (1.f - EaseOutBack(T)) * -140.f * S;
		const float Wobble = FMath::Sin(Now() * 2.0 + i * 0.7) * 2.f * S;
		const FLinearColor C = FMath::Lerp(FLinearColor::White, AccentColor, FMath::Clamp(T * 1.2f, 0.f, 1.f));
		DrawTextAt(Title.Mid(i, 1), LX, TitleY + Drop + Wobble, TitleScale, FLinearColor(C.R, C.G, C.B, FMath::Min(T * 2.f, 1.f) * Fade));
		LX += Widths[i] + Gap;
	}
	// Trait rouge qui se déroule sous le titre, et l'accroche.
	const float LineY = TitleY + MeasureText(Title, TitleScale).Y + 6.f * S;
	const float LineW = Total * EaseOutCubic(Appear01(Since, 0.85, 0.4f));
	DrawRect(RecColor * FLinearColor(1.f, 1.f, 1.f, Fade), Cx - LineW * 0.5f, LineY, LineW, 5.f * S);
	DrawTextAt(TEXT("KNIGHTS  ·  STARSHIPS  ·  GANGSTERS  ·  AND ONE VERY ANGRY DIRECTOR"), Cx, LineY + 20.f * S, 0.9f,
		FLinearColor(0.85f, 0.85f, 0.85f, Appear01(Since, 1.0, 0.5f) * Fade), true);

	// Boutons.
	const float BY = Canvas->ClipY * 0.53f;
	const float Step = 82.f * S;
	auto ButtonAppear = [&](int32 i) { return bLaunchStarted ? Fade : Appear01(Since, 1.1 + i * 0.12, 0.4f); };
	const bool bPlay = MenuButton(TEXT("ACTION!"), Cx, BY, 2.f, ButtonAppear(0));
	const bool bOptions = MenuButton(TEXT("OPTIONS"), Cx, BY + Step * 1.15f, 1.5f, ButtonAppear(1));
	const bool bQuit = MenuButton(TEXT("QUIT"), Cx, BY + Step * 2.15f, 1.5f, ButtonAppear(2));
	if (!bLaunchStarted)
	{
		if (bPlay)
		{
			LaunchFromMenu();
		}
		else if (bOptions)
		{
			OptionsBack = EMenuScreen::Main;
			OpenMenu(EMenuScreen::Options);
		}
		else if (bQuit)
		{
			QuitGame();
		}
	}

	DrawTextAt(TEXT("MOUSE  OR  ARROWS + ENTER"), Cx, Canvas->ClipY - 70.f * S, 0.75f, FLinearColor(0.6f, 0.6f, 0.6f, Appear01(Since, 1.6, 0.5f) * Fade), true);
}

void AReshootHUD::LaunchFromMenu()
{
	bLaunchStarted = true;
	bLaunchDone = false;
	bLaunchClapPlayed = false;
	LaunchTime = Now();
	PlayUISound(WhooshSound, 0.7f);
	if (ProjectorComponent)
	{
		ProjectorComponent->FadeOut(1.2f, 0.f);
		ProjectorComponent = nullptr;
	}
}

void AReshootHUD::DrawLaunchTransition()
{
	if (!bLaunchStarted)
	{
		return;
	}
	const float T = static_cast<float>(Now() - LaunchTime);
	const float Cx = Canvas->ClipX * 0.5f, Cy = Canvas->ClipY * 0.5f;
	// 1) Le clap du casting monte (0,2 s) et claque (0,65 s).
	if (T < 1.7f)
	{
		const float Since = T - 0.2f;
		if (Since > 0.f)
		{
			const float In = EaseOutBack(Since / 0.35f);
			DrawClapperboard(Cx, Cy + (1.f - In) * Canvas->ClipY * 0.7f, 1.15f, Since, TEXT("CASTING"), TEXT("RESHOOT"), TEXT("TAKE 1"), TEXT("DIR. THE DIRECTOR"), true);
		}
		if (Since >= 0.45f && !bLaunchClapPlayed)
		{
			bLaunchClapPlayed = true;
			PlayUISound(ClapSound, 1.f);
		}
	}
	// 2) Fermeture à l'iris sur le clap (1,05 -> 1,6 s), noir, 3) le casting commence, 4) ouverture à l'iris.
	float Iris = 1.f;
	if (T < 1.6f)
	{
		const float K = FMath::Clamp((T - 1.05f) / 0.55f, 0.f, 1.f);
		Iris = 1.f - K * K;
	}
	else if (T < 2.1f)
	{
		Iris = 0.f;
	}
	else
	{
		Iris = EaseOutCubic((T - 2.1f) / 0.9f);
	}
	if (T >= 1.6f && !bLaunchDone)
	{
		bLaunchDone = true;
		if (AReshootCharacter* Player = Cast<AReshootCharacter>(GetOwningPawn()))
		{
			Player->SetFrozen(false);
		}
		OpenMenu(EMenuScreen::None);
		if (AReshootGameMode* GameMode = GetWorld()->GetAuthGameMode<AReshootGameMode>())
		{
			GameMode->StartFromMainMenu();
		}
	}
	DrawIris(Iris, Cx, Cy);
	if (T > 3.1f)
	{
		bLaunchStarted = false;
	}
}

void AReshootHUD::DrawPauseMenu()
{
	const float S = UIScale();
	const float Cx = Canvas->ClipX * 0.5f;
	const double Since = Now() - MenuOpenTime;
	const float A = EaseOutCubic(static_cast<float>(Since) / 0.25f);

	DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.62f * A), 0.f, 0.f, Canvas->ClipX, Canvas->ClipY);
	DrawTex(VignetteTexture, 0.f, 0.f, Canvas->ClipX, Canvas->ClipY, FLinearColor(1.f, 1.f, 1.f, A), false);
	const float Bar = Canvas->ClipY * 0.09f * A;
	DrawRect(FLinearColor::Black, 0.f, 0.f, Canvas->ClipX, Bar);
	DrawRect(FLinearColor::Black, 0.f, Canvas->ClipY - Bar, Canvas->ClipX, Bar);
	DrawGrain(0.7f * A);

	const float TitleY = Canvas->ClipY * 0.15f;
	DrawTextAt(TEXT("INTERMISSION"), Cx, TitleY, 4.2f * Punch(Since, 0.35f, 0.3f), FLinearColor(AccentColor.R, AccentColor.G, AccentColor.B, A), true);
	DrawTextAt(TEXT("The director is on a coffee break."), Cx, TitleY + MeasureText(TEXT("INTERMISSION"), 4.2f).Y + 4.f * S, 1.1f, FLinearColor(0.8f, 0.8f, 0.8f, A), true);

	const float BY = Canvas->ClipY * 0.37f, Step = 76.f * S;
	if (MenuButton(TEXT("RESUME"), Cx, BY, 1.5f, Appear01(Since, 0.05, 0.3f)))
	{
		OpenMenu(EMenuScreen::None);
		return;
	}
	if (MenuButton(TEXT("OPTIONS"), Cx, BY + Step, 1.5f, Appear01(Since, 0.1, 0.3f)))
	{
		OptionsBack = EMenuScreen::Pause;
		OpenMenu(EMenuScreen::Options);
		return;
	}
	if (MenuButton(TEXT("RESTART THE SHOOT"), Cx, BY + Step * 2.f, 1.5f, Appear01(Since, 0.15, 0.3f)))
	{
		TravelToStage(true);
		return;
	}
	if (MenuButton(TEXT("MAIN MENU"), Cx, BY + Step * 3.f, 1.5f, Appear01(Since, 0.2, 0.3f)))
	{
		TravelToStage(false);
		return;
	}
	if (MenuButton(TEXT("QUIT GAME"), Cx, BY + Step * 4.f, 1.5f, Appear01(Since, 0.25, 0.3f)))
	{
		QuitGame();
		return;
	}
	DrawTextAt(TEXT("P  /  ESC  TO RESUME"), Cx, Canvas->ClipY - Bar * 0.5f - 12.f * S, 0.75f, FLinearColor(0.6f, 0.6f, 0.6f, A), true, false);
}

void AReshootHUD::DrawOptionsMenu()
{
	const float S = UIScale();
	const float Cx = Canvas->ClipX * 0.5f;
	const double Since = Now() - MenuOpenTime;
	if (OptionsBack == EMenuScreen::Main)
	{
		DrawFilmBackdrop(1.f, true);
	}
	else
	{
		DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.7f), 0.f, 0.f, Canvas->ClipX, Canvas->ClipY);
		DrawTex(VignetteTexture, 0.f, 0.f, Canvas->ClipX, Canvas->ClipY, FLinearColor::White, false);
		DrawGrain(0.7f);
	}
	DrawTextAt(TEXT("OPTIONS"), Cx, Canvas->ClipY * 0.11f, 3.6f * Punch(Since, 0.3f, 0.3f), AccentColor, true);

	const float Y0 = Canvas->ClipY * 0.3f, Step = 72.f * S;
	bool bChanged = false;
	bChanged |= OptionSlider(TEXT("MASTER VOLUME"), MasterVolume, 0.f, 1.f, 0.1f, Cx, Y0, Appear01(Since, 0.0, 0.3f));
	bChanged |= OptionSlider(TEXT("MUSIC VOLUME"), MusicVolume, 0.f, 1.f, 0.1f, Cx, Y0 + Step, Appear01(Since, 0.05, 0.3f));
	bChanged |= OptionSlider(TEXT("MOUSE SENSITIVITY"), MouseSensitivity, 0.2f, 3.f, 0.1f, Cx, Y0 + Step * 2.f, Appear01(Since, 0.1, 0.3f));
	bChanged |= OptionToggle(TEXT("INVERT MOUSE Y"), bInvertY, Cx, Y0 + Step * 3.f, Appear01(Since, 0.15, 0.3f));
	bChanged |= OptionToggle(TEXT("FULLSCREEN"), bFullscreen, Cx, Y0 + Step * 4.f, Appear01(Since, 0.2, 0.3f));
	if (bChanged)
	{
		ApplySettings();
		SaveSettings();
	}
	if (MenuButton(TEXT("BACK"), Cx, Y0 + Step * 5.f + 24.f * S, 1.4f, Appear01(Since, 0.25, 0.3f)))
	{
		OpenMenu(OptionsBack);
	}
}

void AReshootHUD::DrawEndScreen()
{
	const float S = UIScale();
	const float Cx = Canvas->ClipX * 0.5f;
	const float T = static_cast<float>(Now() - MenuOpenTime);
	const float A = EaseOutCubic(T / 1.0f);

	const AReshootGameState* GameState = GetWorld()->GetGameState<AReshootGameState>();
	const UAudienceRating* Rating = GameState ? GameState->GetAudienceRating() : nullptr;
	const int32 Stars = Rating ? Rating->GetStars() : 0;
	const float FinalRating = Rating ? Rating->GetRating() : 0.f;

	DrawFilmBackdrop(A, false);
	const float Bar = Canvas->ClipY * 0.075f * A;
	DrawRect(FLinearColor::Black, 0.f, 0.f, Canvas->ClipX, Bar);
	DrawRect(FLinearColor::Black, 0.f, Canvas->ClipY - Bar, Canvas->ClipX, Bar);

	// « THAT'S A WRAP! » qui claque.
	const float SlamT = T - 0.6f;
	if (SlamT >= 0.f)
	{
		if (!bEndSlamPlayed)
		{
			bEndSlamPlayed = true;
			PlayUISound(ClapSound, 1.f, 0.9f);
		}
		const float Shake = SlamT < 0.3f ? (1.f - SlamT / 0.3f) * 9.f * S : 0.f;
		DrawTextAt(TEXT("THAT'S A WRAP!"), Cx + FMath::Sin(Now() * 90.0) * Shake, Canvas->ClipY * 0.1f + FMath::Cos(Now() * 70.0) * Shake,
			4.4f * Punch(SlamT, 1.2f, 0.25f), AccentColor, true);
		if (SlamT < 0.12f)
		{
			DrawRect(FLinearColor(1.f, 1.f, 1.f, 0.25f * (1.f - SlamT / 0.12f)), 0.f, 0.f, Canvas->ClipX, Canvas->ClipY);
		}
	}
	const FString Name = GameState ? GameState->GetPlayerName() : FString();
	const FString Starring = (Name.IsEmpty() || Name == TEXT("Extra")) ? FString(TEXT("RESHOOT  ·  STARRING YOU")) : FString::Printf(TEXT("RESHOOT  ·  STARRING %s"), *Name.ToUpper());
	DrawTextAt(Starring, Cx, Canvas->ClipY * 0.1f + MeasureText(TEXT("THAT'S A WRAP!"), 4.4f).Y + 2.f * S, 1.f, FLinearColor(0.85f, 0.85f, 0.85f, Appear01(T, 1.0, 0.4f)), true);

	// Étoiles, une par une, avec un clap à chaque étoile gagnée.
	const float StarY = Canvas->ClipY * 0.36f, StarR = 38.f * S, StarStep = 104.f * S;
	for (int32 i = 0; i < 5; ++i)
	{
		const float K = (T - (1.4f + 0.32f * i)) / 0.3f;
		if (K <= 0.f)
		{
			continue;
		}
		const bool bWon = i < Stars;
		if (EndStarsPlayed <= i)
		{
			EndStarsPlayed = i + 1;
			if (bWon)
			{
				PlayUISound(ClapSound, 0.55f, 1.1f + 0.08f * i);
			}
			else
			{
				PlayUISound(HoverSound, 0.8f, 0.7f);
			}
		}
		const float Sc = EaseOutBack(K);
		const float X = Cx + (i - 2) * StarStep;
		if (bWon)
		{
			DrawTex(GlowTexture, X - StarR * 2.2f, StarY - StarR * 2.2f, StarR * 4.4f, StarR * 4.4f, FLinearColor(1.f, 0.7f, 0.2f, 0.35f * FMath::Min(K, 1.f)), true);
		}
		DrawStar(X, StarY, StarR * Sc * 1.14f, FLinearColor(0.f, 0.f, 0.f, 0.8f));
		DrawStar(X, StarY, StarR * Sc, bWon ? AccentColor : FLinearColor(0.22f, 0.22f, 0.22f, 1.f));
	}

	// Audimat qui roule jusqu'au score final.
	const float CountK = EaseOutCubic(Appear01(T, 1.4, 1.6f));
	const FString Score = FString::Printf(TEXT("AUDIENCE  %d"), FMath::RoundToInt(FinalRating * CountK));
	DrawTextAt(Score, Cx, StarY + 62.f * S, 1.5f, FLinearColor(1.f, 1.f, 1.f, Appear01(T, 1.3, 0.3f)), true);

	// La critique.
	static const TCHAR* Quotes[] = {
		TEXT("\"A disaster. I want my popcorn back.\""),
		TEXT("\"Painful. The extras deserved better.\""),
		TEXT("\"Watchable. Barely.\""),
		TEXT("\"A solid B-movie. Bring friends.\""),
		TEXT("\"Explosive! A star is born.\""),
		TEXT("\"A masterpiece. Give this actor an Oscar.\"") };
	const float QuoteA = Appear01(T, 3.2, 0.5f);
	DrawTextAt(Quotes[FMath::Clamp(Stars, 0, 5)], Cx, StarY + 128.f * S, 1.15f, FLinearColor(1.f, 0.95f, 0.8f, QuoteA), true);
	DrawTextAt(TEXT("THE DAILY EXTRA"), Cx, StarY + 166.f * S, 0.75f, FLinearColor(0.6f, 0.6f, 0.6f, QuoteA), true);

	// Bilan du tournage.
	if (GameState)
	{
		const FShootStats& Stats = GameState->GetShootStats();
		const TPair<const TCHAR*, int32> Rows[] = {
			{ TEXT("KILLS"), Stats.Kills },
			{ TEXT("KILLS ON THE DIRECTOR'S ORDER"), Stats.DirectiveKills },
			{ TEXT("HEADSHOTS"), Stats.Headshots },
			{ TEXT("TAKES IN THE CAN"), Stats.CompletedTakes },
			{ TEXT("RETAKES"), Stats.FailedTakes } };
		const float PW = 620.f * S, RowH = 36.f * S;
		const float PX = Cx - PW * 0.5f, PY = Canvas->ClipY * 0.565f;
		const float PanelA = Appear01(T, 3.5, 0.3f);
		if (PanelA > 0.f)
		{
			DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.5f * PanelA), PX, PY - 10.f * S, PW, RowH * 5.f + 20.f * S);
			DrawRect(AccentColor * FLinearColor(1.f, 1.f, 1.f, PanelA), PX, PY - 10.f * S, 5.f * S, RowH * 5.f + 20.f * S);
		}
		for (int32 i = 0; i < 5; ++i)
		{
			const float RA = Appear01(T, 3.6 + 0.15 * i, 0.25f);
			if (RA <= 0.f)
			{
				continue;
			}
			const float Y = PY + RowH * i + (1.f - EaseOutCubic(RA)) * 14.f * S;
			DrawTextAt(Rows[i].Key, PX + 28.f * S, Y, 0.95f, FLinearColor(0.85f, 0.85f, 0.85f, RA));
			const FString Value = FString::Printf(TEXT("%d"), FMath::RoundToInt(Rows[i].Value * EaseOutCubic(Appear01(T, 3.6 + 0.15 * i, 0.6f))));
			DrawTextAt(Value, PX + PW - 28.f * S - MeasureText(Value, 1.05f).X, Y - 2.f * S, 1.05f, FLinearColor(AccentColor.R, AccentColor.G, AccentColor.B, RA));
		}
	}

	// Boutons côte à côte.
	const float BY = Canvas->ClipY * 0.82f;
	const float BStep = 400.f * S;
	if (MenuButton(TEXT("SHOOT AGAIN"), Cx - BStep, BY, 1.4f, Appear01(T, 4.8, 0.35f)))
	{
		TravelToStage(true);
		return;
	}
	if (MenuButton(TEXT("MAIN MENU"), Cx, BY, 1.4f, Appear01(T, 4.9, 0.35f)))
	{
		TravelToStage(false);
		return;
	}
	if (MenuButton(TEXT("QUIT"), Cx + BStep, BY, 1.4f, Appear01(T, 5.0, 0.35f)))
	{
		QuitGame();
		return;
	}

	// Générique qui défile dans la bande noire du bas.
	if (Bar > 1.f && !CreditsLine.IsEmpty())
	{
		const float CW = MeasureText(CreditsLine, 0.8f).X;
		const float Loop = CW + Canvas->ClipX;
		const float X = Canvas->ClipX - FMath::Fmod(T * 90.f * S, Loop);
		DrawTextAt(CreditsLine, X, Canvas->ClipY - Bar * 0.5f - 12.f * S, 0.8f, FLinearColor(0.75f, 0.75f, 0.75f, A), false, false);
	}
}

// ---------------------------------------------------------------------------
// Actions et réglages

void AReshootHUD::TravelToStage(bool bPlay)
{
	UGameplayStatics::SetGamePaused(this, false);
	const UShootDirectorSubsystem* Director = GetWorld()->GetSubsystem<UShootDirectorSubsystem>();
	// Recommencer pendant le casting refait le casting ; sinon on repart de la scène 1.
	const bool bInCasting = Director && Director->GetPhase() == EShootPhase::Casting;
	const FString Options = !bPlay ? FString() : (bInCasting ? FString(TEXT("Play")) : FString(TEXT("Play?SkipCasting")));
	UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this, true)), true, Options);
}

void AReshootHUD::QuitGame()
{
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayerController(), EQuitPreference::Quit, false);
}

void AReshootHUD::LoadSettings()
{
	if (!GConfig)
	{
		return;
	}
	GConfig->GetFloat(SettingsSection, TEXT("MasterVolume"), MasterVolume, GGameUserSettingsIni);
	GConfig->GetFloat(SettingsSection, TEXT("MusicVolume"), MusicVolume, GGameUserSettingsIni);
	GConfig->GetFloat(SettingsSection, TEXT("MouseSensitivity"), MouseSensitivity, GGameUserSettingsIni);
	GConfig->GetBool(SettingsSection, TEXT("InvertY"), bInvertY, GGameUserSettingsIni);
	if (const UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr)
	{
		bFullscreen = Settings->GetFullscreenMode() != EWindowMode::Windowed;
	}
	bSettingsLoaded = true;
}

void AReshootHUD::SaveSettings()
{
	if (!GConfig)
	{
		return;
	}
	GConfig->SetFloat(SettingsSection, TEXT("MasterVolume"), MasterVolume, GGameUserSettingsIni);
	GConfig->SetFloat(SettingsSection, TEXT("MusicVolume"), MusicVolume, GGameUserSettingsIni);
	GConfig->SetFloat(SettingsSection, TEXT("MouseSensitivity"), MouseSensitivity, GGameUserSettingsIni);
	GConfig->SetBool(SettingsSection, TEXT("InvertY"), bInvertY, GGameUserSettingsIni);
	GConfig->Flush(false, GGameUserSettingsIni);
}

void AReshootHUD::ApplySettings()
{
	if (FAudioDevice* Device = GEngine ? GEngine->GetMainAudioDeviceRaw() : nullptr)
	{
		Device->SetTransientPrimaryVolume(MasterVolume);
	}
	SetCVar(TEXT("reshoot.MusicVolume"), MusicVolume);
	SetCVar(TEXT("reshoot.LookSensitivity"), MouseSensitivity);
	SetCVar(TEXT("reshoot.InvertLook"), bInvertY ? 1.f : 0.f);
	if (UShootDirectorSubsystem* Director = GetWorld()->GetSubsystem<UShootDirectorSubsystem>())
	{
		Director->RefreshMusicVolume();
	}
	// Plein écran : seulement dans le jeu packagé (dans l'éditeur, la fenêtre est celle du PIE).
	UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr;
	if (Settings && !GIsEditor)
	{
		const EWindowMode::Type Wanted = bFullscreen ? EWindowMode::WindowedFullscreen : EWindowMode::Windowed;
		if (Settings->GetFullscreenMode() != Wanted)
		{
			Settings->SetFullscreenMode(Wanted);
			Settings->SetScreenResolution(bFullscreen ? Settings->GetDesktopResolution() : FIntPoint(1600, 900));
			Settings->ApplyResolutionSettings(false);
			Settings->SaveSettings();
		}
	}
}
