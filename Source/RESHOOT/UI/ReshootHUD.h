// RESHOOT — HUD de jeu, façon régie de tournage (GDD, section Interface). Dessiné au Canvas, sans asset :
//  - haut gauche : « Scène 2, prise 3 » + voyant REC ; haut centre : directive ; haut droite : audimat (pulse + « +25 »)
//  - bas gauche : vie ; bas droite : arme (munitions, mode médiéval, charge de l'arc) ; bas centre : sous-titres
//  - centre : réticule selon l'arme, marqueur de touche ; bord rouge quand le joueur est touché ; bandes noires au CUT!
//  - combo médiéval sous l'audimat quand l'arme médiévale est en main.
// Menus, dessinés au Canvas eux aussi (souris ou clavier/manette) :
//  - menu principal façon salle de projection, puis transition clap + iris vers le casting ;
//  - pause (P ou Échap) avec les options de base (volumes, sensibilité, axe inversé, plein écran) ;
//  - écran de fin au clap de fin (Wrap) : étoiles, critique, bilan, rejouer.
// L'interface écoute les delegates et lit les getters ; elle n'appelle jamais la logique de jeu.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "Core/ReshootTypes.h"
#include "InputCoreTypes.h"
#include "ReshootHUD.generated.h"

class UFont;
class UHealthComponent;
class AReshootEnemy;
class USoundBase;
class UTexture2D;
class UAudioComponent;

UCLASS(Blueprintable)
class RESHOOT_API AReshootHUD : public AHUD
{
	GENERATED_BODY()

public:
	AReshootHUD();

	virtual void DrawHUD() override;

	/** Police de l'interface (par défaut Roboto du moteur). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HUD")
	TObjectPtr<UFont> Font;

	/** Taille de texte de base, multipliée par la hauteur d'écran / 1080. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HUD", meta = (ClampMin = "0.1"))
	float TextScale = 1.f;

	/** Style de la police (nom d'une typographie de la police composite : Bold, Regular...). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HUD")
	FName FontTypeface = TEXT("Bold");

	/** Taille en pixels d'un texte d'échelle 1 à 1080p. Le texte est rendu à sa taille réelle : il reste net. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HUD", meta = (ClampMin = "4"))
	float FontPixelSize = 20.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HUD")
	FLinearColor AccentColor = FLinearColor(1.f, 0.78f, 0.25f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HUD")
	FLinearColor RecColor = FLinearColor(0.95f, 0.1f, 0.08f, 1.f);

	/** Durée du marqueur de touche au centre. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HUD", meta = (ClampMin = "0"))
	float HitMarkerDuration = 0.18f;

	/** Durée du bord rouge quand le joueur est touché. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HUD", meta = (ClampMin = "0"))
	float DamageFlashDuration = 0.45f;

	// --- Menus ---

	/** Clap du clap de cinéma (transition vers le casting, étoiles de l'écran de fin). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HUD|Menu")
	TObjectPtr<USoundBase> ClapSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HUD|Menu")
	TObjectPtr<USoundBase> HoverSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HUD|Menu")
	TObjectPtr<USoundBase> ClickSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HUD|Menu")
	TObjectPtr<USoundBase> WhooshSound;

	/** Ronron de projecteur en boucle sous le menu principal. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HUD|Menu")
	TObjectPtr<USoundBase> ProjectorSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HUD|Menu", meta = (ClampMin = "0"))
	float MenuSoundVolume = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HUD|Menu", meta = (ClampMin = "0"))
	float ProjectorVolume = 0.22f;

	/** Noms au générique de l'écran de fin. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HUD|Menu")
	FString CreditsLine = TEXT("A FILM BY JÉRÉMY · ILEANA · JULES · ENZO      MUSIC KEVIN MACLEOD (CC-BY 4.0)      TITLES BEBAS NEUE (SIL OFL)");

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	// Dessin
	float UIScale() const;
	void DrawTextAt(const FString& Text, float X, float Y, float Scale, const FLinearColor& Color, bool bCenterX = false, bool bShadow = true);
	FVector2D MeasureText(const FString& Text, float Scale) const;
	void DrawBar(float X, float Y, float W, float H, float Pct, const FLinearColor& Fill, const FLinearColor& Back);

	void DrawTake();
	void DrawDirective();
	void DrawAudience();
	void DrawHealth();
	void DrawWeapon();
	void DrawSubtitle();
	void DrawReticle();
	void DrawDamageFlash();
	void DrawCutBars();
	void DrawEventBanner();
	/** Clap de début de scène (et petit clap à chaque bascule du final). */
	void DrawSlate();
	/** Repère les changements de phase, d'univers, de directive et d'événement (départ des animations). */
	void TrackChanges();
	/** Police vectorielle à la bonne taille, avec contour. */
	struct FSlateFontInfo MakeFont(float Scale) const;
	/** Police selon le texte : titres en capitales (police d'affiche), phrases en police de lecture. */
	struct FSlateFontInfo MakeFontFor(const FString& Text, float Scale) const;
	/** Carte sombre avec un liseré de couleur à gauche : fond commun des blocs du HUD. */
	void DrawPanel(float X, float Y, float W, float H, const FLinearColor& Stripe);
	/** Clap de cinéma complet (ardoise + claquette qui se rabat à 0,45 s). */
	void DrawClapperboard(float Cx, float Cy, float Size, double Since, const FString& Top, const FString& Middle,
		const FString& BottomLeft, const FString& BottomRight, bool bFlash);

