/**
 * Copyright 2022 Theai, Inc. (DBA Inworld)
 *
 * Use of this source code is governed by the Inworld.ai Software Development Kit License Agreement
 * that can be found in the LICENSE.md file or at https://www.inworld.ai/sdk-license
 */

#include "InworldPlayerComponent.h"
#include "NDK/Proto/ProtoDisableWarning.h"
#include "InworldApi.h"
#include "InworldCharacterComponent.h"
void UInworldPlayerComponent::BeginPlay()
{
    Super::BeginPlay();

    InworldSubsystem = GetWorld()->GetSubsystem<UInworldApiSubsystem>();
    if (InworldSubsystem.IsValid())
    {
        InworldSubsystem->RegisterPlayerComponent(this);
    }
}

void UInworldPlayerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (InworldSubsystem.IsValid())
    {
        InworldSubsystem->UnregisterPlayerComponent();
    }

    Super::EndPlay(EndPlayReason);
}

Inworld::ICharacterComponent* UInworldPlayerComponent::GetTargetCharacter()
{
    if (InworldSubsystem.IsValid() && !TargetCharacterAgentId.IsEmpty())
    {
        return InworldSubsystem->GetCharacterComponentByAgentId(TargetCharacterAgentId);
    }

    return nullptr;
}

void UInworldPlayerComponent::SetTargetInworldCharacter(UInworldCharacterComponent* Character)
{
    if (!ensureMsgf(Character && !Character->GetAgentId().IsEmpty(), TEXT("UInworldPlayerComponent::SetTargetCharacter: the Character must have valid AgentId")))
    {
        return;
    }

    if (Character->StartPlayerInteraction(this))
    {
        TargetCharacterAgentId = Character->GetAgentId();
        OnTargetChange.Broadcast(Character);
    }
}

void UInworldPlayerComponent::ClearTargetInworldCharacter()
{
    UInworldCharacterComponent* TargetCharacter = GetTargetInworldCharacter();
    if (TargetCharacter && TargetCharacter->StopPlayerInteraction(this))
    {
		StopAudioSessionWithTarget();
		TargetCharacterAgentId = FString();

		OnTargetChange.Broadcast(nullptr);
    }
}

void UInworldPlayerComponent::SendTextMessageToTarget(const FString& Message)
{
    if (!TargetCharacterAgentId.IsEmpty())
    {
        InworldSubsystem->SendTextMessage(TargetCharacterAgentId, Message);
    }
}

void UInworldPlayerComponent::SendCustomEventToTarget(const FString& Name)
{
    if (!TargetCharacterAgentId.IsEmpty())
    {
        InworldSubsystem->SendCustomEvent(TargetCharacterAgentId, Name);
    }
}

void UInworldPlayerComponent::StartAudioSessionWithTarget()
{
    if (!TargetCharacterAgentId.IsEmpty())
    {
        InworldSubsystem->StartAudioSession(TargetCharacterAgentId);
    }
}

void UInworldPlayerComponent::StopAudioSessionWithTarget()
{
    if (!TargetCharacterAgentId.IsEmpty())
	{
		InworldSubsystem->StopAudioSession(TargetCharacterAgentId);
	}
}

void UInworldPlayerComponent::SendAudioMessageToTarget(USoundWave* SoundWave)
{
    if (!TargetCharacterAgentId.IsEmpty())
    {
        InworldSubsystem->SendAudioMessage(TargetCharacterAgentId, SoundWave);
    }
}

void UInworldPlayerComponent::SendAudioDataMessageToTarget(const std::string& Data)
{
    if (!TargetCharacterAgentId.IsEmpty())
    {
        InworldSubsystem->SendAudioDataMessage(TargetCharacterAgentId, Data);
    }
}

