#include "ElderbreachBrawlersGameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Subsystems/WidgetSequenceManager.h"
#include "MainGameMode.h"
#include "Actors/FightingCameraActor.h"

void UElderbreachBrawlersGameInstance::Init()
{
	Super::Init();
}

void UElderbreachBrawlersGameInstance::OnStart()
{
	Super::OnStart();

	// Arrancar el menú cuando estemos en L_MainMenu (Editor y .exe).
	// Reintentar cada poco por si el mundo/PC aún no están listos.
	MenuStartAttempts = 0;
	UWorld* World = GetWorld();
	if (World)
	{
		World->GetTimerManager().SetTimer(MenuStartTimerHandle, this, &UElderbreachBrawlersGameInstance::TryStartMenuSequence, 0.35f, false);
	}
	else
	{
		// Sin mundo aún; el primer intento se hará cuando algo llame a TryStartMenuSequence
		// (podemos usar GetTimerManager del Engine)
		if (GEngine && GEngine->GetWorldContexts().Num() > 0)
		{
			World = GEngine->GetWorldContexts()[0].World();
			if (World)
			{
				World->GetTimerManager().SetTimer(MenuStartTimerHandle, this, &UElderbreachBrawlersGameInstance::TryStartMenuSequence, 0.5f, false);
			}
		}
	}
}

void UElderbreachBrawlersGameInstance::TryStartMenuSequence()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		TryAgainOrGiveUp();
		return;
	}

	const FString LevelName = UGameplayStatics::GetCurrentLevelName(World, true);
	// Mostrar men? tanto en L_MainMenu como si el .exe arranca directamente en L_Brawlers_Base
	const bool bIsFrontend = LevelName.Equals(TEXT("L_MainMenu"), ESearchCase::IgnoreCase);
	const bool bIsArenaStart = LevelName.Equals(TEXT("L_Brawlers_Base"), ESearchCase::IgnoreCase)
		|| LevelName.Contains(TEXT("Brawlers_Base"), ESearchCase::IgnoreCase);
	if (!bIsFrontend && !bIsArenaStart)
	{
		return;
	}

	APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
	if (!PC)
	{
		TryAgainOrGiveUp();
		return;
	}

	UWidgetSequenceManager* WidgetMgr = GetSubsystem<UWidgetSequenceManager>();
	if (!WidgetMgr)
	{
		TryAgainOrGiveUp();
		return;
	}
	if (WidgetMgr->IsFrontendWidgetVisible())
	{
		return; // Ya está el menú visible
	}

	// Asignar cámara para evitar pantalla negra
	if (AMainGameMode* GM = Cast<AMainGameMode>(UGameplayStatics::GetGameMode(World)))
	{
		if (GM->FightCam)
		{
			FViewTargetTransitionParams Params;
			Params.BlendTime = 0.f;
			PC->SetViewTarget(GM->FightCam, Params);
		}
	}

	WidgetMgr->StartWidgetSequence(PC);
}

void UElderbreachBrawlersGameInstance::TryAgainOrGiveUp()
{
	MenuStartAttempts++;
	if (MenuStartAttempts >= MaxMenuStartAttempts)
	{
		return;
	}
	UWorld* World = GetWorld();
	if (World)
	{
		World->GetTimerManager().SetTimer(MenuStartTimerHandle, this, &UElderbreachBrawlersGameInstance::TryStartMenuSequence, MenuStartRetryInterval, false);
	}
}
