/**
 * Copyright 2022 Theai, Inc. (DBA Inworld)
 *
 * Use of this source code is governed by the Inworld.ai Software Development Kit License Agreement
 * that can be found in the LICENSE.md file or at https://www.inworld.ai/sdk-license
 */

#pragma once

#include "CoreMinimal.h"
#include "InworldCharacterPlayback.h"
#include "InworldCharacterMessage.h"

#include "InworldCharacterPlaybackText.generated.h"

UCLASS(BlueprintType, Blueprintable)
class INWORLDAIINTEGRATION_API UInworldCharacterPlaybackText : public UInworldCharacterPlayback
{
	GENERATED_BODY()

public:
	virtual void HandlePlayerTalking(const Inworld::FCharacterMessageUtterance& Message) override;

	virtual void Visit(const Inworld::FCharacterMessageUtterance& Event) override;
	virtual void Visit(const Inworld::FCharacterMessageInteractionEnd& Event) override;

public:
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInworldCharacterTextStart, const FString&, Id, bool, bIsPlayer);
	UPROPERTY(BlueprintAssignable, Category = "EventDispatchers")
	FOnInworldCharacterTextStart OnCharacterTextStart;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnInworldCharacterTextChanged, const FString&, Id, bool, bIsPlayer, const FString&, Text);
	UPROPERTY(BlueprintAssignable, Category = "EventDispatchers")
	FOnInworldCharacterTextChanged OnCharacterTextChanged;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInworldCharacterTextFinal, const FString&, Id, bool, bIsPlayer);
	UPROPERTY(BlueprintAssignable, Category = "EventDispatchers")
	FOnInworldCharacterTextFinal OnCharacterTextFinal;

private:
	void UpdateUtterance(const Inworld::FCharacterMessageUtterance& Message, bool bIsPlayer);

	TMap<FString, TArray<FString>> InteractionIdToUtteranceIdMap;

	struct FInworldCharacterText
	{
		FInworldCharacterText() = default;
		FInworldCharacterText(const Inworld::FCharacterMessageUtterance& InMessage, bool InPlayer);

		FString Id;
		FString Text;
		bool bTextFinal = false;
		bool bIsPlayer = false;
	};

	TArray<FInworldCharacterText> CharacterTexts;
};
