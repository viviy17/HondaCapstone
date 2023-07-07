// Copyright Epic Games, Inc. All Rights Reserved.

#include "MetahumanExampleGameMode.h"
#include "MetahumanExampleCharacter.h"
#include "UObject/ConstructorHelpers.h"

AMetahumanExampleGameMode::AMetahumanExampleGameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPerson/Blueprints/BP_FirstPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

}