	// Menus
	enum class EMenuScreen : uint8 { None, Main, Pause, Options, End };
	void UpdateMenus();
	void PollMenuInput();
	bool WasPressed(const FKey& Key) const;
	bool AnyPressed(std::initializer_list<FKey> Keys) const;
	void OpenMenu(EMenuScreen Screen);
	void SetMenuInput(bool bOn);
	/** Bouton de menu : vrai le frame où il est activé (clic, Entrée). Appear de 0 à 1 pour l'entrée animée. */
	bool MenuButton(const FString& Label, float Cx, float Y, float Scale, float Appear);
	/** Ligne d'option à curseur : gauche/droite, clic sur la barre. Vrai si la valeur a changé. */
	bool OptionSlider(const FString& Label, float& Value, float Min, float Max, float Step, float Cx, float Y, float Appear);
	bool OptionToggle(const FString& Label, bool& bValue, float Cx, float Y, float Appear);
	void DrawMainMenu();
	void DrawPauseMenu();
	void DrawOptionsMenu();
	void DrawEndScreen();
	void DrawLaunchTransition();
	void DrawFilmBackdrop(float Alpha, bool bStrips);
	void DrawGrain(float Alpha);
	void DrawIris(float Radius01, float Cx, float Cy);
	void DrawStar(float Cx, float Cy, float R, const FLinearColor& Color);
	void DrawTex(UTexture2D* Texture, float X, float Y, float W, float H, const FLinearColor& Color, bool bAdditive);
	void LaunchFromMenu();
	void TravelToStage(bool bPlay);
	void QuitGame();
	void LoadSettings();
	void SaveSettings();
	void ApplySettings();
	void PlayUISound(USoundBase* Sound, float Volume = 1.f, float Pitch = 1.f);
	UTexture2D* MakeRadialTexture(bool bVignette);
	bool IsGameplayHudHidden() const;

	// Écoute
	void TryBind();
	void BindEnemy(AActor* Actor);
	void HandleActorSpawned(AActor* Actor);

	UFUNCTION()
	void HandleRatingChanged(float NewRating, float Delta);

	UFUNCTION()
	void HandleSubtitle(FName LineId, const FText& Subtitle, float Duration);

	UFUNCTION()
	void HandlePlayerHealthChanged(UHealthComponent* HealthComponent, float NewHealth, float Delta);

	UFUNCTION()
	void HandleEnemyHealthChanged(UHealthComponent* HealthComponent, float NewHealth, float Delta);

	UFUNCTION()
	void HandleKillScored(const FKillInfo& Kill, float Points, bool bDirectiveRespected);

	bool bBoundDirector = false;
	double BoredSince = -1.0;

	// Animations
	EShootPhase LastPhase = EShootPhase::Menu;
	double PhaseChangeTime = -100.0;
	EUniverse LastUniverse = EUniverse::None;
	double UniverseChangeTime = -100.0;
	TWeakObjectPtr<const class UDirective> LastDirective;
	double DirectiveChangeTime = -100.0;
	int32 LastEventState = 0;
	double EventStateTime = -100.0;
	float ShownRating = 0.f;
	double LastDrawTime = 0.0;

	double Now() const;

	bool bBoundGlobal = false;
	FTimerHandle BindRetryTimer;
	FDelegateHandle ActorSpawnedHandle;

	UPROPERTY(Transient)
	TObjectPtr<UHealthComponent> BoundPlayerHealth;

	FText SubtitleText;
	double SubtitleEndTime = 0.0;
	double RatingPulseTime = -100.0;
	double HitMarkerTime = -100.0;
	double DamageFlashTime = -100.0;

	struct FFloatingGain
	{
		FString Text;
		double StartTime = 0.0;
		FLinearColor Color = FLinearColor::White;
	};
	TArray<FFloatingGain> FloatingGains;

	// État des menus
	EMenuScreen Menu = EMenuScreen::None;
	EMenuScreen OptionsBack = EMenuScreen::None;
	double MenuOpenTime = -100.0;
	int32 MenuSel = 0;
	int32 MenuCount = 0;
	int32 MenuBuilt = 0;
	bool bMenuInput = false;
	bool bMouseUsed = false;
	bool bSelectPressed = false;
	bool bClickPressed = false;
	FVector2D MousePos = FVector2D::ZeroVector;
	FVector2D LastMousePos = FVector2D::ZeroVector;
	TMap<FKey, bool> KeyWasDown;
	TSet<FKey> KeysPressed;
	double LaunchTime = -1.0;
	bool bLaunchStarted = false;
	bool bEndShown = false;
	int32 EndStarsPlayed = 0;
	bool bEndSlamPlayed = false;
	bool bSettingsLoaded = false;
	bool bMouseDown = false;
	bool bLaunchClapPlayed = false;
	bool bLaunchDone = false;
	double SelChangeTime = -100.0;
	int32 LastSel = -1;

	// Réglages (GameUserSettings.ini, section [Reshoot.Settings])
	float MasterVolume = 1.f;
	float MusicVolume = 1.f;
	float MouseSensitivity = 1.f;
	bool bInvertY = false;
	bool bFullscreen = true;

	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> VignetteTexture;

	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> GlowTexture;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> ProjectorComponent;
};
