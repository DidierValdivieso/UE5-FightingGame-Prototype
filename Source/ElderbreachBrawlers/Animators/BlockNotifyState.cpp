#include "BlockNotifyState.h"
#include "../Characters/IO89Character.h"
#include "Components/SkeletalMeshComponent.h"

void UBlockNotifyState::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration)
{
	if (AIO89Character* Char = Cast<AIO89Character>(MeshComp->GetOwner()))
	{
		if (!Char->bCanBlock)
		{
			return;
		}

		FVector SocketLoc = MeshComp->GetSocketLocation(FName("hand_l"));
		FRotator SocketRot = MeshComp->GetSocketRotation(FName("hand_l"));
		Char->ServerEnableBlockHitbox(SocketLoc, SocketRot, TotalDuration);
	}
}

void UBlockNotifyState::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	if (AIO89Character* Char = Cast<AIO89Character>(MeshComp->GetOwner()))
	{
		Char->ServerDisableBlockHitbox();
	}
}
