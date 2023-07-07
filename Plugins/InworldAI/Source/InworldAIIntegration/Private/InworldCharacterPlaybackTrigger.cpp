/**
 * Copyright 2022 Theai, Inc. (DBA Inworld)
 *
 * Use of this source code is governed by the Inworld.ai Software Development Kit License Agreement
 * that can be found in the LICENSE.md file or at https://www.inworld.ai/sdk-license
 */

#include "InworldCharacterPlaybackTrigger.h"
#include "InworldApi.h"

void UInworldCharacterPlaybackTrigger::Visit(const Inworld::FCharacterMessageTrigger& Event)
{
	TriggerMessage = Event;
}

void UInworldCharacterPlaybackTrigger::Visit(const Inworld::FCharacterMessageInteractionEnd& Event)
{
	if (TriggerMessage.InteractionId == Event.InteractionId)
	{
		OwnerActor->GetWorld()->GetSubsystem<UInworldApiSubsystem>()->NotifyCustomTrigger(TriggerMessage.Name);
		TriggerMessage.InteractionId.Empty();
	}
}
