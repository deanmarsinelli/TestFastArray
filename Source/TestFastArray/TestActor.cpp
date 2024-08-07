#include "TestActor.h"
#include "Net/UnrealNetwork.h"

ATestActor::ATestActor()
{
	bReplicates = true;
}

void ATestActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ThisClass, TestFastArray);
}

void ATestActor::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		// add some starting data to the array on the server before the first replication
		constexpr int32 Size = 10;
		TestFastArray.Entries.SetNum(Size);

		for (int32 Idx = 0; Idx < Size; ++Idx)
		{
			FTestFastArrayItem& Item = TestFastArray.Entries[Idx];
			Item.Num = Idx;
			TestFastArray.MarkItemDirty(Item);
		}
	}
}
