// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Scorable.generated.h"

USTRUCT(BlueprintType)
struct TANKOGEDDON_API FScoredKillData
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
		int32 ScoreValue;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
		APawn* Killer;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
		APawn* Killed;
};


// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UScorable : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class TANKOGEDDON_API IScorable
{
	GENERATED_BODY()


protected:
	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnScoredKill, FScoredKillData);
	FOnScoredKill OnScoredKill;
	virtual int32 GetScore() const = 0;
	virtual void ResetScoreOnDeath() = 0;

};
