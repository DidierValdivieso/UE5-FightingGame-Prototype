#include "PunchRecoveryNotify.h"
#include "../Characters/IO89Character.h"

void UPunchRecoveryNotify::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	if (!MeshComp) return;
	AIO89Character* Char = Cast<AIO89Character>(MeshComp->GetOwner());
	if (!Char) return;

	Char->ResetPunch();
}

