#include "ControllerSelectWidget.h"
#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "../Subsystems/WidgetSequenceManager.h"
#include "../Structs/CombatTestStructs.h"

void UControllerSelectWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // Opciones: CPU (muestra "CPU", internamente AggressiveAI) y Player. Sin DefensiveAI.
    if (ControllerType1)
    {
        ControllerType1->ClearOptions();
        ControllerType1->AddOption(TEXT("CPU"));
        ControllerType1->AddOption(TEXT("Player"));
        ControllerType1->SetSelectedIndex(0); // CPU vs ...
        ControllerType1->OnSelectionChanged.AddDynamic(this, &UControllerSelectWidget::OnControllerType1SelectionChanged);
    }

    if (ControllerType2)
    {
        ControllerType2->ClearOptions();
        ControllerType2->AddOption(TEXT("CPU"));
        ControllerType2->AddOption(TEXT("Player"));
        ControllerType2->SetSelectedIndex(1); // ... vs Player (CPU vs Player por defecto)
        ControllerType2->OnSelectionChanged.AddDynamic(this, &UControllerSelectWidget::OnControllerType2SelectionChanged);
    }

    if (StartButton)
    {
        StartButton->OnClicked.AddDynamic(this, &UControllerSelectWidget::OnStartTestClicked);
    }

    if (BackButton)
    {
        BackButton->OnClicked.AddDynamic(this, &UControllerSelectWidget::OnBackButtonClicked);
    }
}

