/**
 * Copyright 2022 Theai, Inc. (DBA Inworld)
 *
 * Use of this source code is governed by the Inworld.ai Software Development Kit License Agreement
 * that can be found in the LICENSE.md file or at https://www.inworld.ai/sdk-license
 */

#include "InworldCharacterPlaybackText.h"
#include "InworldCharacterMessage.h"

UInworldCharacterPlaybackText::FInworldCharacterText::FInworldCharacterText(const Inworld::FCharacterMessageUtterance& InMessage, bool InPlayerInteraction)
	: Id(InMessage.UtteranceId)
	, Text(InMessage.Text)
	, bTextFinal(InMessage.bTextFinal)
	, bIsPlayer(InPlayerInteraction)
{}

void UInworldCharacterPlaybackText::HandlePlayerTalking(const Inworld::FCharacterMessageUtterance& Message)
{
	UpdateUtterance(Message, true);
}

void UInworldCharacterPlaybackText::Visit(const Inworld::FCharacterMessageUtterance& Message)
{
	UpdateUtterance(Message, false);
}

void UInworldCharacterPlaybackText::Visit(const Inworld::FCharacterMessageInteractionEnd& Message)
{
	const FString InteractionId = Message.InteractionId;
	if (InteractionIdToUtteranceIdMap.Contains(InteractionId))
	{
		const TArray<FString>& InteractionUtteranceIds = InteractionIdToUtteranceIdMap[InteractionId];
		CharacterTexts.RemoveAll([InteractionUtteranceIds](const auto& Text) { return InteractionUtteranceIds.Contains(Text.Id); });
		InteractionIdToUtteranceIdMap.Remove(InteractionId);
	}
}

void UInworldCharacterPlaybackText::UpdateUtterance(const Inworld::FCharacterMessageUtterance& Message, bool bIsPlayer)
{
	const FString InteractionId = Message.InteractionId;
	if (!InteractionIdToUtteranceIdMap.Contains(InteractionId))
	{
		InteractionIdToUtteranceIdMap.Add(InteractionId, TArray<FString>());
	}

	const FString UtteranceId = Message.UtteranceId;
	auto* Text = CharacterTexts.FindByPredicate([UtteranceId](const auto& Text) { return Text.Id == UtteranceId; });
	if (Text)
	{
		*Text = FInworldCharacterText(Message, bIsPlayer);
	}
	else
	{
		Text = &CharacterTexts.Emplace_GetRef(Message, bIsPlayer);
		InteractionIdToUtteranceIdMap[InteractionId].Add(Text->Id);
		OnCharacterTextStart.Broadcast(Text->Id, Text->bIsPlayer);
	}

	OnCharacterTextChanged.Broadcast(Text->Id, Text->bIsPlayer, Text->Text);
	if (Text->bTextFinal)
	{
		OnCharacterTextFinal.Broadcast(Text->Id, Text->bIsPlayer);
	}
}
