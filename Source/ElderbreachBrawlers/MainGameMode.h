#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MainGameMode.generated.h"

class AIO89Character;
class AFightingCameraActor;
class ALevelManagerActor;

UCLASS()
class ELDERBREACHBRAWLERS_API AMainGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
    AMainGameMode();

protected:
    virtual void BeginPlay() override;
    virtual void PostLogin(APlayerController* NewPlayer) override;

public:
    UPROPERTY(EditAnywhere, Category = "Player")
    TSubclassOf<AIO89Character> PlayerBPClass;

    UPROPERTY()
    TArray<AIO89Character*> SpawnedPlayers;

    UPROPERTY(EditDefaultsOnly, Category = "Camera")
    TSubclassOf<AFightingCameraActor> FightingCameraClass;

    UPROPERTY()
    AFightingCameraActor* FightCam;

    UPROPERTY()
    ALevelManagerActor* LevelManager;

    /** Mapa donde se muestra el menú (Frontend). Solo en este mapa se inicia la secuencia de widgets. */
    UPROPERTY(EditDefaultsOnly, Category = "Maps", meta = (DisplayName = "Frontend Map Name"))
    FName FrontendMapName;

    /** Mapa de arena/hub al que se viaja al confirmar la pelea desde el menú (tiene LevelManager y niveles streamed). */
    UPROPERTY(EditDefaultsOnly, Category = "Maps", meta = (DisplayName = "Arena Hub Map Name"))
    FName ArenaHubMapName;

    /** True si el mapa actual es el de menú (frontend). */
    bool IsFrontendMap() const;

private:
    /** Inicia la secuencia de widgets del menú con un pequeño retraso (para que FightCam y PC estén listos). */
    void StartMenuWidgetsWithDelay();

    int32 ConnectedPlayers = 0;
};
