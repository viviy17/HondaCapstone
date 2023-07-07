/**
 * Copyright 2022 Theai, Inc. (DBA Inworld)
 *
 * Use of this source code is governed by the Inworld.ai Software Development Kit License Agreement
 * that can be found in the LICENSE.md file or at https://www.inworld.ai/sdk-license
 */

#pragma once

#include "CoreMinimal.h"
#include "InworldState.h"
#include "InworldCharacterComponent.h"
#include "InworldUtils.h"
#include "NDK/Client.h"

#include "InworldPlayerComponent.generated.h"

class UInworldApiSubsystem;
class UInworldCharacterComponent;

UCLASS(ClassGroup = (Inworld), meta = (BlueprintSpawnableComponent))
class INWORLDAIINTEGRATION_API UInworldPlayerComponent : public UActorComponent, public Inworld::IPlayerComponent
{
	GENERATED_BODY()

public:
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnInworldPlayerTargetChange, UInworldCharacterComponent*);
    FOnInworldPlayerTargetChange OnTargetChange;

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UFUNCTION(BlueprintCallable, Category = "Interaction", meta = (Displayname = "GetTargetCharacter"))
    UInworldCharacterComponent* GetTargetInworldCharacter() { return static_cast<UInworldCharacterComponent*>(GetTargetCharacter()); }

    virtual Inworld::ICharacterComponent* GetTargetCharacter() override;

    UFUNCTION(BlueprintCallable, Category = "Interaction", meta = (Displayname = "SetTargetCharacter"))
    void SetTargetInworldCharacter(UInworldCharacterComponent* Character);

    UFUNCTION(BlueprintCallable, Category = "Interaction", meta = (Displayname = "ClearTargetCharacter"))
    void ClearTargetInworldCharacter();

	UFUNCTION(BlueprintCallable, Category = "Interaction")
    bool IsInteracting() { return !TargetCharacterAgentId.IsEmpty(); }

    UFUNCTION(BlueprintCallable, Category = "Interaction")
    void SendTextMessageToTarget(const FString& Message);

    UFUNCTION(BlueprintCallable, Category = "Interaction")
    void SendCustomEventToTarget(const FString& Name);

    UFUNCTION(BlueprintCallable, Category = "Interaction")
    void StartAudioSessionWithTarget();

    UFUNCTION(BlueprintCallable, Category = "Interaction")
    void StopAudioSessionWithTarget();

    UFUNCTION(BlueprintCallable, Category = "Interaction")
    void SendAudioMessageToTarget(USoundWave* SoundWave);
    void SendAudioDataMessageToTarget(const std::string& Data);

private:
    UPROPERTY(EditAnywhere, Category = "UI")
    FString UiName = "Player";

    TWeakObjectPtr<UInworldApiSubsystem> InworldSubsystem;

    FString TargetCharacterAgentId;
};