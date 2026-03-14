#include "StageSelectWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/ScaleBox.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Engine/Texture2D.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"
#include "../Subsystems/WidgetSequenceManager.h"
#include "ButtonStageWidget.h"

UStageSelectWidget::UStageSelectWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    // Referencias para que el cooker incluya las texturas en el build (.exe)
    struct FStageTex { const TCHAR* Path; };
    static const FStageTex StageTexPaths[] = {
        { TEXT("/Game/UI/Resources/Bg_StagePaddockRight.Bg_StagePaddockRight") },
        { TEXT("/Game/UI/Resources/Bg_StageArena.Bg_StageArena") },
        { TEXT("/Game/UI/Resources/Bg_StagePaddockLeft.Bg_StagePaddockLeft") },
        { TEXT("/Game/UI/Resources/Bg_StageVendorCorridor_Left.Bg_StageVendorCorridor_Left") },
        { TEXT("/Game/UI/Resources/Bg_StageMaintenance.Bg_StageMaintenance") },
        { TEXT("/Game/UI/Resources/Bg_StageConcourseLeft.Bg_StageConcourseLeft") },
        { TEXT("/Game/UI/Resources/Bg_StageElderbreach.Bg_StageElderbreach") },
        { TEXT("/Game/UI/Resources/Bg_StageGarbageEjection.Bg_StageGarbageEjection") }
    };
    CookerStageTextureRefs.Reserve(UE_ARRAY_COUNT(StageTexPaths));
    for (const FStageTex& T : StageTexPaths)
    {
        ConstructorHelpers::FObjectFinder<UTexture2D> Finder(T.Path);
        if (Finder.Succeeded())
        {
            CookerStageTextureRefs.Add(Finder.Object);
        }
    }
}

void UStageSelectWidget::NativeConstruct()
{
    Super::NativeConstruct();

    bIsContainerVisible = true;

    LoadStageSelectionMaterials();
    PreloadStageImages();
    InitializeExistingButtons();

    if (StageButtons.Num() == 0)
    {
        CreateStageButtons();
    }

    if (ConfirmButton)
    {
        ConfirmButton->OnClicked.AddDynamic(this, &UStageSelectWidget::OnConfirmButtonClicked);
    }

    if (BackButton)
    {
        BackButton->OnClicked.AddDynamic(this, &UStageSelectWidget::OnBackButtonClicked);
    }

    if (ToggleVisibilityButton)
    {
        ToggleVisibilityButton->OnClicked.AddDynamic(this, &UStageSelectWidget::OnToggleVisibilityButtonClicked);
    }
}

void UStageSelectWidget::OnConfirmButtonClicked()
{
    NavigateToCombatConfig();
}

void UStageSelectWidget::OnBackButtonClicked()
{
    NavigateBackToFrontend();
}

void UStageSelectWidget::NavigateToCombatConfig()
{
    if (SelectedStageName.IsEmpty())
    {
        return;
    }

    if (UWorld* World = GetWorld())
    {
        if (UGameInstance* GameInstance = World->GetGameInstance())
        {
            if (UWidgetSequenceManager* Manager = GameInstance->GetSubsystem<UWidgetSequenceManager>())
            {
                Manager->OnStageSelected(FName(*SelectedStageName));
                RemoveFromParent();
            }
        }
    }
}

void UStageSelectWidget::NavigateBackToFrontend()
{
    if (UWorld* World = GetWorld())
    {
        if (UGameInstance* GameInstance = World->GetGameInstance())
        {
            if (UWidgetSequenceManager* Manager = GameInstance->GetSubsystem<UWidgetSequenceManager>())
            {
                if (APlayerController* PC = GetOwningPlayer())
                {
                    Manager->StartWidgetSequence(PC);
                    RemoveFromParent();
                }
            }
        }
    }
}

void UStageSelectWidget::UpdateBackgroundImage(const FString& StageName)
{
    if (!BackgroundImage) return;

    UTexture2D* Texture = nullptr;
    if (TObjectPtr<UTexture2D>* Cached = LoadedStageTextures.Find(StageName))
    {
        Texture = *Cached;
    }
    if (!Texture)
    {
        FString TexturePath = GetTexturePathForStage(StageName);
        if (!TexturePath.IsEmpty())
        {
            Texture = LoadObject<UTexture2D>(nullptr, *TexturePath);
            if (Texture) LoadedStageTextures.Add(StageName, Texture);
        }
    }
    if (Texture)
    {
        BackgroundImage->SetBrushFromTexture(Texture, true);
        // Zoom "cover" si está dentro de un ScaleBox
        for (UPanelWidget* Parent = BackgroundImage->GetParent(); Parent; Parent = Parent->GetParent())
        {
            if (UScaleBox* ScaleBox = Cast<UScaleBox>(Parent))
            {
                ScaleBox->SetStretch(EStretch::ScaleToFill);
                break;
            }
        }
    }
}

