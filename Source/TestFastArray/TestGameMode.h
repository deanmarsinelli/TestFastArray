#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "UObject/SoftObjectPtr.h"
#include "TestGameMode.generated.h"

class ATestActor;

UCLASS()
class ATestGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	TSoftClassPtr<ATestActor> ChildClass;

	UPROPERTY(EditAnywhere)
	TSoftClassPtr<ATestActor> GrandChildClass;

	virtual void BeginPlay() override;

	UFUNCTION()
	void SpawnChild();

	UFUNCTION()
	void SpawnGrandChild();
};
