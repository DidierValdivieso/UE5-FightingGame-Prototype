#include "LevelManagerActor.h"
#include "Engine/LevelStreaming.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "HAL/PlatformTime.h"
#include "GenericPlatform/GenericPlatformProcess.h"

/** Log de punto [PERF] con RAM del proceso (MB) para revision de rendimiento. UE5.6: TryGetMemoryUsage(FProcHandle, Stats). */
static void LogPerfMemory(const TCHAR* Prefix)
{
	FProcHandle ProcHandle = FPlatformProcess::OpenProcess(FPlatformProcess::GetCurrentProcessId());
	if (ProcHandle.IsValid())
	{
		FPlatformProcessMemoryStats Mem = {};
		if (FPlatformProcess::TryGetMemoryUsage(ProcHandle, Mem))
		{
			const double UsedMB = static_cast<double>(Mem.UsedPhysical) / (1024.0 * 1024.0);
			const double PeakMB = static_cast<double>(Mem.PeakUsedPhysical) / (1024.0 * 1024.0);
			UE_LOG(LogTemp, Log, TEXT("[PERF] %s | RAM_proceso=%.1f MB | RAM_peak=%.1f MB"), Prefix, UsedMB, PeakMB);
		}
		FPlatformProcess::CloseProc(ProcHandle);
	}
}

ALevelManagerActor::ALevelManagerActor()
{
    PrimaryActorTick.bCanEverTick = false;
}

/** Busca un ULevelStreaming por nombre (exacto o sufijo; en PIE los nombres llevan prefijo UEDPIE_0_). */
static ULevelStreaming* FindStreamingLevelByName(UWorld* World, FName LevelPackageName)
{
    if (!World || LevelPackageName.IsNone()) return nullptr;
    const auto& Levels = World->GetStreamingLevels();
    const FString SearchStr = LevelPackageName.ToString();
    for (ULevelStreaming* LevelStreaming : Levels)
    {
        if (LevelStreaming)
        {
            FString PackageName = LevelStreaming->GetWorldAssetPackageName();
            FName PackageFName = LevelStreaming->GetWorldAssetPackageFName();
            if (PackageFName == LevelPackageName)
                return LevelStreaming;
            if (PackageName.EndsWith(SearchStr, ESearchCase::IgnoreCase))
                return LevelStreaming;
        }
    }
    return nullptr;
}

void ALevelManagerActor::LoadStage(FName BaseName)
{
    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogTemp, Error, TEXT("[LevelManager] LoadStage: World is null."));
        return;
    }

    StageLoadStartTime = FPlatformTime::Seconds();
    UE_LOG(LogTemp, Warning, TEXT("[LevelManager] LoadStage INICIO -> escenario: %s"), *BaseName.ToString());
    UE_LOG(LogTemp, Log, TEXT("[PERF] [LevelManager] Punto revision 1: LoadStage INICIO | escenario=%s | time=%.3f"), *BaseName.ToString(), StageLoadStartTime);
    LogPerfMemory(TEXT("[LevelManager] Punto 1 memoria:"));

    World->GetTimerManager().ClearTimer(StreamingCheckTimerHandle);
    World->GetTimerManager().ClearTimer(UnloadDelayTimerHandle);
    PendingStreamingLevelNames.Empty();

    const FString PackagePath = "/Game/Maps/" + BaseName.ToString();
    if (FPackageName::DoesPackageExist(PackagePath))
    {
        UE_LOG(LogTemp, Warning, TEXT("[LevelManager] Mapa completo encontrado -> OpenLevel(%s) (no se espera callback)."), *BaseName.ToString());
        UE_LOG(LogTemp, Log, TEXT("[PERF] [LevelManager] Punto revision: OpenLevel (mapa completo) | escenario=%s | sin streaming."), *BaseName.ToString());
        UGameplayStatics::OpenLevel(World, BaseName);
        return;
    }

    const FString BaseStr = BaseName.ToString();
    const TArray<FString> StageSuffixes = { TEXT("_LevelDesign"), TEXT("_Art"), TEXT("_Light") };
    const auto& AllStreaming = World->GetStreamingLevels();

    // Descarar otros escenarios para no tener varios a la vez (evita mapa "daniado" / superpuesto)
    for (ULevelStreaming* LS : AllStreaming)
    {
        if (!LS) continue;
        FString PackageName = LS->GetWorldAssetPackageName();
        int32 LastSlash = -1;
        if (PackageName.FindLastChar(TEXT('/'), LastSlash))
            PackageName = PackageName.Mid(LastSlash + 1);
        FString StageBase;
        for (const FString& Suffix : StageSuffixes)
        {
            if (PackageName.EndsWith(Suffix, ESearchCase::IgnoreCase))
            {
                StageBase = PackageName.LeftChop(Suffix.Len());
                break;
            }
        }
        if (StageBase.IsEmpty()) continue;
        // No descargar niveles del escenario que vamos a cargar (en PIE el nombre lleva prefijo UEDPIE_0_)
        if (StageBase.Equals(BaseStr, ESearchCase::IgnoreCase) || StageBase.EndsWith(BaseStr, ESearchCase::IgnoreCase))
            continue;

        // Desactivar el resto (incl. ElderBreach) para que solo quede activo el elegido (ej. solo Garbage Art+Light; spawn en suelo)
        UE_LOG(LogTemp, Warning, TEXT("[LevelManager] Desactivando otro escenario: %s"), *LS->GetWorldAssetPackageName());
        LS->SetShouldBeVisible(false);
        LS->SetShouldBeLoaded(false);
    }

    // Breve delay para que el motor procese la descarga antes de cargar el nuevo escenario (evita ver dos mapas a la vez)
    CurrentStageBaseName = BaseName;
    World->GetTimerManager().SetTimer(UnloadDelayTimerHandle, this, &ALevelManagerActor::DoLoadStageStreaming, 0.2f, false);
}

