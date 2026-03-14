#include "PunchCollisionNotifyState.h"
#include "../Characters/IO89Character.h"
#include "../Actors/ProximityAttackActor.h"

void UPunchCollisionNotifyState::NotifyBegin(
    USkeletalMeshComponent* MeshComp,
    UAnimSequenceBase* Animation,
    float TotalDuration)
{
    if (!MeshComp) return;
    AIO89Character* Char = Cast<AIO89Character>(MeshComp->GetOwner());
    if (!Char) return;

    UE_LOG(LogTemp, Warning, TEXT("[%s] PunchCollisionNotifyState::NotifyBegin (request spawn proximity)"), *Char->GetName());
    Char->ServerSpawnProximity();
}

void UPunchCollisionNotifyState::NotifyEnd(
    USkeletalMeshComponent* MeshComp,
    UAnimSequenceBase* Animation)
{
    if (!MeshComp) return;
    if (AIO89Character* Char = Cast<AIO89Character>(MeshComp->GetOwner()))
    {
        Char->ServerDestroyProximity();
        UE_LOG(LogTemp, Warning, TEXT("[%s] PunchCollisionNotifyState::NotifyEnd (stop anticipation)"), *Char->GetName());
    }
}