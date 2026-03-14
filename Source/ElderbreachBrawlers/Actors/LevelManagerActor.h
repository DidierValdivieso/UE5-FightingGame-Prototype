#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LevelManagerActor.generated.h"

/** Se ejecuta cuando el escenario en streaming ha terminado de cargar (suelo/geometría listos). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStageLoadedSignature, FName, LoadedStageName);

UCLASS()
class ELDERBREACHBRAWLERS_API ALevelManagerActor : public AActor
{
	GENERATED_BODY()
	
public:	
	ALevelManagerActor();

	/** Delegado que se dispara cuando el escenario está listo (para streaming). No se usa si se carga mapa completo con OpenLevel. */
	UPROPERTY(BlueprintAssignable, Category = "Level")
	FOnStageLoadedSignature OnStageLoaded;

protected:
	UFUNCTION()
	void OnSubLevelLoaded();

	/** Timer que comprueba si los subniveles en streaming ya están cargados. */
	void CheckStreamingLevelsLoaded();
	/** Carga los niveles de streaming del escenario (usa CurrentStageBaseName; llamado tras breve delay). */
	void DoLoadStageStreaming();

public:	
	UFUNCTION(BlueprintCallable)
	void LoadStage(FName StageBaseName);

private:
	FTimerHandle UnloadDelayTimerHandle;
	int32 LevelsToLoad;
	int32 LevelsLoaded;

	/** Nombres de niveles que estamos cargando por streaming (para comprobar cuándo están listos). */
	TArray<FName> PendingStreamingLevelNames;
	FTimerHandle StreamingCheckTimerHandle;
	/** Nombre base del escenario en carga (para el broadcast de OnStageLoaded). */
	FName CurrentStageBaseName;
	/** Tiempo de inicio de LoadStage (solo para logs [PERF] de revisión). */
	double StageLoadStartTime;
};