void ALevelManagerActor::DoLoadStageStreaming()
{
    UnloadDelayTimerHandle.Invalidate();
    UWorld* World = GetWorld();
    if (!World) return;

    UE_LOG(LogTemp, Log, TEXT("[PERF] [LevelManager] Punto revision 2: DoLoadStageStreaming INICIO | escenario=%s | time=%.3f"), *CurrentStageBaseName.ToString(), FPlatformTime::Seconds());
    LogPerfMemory(TEXT("[LevelManager] Punto 2 memoria:"));

    const FString BaseStr = CurrentStageBaseName.ToString();
    const TArray<FString> StageSuffixes = { TEXT("_LevelDesign"), TEXT("_Art"), TEXT("_Light") };
    const auto& AllStreaming = World->GetStreamingLevels();
    PendingStreamingLevelNames.Empty();

    // Escenario elegido (ej. Garbage_Art + Garbage_Light)
    for (ULevelStreaming* LS : AllStreaming)
    {
        if (!LS) continue;
        FString PackageName = LS->GetWorldAssetPackageName();
        if (!PackageName.Contains(BaseStr, ESearchCase::IgnoreCase))
            continue;
        bool bIsStagePart = false;
        for (const FString& Suffix : StageSuffixes)
        {
            if (PackageName.EndsWith(Suffix, ESearchCase::IgnoreCase))
            {
                bIsStagePart = true;
                break;
            }
        }
        if (!bIsStagePart) continue;

        FName ExactName = LS->GetWorldAssetPackageFName();
        UE_LOG(LogTemp, Warning, TEXT("[LevelManager] Encontrado nivel del escenario (nombre real en mundo): %s -> cargando."), *ExactName.ToString());
        UGameplayStatics::LoadStreamLevel(World, ExactName, true, true, FLatentActionInfo());
        LS->SetShouldBeVisible(true);
        LS->SetShouldBeLoaded(true);
        if (!PendingStreamingLevelNames.Contains(ExactName))
            PendingStreamingLevelNames.Add(ExactName);
    }

    if (PendingStreamingLevelNames.Num() > 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("[LevelManager] Esperando %d nivel(es) en streaming. Timer cada 0.1s hasta que IsLevelLoaded()=true en todos."), PendingStreamingLevelNames.Num());
        World->GetTimerManager().SetTimer(StreamingCheckTimerHandle, this, &ALevelManagerActor::CheckStreamingLevelsLoaded, 0.1f, true);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[LevelManager] No hay niveles de este escenario en el mundo (o no tienen sufijo _LevelDesign/_Art/_Light) -> Broadcast OnStageLoaded(%s) ahora."), *CurrentStageBaseName.ToString());
        OnStageLoaded.Broadcast(CurrentStageBaseName);
    }
}

void ALevelManagerActor::CheckStreamingLevelsLoaded()
{
    UWorld* World = GetWorld();
    if (!World || PendingStreamingLevelNames.Num() == 0) return;

    for (const FName& LevelName : PendingStreamingLevelNames)
    {
        ULevelStreaming* StreamingLevel = FindStreamingLevelByName(World, LevelName);
        if (!StreamingLevel)
        {
            UE_LOG(LogTemp, Verbose, TEXT("[LevelManager] CheckStreaming: %s no encontrado, sigo esperando."), *LevelName.ToString());
            return;
        }
        if (!StreamingLevel->IsLevelLoaded())
        {
            UE_LOG(LogTemp, Verbose, TEXT("[LevelManager] CheckStreaming: %s aun no IsLevelLoaded(), sigo esperando."), *LevelName.ToString());
            return;
        }
    }

    World->GetTimerManager().ClearTimer(StreamingCheckTimerHandle);
    FName LoadedStage = CurrentStageBaseName;
    PendingStreamingLevelNames.Empty();
    const double LoadDurationSec = FPlatformTime::Seconds() - StageLoadStartTime;
    UE_LOG(LogTemp, Warning, TEXT("[LevelManager] *** TODOS los niveles en streaming cargados -> Broadcast OnStageLoaded(%s) ***"), *LoadedStage.ToString());
    UE_LOG(LogTemp, Log, TEXT("[PERF] [LevelManager] Punto revision 3: Streaming COMPLETO | escenario=%s | duracion_carga=%.3fs | time=%.3f"), *LoadedStage.ToString(), LoadDurationSec, FPlatformTime::Seconds());
    LogPerfMemory(TEXT("[LevelManager] Punto 3 memoria (post-carga):"));
    OnStageLoaded.Broadcast(LoadedStage);
}

void ALevelManagerActor::OnSubLevelLoaded()
{
    LevelsLoaded++;
    if (LevelsLoaded >= LevelsToLoad)
    {
    }
}