void UStageSelectWidget::LoadStageSelectionMaterials()
{
    LoadedMaterials.Empty();
    if (StageImageMaterial.IsValid())
    {
        if (UMaterialInterface* Mat = StageImageMaterial.LoadSynchronous())
        {
            LoadedMaterials.Add(Mat);
        }
    }
    for (TSoftObjectPtr<UMaterialInterface>& MatPtr : MaterialsToLoad)
    {
        if (!MatPtr.IsNull())
        {
            if (UMaterialInterface* Mat = MatPtr.LoadSynchronous())
            {
                LoadedMaterials.Add(Mat);
            }
        }
    }
    if (UMaterialInterface* RomboMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/UI/Resources/M_RomboMasked.M_RomboMasked")))
    {
        LoadedMaterials.Add(RomboMat);
    }
}

void UStageSelectWidget::PreloadStageImages()
{
    LoadedStageTextures.Empty();
    static const TArray<FString> StageNames = {
        TEXT("L_PaddockRight"),
        TEXT("L_Arena"),
        TEXT("L_PaddockLeft"),
        TEXT("L_VendorCorridor_Left"),
        TEXT("L_Maintenance"),
        TEXT("L_ConcourseLeft"),
        TEXT("L_ElderBreach"),
        TEXT("L_GarbageEjection")
    };
    for (const FString& StageName : StageNames)
    {
        FString TexturePath = GetTexturePathForStage(StageName);
        if (TexturePath.IsEmpty()) continue;
        if (UTexture2D* Tex = LoadObject<UTexture2D>(nullptr, *TexturePath))
        {
            LoadedStageTextures.Add(StageName, Tex);
        }
    }
}

FString UStageSelectWidget::GetTexturePathForStage(const FString& StageName)
{
    static TMap<FString, FString> StageTextureMap = {
        {TEXT("L_PaddockRight"), TEXT("/Game/UI/Resources/Bg_StagePaddockRight.Bg_StagePaddockRight")},
        {TEXT("L_Arena"), TEXT("/Game/UI/Resources/Bg_StageArena.Bg_StageArena")},
        {TEXT("L_PaddockLeft"), TEXT("/Game/UI/Resources/Bg_StagePaddockLeft.Bg_StagePaddockLeft")},
        {TEXT("L_VendorCorridor_Left"), TEXT("/Game/UI/Resources/Bg_StageVendorCorridor_Left.Bg_StageVendorCorridor_Left")},
        {TEXT("L_Maintenance"), TEXT("/Game/UI/Resources/Bg_StageMaintenance.Bg_StageMaintenance")},
        {TEXT("L_ConcourseLeft"), TEXT("/Game/UI/Resources/Bg_StageConcourseLeft.Bg_StageConcourseLeft")},
        {TEXT("L_ElderBreach"), TEXT("/Game/UI/Resources/Bg_StageElderbreach.Bg_StageElderbreach")},
        {TEXT("L_GarbageEjection"), TEXT("/Game/UI/Resources/Bg_StageGarbageEjection.Bg_StageGarbageEjection")}
    };

    if (const FString* FoundPath = StageTextureMap.Find(StageName))
    {
        return *FoundPath;
    }

    return FString();
}

bool UStageSelectWidget::IsStageAvailable(const FString& StageName) const
{
    static const TMap<FString, bool> StageAvailabilityMap = {
        {TEXT("L_PaddockRight"), false},
        {TEXT("L_Arena"), false},
        {TEXT("L_PaddockLeft"), false},
        {TEXT("L_VendorCorridor_Left"), false},
        {TEXT("L_Maintenance"), false},
        {TEXT("L_ConcourseLeft"), false},
        {TEXT("L_ElderBreach"), true},
        {TEXT("L_GarbageEjection"), true}
    };
    if (const bool* bAvailable = StageAvailabilityMap.Find(StageName))
    {
        return *bAvailable;
    }
    return false;
}

void UStageSelectWidget::CreateStageButtons()
{
    if (!StageButtonsContainer)
    {
        return;
    }

    if (!ButtonStageWidgetClass)
    {
        return;
    }

    TArray<FString> StageNames = {
        TEXT("L_PaddockRight"),
        TEXT("L_Arena"),
        TEXT("L_PaddockLeft"),
        TEXT("L_VendorCorridor_Left"),
        TEXT("L_Maintenance"),
        TEXT("L_ConcourseLeft"),
        TEXT("L_ElderBreach"),
        TEXT("L_GarbageEjection")
    };

    StageButtonsContainer->ClearChildren();
    StageButtons.Empty();

    for (const FString& StageName : StageNames)
    {
        UButtonStageWidget* StageButton = CreateWidget<UButtonStageWidget>(this, ButtonStageWidgetClass);
        if (!StageButton)
        {
            continue;
        }

        UTexture2D* StageTexture = nullptr;
        if (TObjectPtr<UTexture2D>* Cached = LoadedStageTextures.Find(StageName))
        {
            StageTexture = *Cached;
        }
        if (!StageTexture)
        {
            FString TexturePath = GetTexturePathForStage(StageName);
            if (!TexturePath.IsEmpty())
            {
                StageTexture = LoadObject<UTexture2D>(nullptr, *TexturePath);
                if (StageTexture) LoadedStageTextures.Add(StageName, StageTexture);
            }
        }
        const bool bAvailable = IsStageAvailable(StageName);
        StageButton->InitializeStageButton(StageName, StageTexture, bAvailable);

        StageButton->OnStageSelected.AddDynamic(this, &UStageSelectWidget::OnStageButtonSelected);

        StageButtonsContainer->AddChild(StageButton);
        StageButtons.Add(StageButton);
    }

    SelectedStageName = TEXT("L_ElderBreach");
    UpdateBackgroundImage(SelectedStageName);
    RefreshSelectionBorder();
}

void UStageSelectWidget::RefreshSelectionBorder()
{
    for (UButtonStageWidget* StageBtn : StageButtons)
    {
        if (StageBtn)
        {
            StageBtn->SetSelected(StageBtn->GetStageName() == SelectedStageName);
        }
    }
}

void UStageSelectWidget::OnStageButtonSelected(FString InSelectedStageName)
{
    SelectedStageName = InSelectedStageName;
    UpdateBackgroundImage(InSelectedStageName);
    RefreshSelectionBorder();
}

void UStageSelectWidget::InitializeExistingButtons()
{
    StageButtons.Empty();
    const FString BtnPrefix = TEXT("btnStage_");

    if (UWidgetTree* Tree = WidgetTree)
    {
        Tree->ForEachWidget([this, &BtnPrefix](UWidget* Widget)
        {
            if (UButtonStageWidget* StageBtn = Cast<UButtonStageWidget>(Widget))
            {
                FString WidgetName = StageBtn->GetName();
                if (WidgetName.StartsWith(BtnPrefix))
                {
                    FString StageName = WidgetName.Mid(BtnPrefix.Len());
                    if (StageName.EndsWith(TEXT("_C")))
                    {
                        StageName.LeftChopInline(2);
                    }

                    UTexture2D* StageTexture = nullptr;
                    if (TObjectPtr<UTexture2D>* Cached = LoadedStageTextures.Find(StageName))
                    {
                        StageTexture = *Cached;
                    }
                    if (!StageTexture)
                    {
                        FString TexturePath = GetTexturePathForStage(StageName);
                        if (!TexturePath.IsEmpty())
                        {
                            StageTexture = LoadObject<UTexture2D>(nullptr, *TexturePath);
                            if (StageTexture) LoadedStageTextures.Add(StageName, StageTexture);
                        }
                    }

                    const bool bAvailable = IsStageAvailable(StageName);
                    StageBtn->InitializeStageButton(StageName, StageTexture, bAvailable);
                    StageBtn->OnStageSelected.AddDynamic(this, &UStageSelectWidget::OnStageButtonSelected);

                    StageButtons.Add(StageBtn);
                }
            }
        });
    }

    if (StageButtons.Num() > 0)
    {
        SelectedStageName = TEXT("L_ElderBreach");
        UpdateBackgroundImage(SelectedStageName);
        RefreshSelectionBorder();
    }
}

void UStageSelectWidget::OnToggleVisibilityButtonClicked()
{
    bIsContainerVisible = !bIsContainerVisible;
    
    ESlateVisibility NewVisibility = bIsContainerVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;

    if (StageButtonsContainer)
    {
        StageButtonsContainer->SetVisibility(NewVisibility);
    }

    if (bgContainer_image)
    {
        bgContainer_image->SetVisibility(NewVisibility);
    }
}
