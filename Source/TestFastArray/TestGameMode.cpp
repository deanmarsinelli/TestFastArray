#include "TestGameMode.h"
#include "Engine/World.h"
#include "TestActor.h"
#include "TimerManager.h"

void ATestGameMode::BeginPlay()
{
	FTimerHandle Handle;
	GetWorld()->GetTimerManager().SetTimer(Handle, this, &ATestGameMode::SpawnChild, 2.f);
}

void ATestGameMode::SpawnChild()
{
	UClass* Class = ChildClass.LoadSynchronous();
	if (ensureAlways(Class))
	{
		FTransform Transform;
		Transform.SetLocation(FVector(400.f, 400.f, 0.f));
		GetWorld()->SpawnActor(Class, &Transform);

		FTimerHandle Handle;
		GetWorld()->GetTimerManager().SetTimer(Handle, this, &ATestGameMode::SpawnGrandChild, 2.f);
	}
}

void ATestGameMode::SpawnGrandChild()
{
	UClass* Class = GrandChildClass.LoadSynchronous();
	if (ensureAlways(Class))
	{
		FTransform Transform;
		Transform.SetLocation(FVector(-400.f, -400.f, 0.f));
		GetWorld()->SpawnActor(Class, &Transform);
	}
}