void UControllerSelectWidget::OnStartTestClicked()
{
    if (UWorld* World = GetWorld())
    {
        if (UGameInstance* GameInstance = World->GetGameInstance())
        {
            if (UWidgetSequenceManager* Manager = GameInstance->GetSubsystem<UWidgetSequenceManager>())
            {
                FCombatTestConfig CurrentConfig = Manager->GetCurrentConfig();
                FName SelectedMap = CurrentConfig.InitialMap;

                if (SelectedMap.IsNone())
                {
                    UE_LOG(LogTemp, Warning, TEXT("[ControllerSelectWidget] No map selected in manager"));
                    return;
                }

                FCombatTestConfig Config;
                Config.bDebugMode = true;
                Config.bUseSpectator = true;
                Config.MaxTestDuration = 60.f;
                Config.InitialMap = SelectedMap;

                FParticipantConfig P1;
                if (ControllerType1)
                {
                    FString Type1 = ControllerType1->GetSelectedOption();
                    P1.ControllerType = (Type1 == TEXT("Player")) ? ECombatControllerType::Player : ECombatControllerType::AggressiveAI; // "CPU" -> AggressiveAI
                }

                P1.PawnClass = LoadClass<APawn>(nullptr, TEXT("/Game/Blueprints/Characters/BP_IO89Character.BP_IO89Character_C"));

                if (UMaterialInterface* PlayerBodyMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materials/MI_CH_IO89_Body_001.MI_CH_IO89_Body_001")))
                {
                    P1.BodyMaterialOverride = PlayerBodyMat;
                    P1.BodyMaterialSlotIndex = 0;
                }

                FParticipantConfig P2;
                if (ControllerType2)
                {
                    FString Type2 = ControllerType2->GetSelectedOption();
                    P2.ControllerType = (Type2 == TEXT("Player")) ? ECombatControllerType::Player : ECombatControllerType::AggressiveAI; // "CPU" -> AggressiveAI
                }

                P2.PawnClass = P1.PawnClass;

                if (P2.ControllerType != ECombatControllerType::Player)
                {
                    if (UMaterialInterface* EnemyBodyMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materials/MI_CH_IO89_Body_002.MI_CH_IO89_Body_002")))
                    {
                        P2.BodyMaterialOverride = EnemyBodyMat;
                        P2.BodyMaterialSlotIndex = 0;
                    }
                }

                FVector P1Location, P2Location;
                FRotator P1Rotation, P2Rotation;
                GetSpawnPositionsForMap(SelectedMap, P1Location, P1Rotation, P2Location, P2Rotation);

                P1.SpawnLocation = P1Location;
                P1.SpawnRotation = P1Rotation;
                P2.SpawnLocation = P2Location;
                P2.SpawnRotation = P2Rotation;

                Config.Participants = { P1, P2 };

                RemoveFromParent();
                Manager->OnCombatConfigComplete(Config);
            }
        }
    }
}

void UControllerSelectWidget::OnBackButtonClicked()
{
    NavigateBackToStageSelection();
}

void UControllerSelectWidget::OnControllerType1SelectionChanged(FString SelectedOption, ESelectInfo::Type SelectionType)
{
    // No permitir Player vs Player: si P1 eligió Player y P2 ya es Player, forzar P2 a CPU
    if (ControllerType1 && ControllerType2 && SelectedOption == TEXT("Player") && ControllerType2->GetSelectedOption() == TEXT("Player"))
    {
        ControllerType2->SetSelectedIndex(0); // CPU
    }
}

void UControllerSelectWidget::OnControllerType2SelectionChanged(FString SelectedOption, ESelectInfo::Type SelectionType)
{
    // No permitir Player vs Player: si P2 eligió Player y P1 ya es Player, forzar P1 a CPU
    if (ControllerType1 && ControllerType2 && SelectedOption == TEXT("Player") && ControllerType1->GetSelectedOption() == TEXT("Player"))
    {
        ControllerType1->SetSelectedIndex(0); // CPU
    }
}

void UControllerSelectWidget::GetSpawnPositionsForMap(FName MapName, FVector& OutP1Location, FRotator& OutP1Rotation,
    FVector& OutP2Location, FRotator& OutP2Rotation)
{
    FString MapString = MapName.ToString();

    if (MapString == "L_PaddockRight")
    {
        OutP1Location = FVector(-300.f, -6800.f, 5230.f);
        OutP1Rotation = FRotator(0.f, 90.f, 0.f);
        OutP2Location = FVector(-300.f, -6500.f, 5230.f);
        OutP2Rotation = FRotator(0.f, -90.f, 0.f);
    }
    else if (MapString == "L_Arena")
    {
        OutP1Location = FVector(-300.f, -4500.f, 5230.f);
        OutP1Rotation = FRotator(0.f, 90.f, 0.f);
        OutP2Location = FVector(-300.f, -4200.f, 5230.f);
        OutP2Rotation = FRotator(0.f, -90.f, 0.f);
    }
    else if (MapString == "L_PaddockLeft")
    {
        OutP1Location = FVector(-300.f, -6800.f, 4130.f);
        OutP1Rotation = FRotator(0.f, 90.f, 0.f);
        OutP2Location = FVector(-300.f, -6500.f, 4130.f);
        OutP2Rotation = FRotator(0.f, -90.f, 0.f);
    }
    else if (MapString == "L_VendorCorridor_Left")
    {
        OutP1Location = FVector(-300.f, -4500.f, 4130.f);
        OutP1Rotation = FRotator(0.f, 90.f, 0.f);
        OutP2Location = FVector(-300.f, -4200.f, 4130.f);
        OutP2Rotation = FRotator(0.f, -90.f, 0.f);
    }
    else if (MapString == "L_Maintenance")
    {
        OutP1Location = FVector(-300.f, -6800.f, 3030.f);
        OutP1Rotation = FRotator(0.f, 90.f, 0.f);
        OutP2Location = FVector(-300.f, -6500.f, 3030.f);
        OutP2Rotation = FRotator(0.f, -90.f, 0.f);
    }
    else if (MapString == "L_ConcourseLeft")
    {
        OutP1Location = FVector(-300.f, -4500.f, 3030.f);
        OutP1Rotation = FRotator(0.f, 90.f, 0.f);
        OutP2Location = FVector(-300.f, -4200.f, 3030.f);
        OutP2Rotation = FRotator(0.f, -90.f, 0.f);
    }
    else if (MapString == "L_ElderBreach")
    {
        // Misma posicion que antes; Z corregido a suelo (-10)
        OutP1Location = FVector(-1800.f, -100.f, -10.f);
        OutP1Rotation = FRotator(0.f, 90.f, 0.f);
        OutP2Location = FVector(-1800.f, 100.f, -10.f);
        OutP2Rotation = FRotator(0.f, -90.f, 0.f);
    }
    else if (MapString == "L_GarbageEjection")
    {
        // Misma posicion que antes; Z corregido a suelo (-3410)
        OutP1Location = FVector(-60.f, 5650.f, -3410.f);
        OutP1Rotation = FRotator(0.f, 90.f, 0.f);
        OutP2Location = FVector(-60.f, 5850.f, -3410.f);
        OutP2Rotation = FRotator(0.f, -90.f, 0.f);
    }
    else
    {
        // Default positions
        OutP1Location = FVector(-300.f, -4500.f, 5230.f);
        OutP1Rotation = FRotator(0.f, 90.f, 0.f);
        OutP2Location = FVector(-300.f, -4200.f, 5230.f);
        OutP2Rotation = FRotator(0.f, -90.f, 0.f);
    }
}

void UControllerSelectWidget::NavigateBackToStageSelection()
{
    if (UWorld* World = GetWorld())
    {
        if (UGameInstance* GameInstance = World->GetGameInstance())
        {
            if (UWidgetSequenceManager* Manager = GameInstance->GetSubsystem<UWidgetSequenceManager>())
            {
                Manager->ShowStageSelection();
                RemoveFromParent();
            }
        }
    }
}

