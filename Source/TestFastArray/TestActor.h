#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "TestActor.generated.h"

USTRUCT()
struct FTestFastArrayItem : public FFastArraySerializerItem
{
	GENERATED_BODY()

	UPROPERTY()
	int32 Num = 0;
};

USTRUCT()
struct FTestFastArray : public FFastArraySerializer
{
	GENERATED_BODY()

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FTestFastArrayItem, FTestFastArray>(Entries, DeltaParms, *this);
	}

	UPROPERTY()
	TArray<FTestFastArrayItem> Entries;
};

template<>
struct TStructOpsTypeTraits<FTestFastArray> : public TStructOpsTypeTraitsBase2<FTestFastArray>
{
	enum { WithNetDeltaSerializer = true };
};



// simple class with a single FastArraySerializer property
UCLASS()
class ATestActor : public AActor
{
	GENERATED_BODY()

public:
	UPROPERTY(Replicated)
	FTestFastArray TestFastArray;

	ATestActor();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
};
