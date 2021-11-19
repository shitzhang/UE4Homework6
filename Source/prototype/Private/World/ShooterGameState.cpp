// Fill out your copyright notice in the Description page of Project Settings.


#include "World/ShooterGameState.h"
#include "ShooterPlayerController.h"
#include "World/ShooterGameInstance.h"
#include "Net/UnrealNetwork.h"



AShooterGameState::AShooterGameState()
{
	/* 1 SECOND real time is 1*TimeScale MINUTES game time */
	TimeScale = 10.0f;
	bIsNight = false;

	SunriseTimeMark = 6.0f;
	SunsetTimeMark = 18.0f;
}


void AShooterGameState::SetTimeOfDay(float NewHourOfDay)
{
	ElapsedGameMinutes = NewHourOfDay * 60;
}


bool AShooterGameState::GetIsNight()
{
	return bIsNight;
}


float AShooterGameState::GetTimeOfDayIncrement()
{
	return (GetWorldSettings()->GetEffectiveTimeDilation() * TimeScale);
}


int32 AShooterGameState::GetElapsedDays()
{
	const float MinutesInDay = 24 * 60;
	const float ElapsedDays = ElapsedGameMinutes / MinutesInDay;
	return FMath::FloorToInt(ElapsedDays);
}


int32 AShooterGameState::GetElapsedFullDaysInMinutes()
{
	const int32 MinutesInDay = 24 * 60;
	return GetElapsedDays() * MinutesInDay;
}


bool AShooterGameState::GetAndUpdateIsNight()
{
	const float TimeOfDay = ElapsedGameMinutes - GetElapsedFullDaysInMinutes();
	if (TimeOfDay > (SunriseTimeMark * 60) && TimeOfDay < (SunsetTimeMark * 60))
	{
		bIsNight = false;
	}
	else
	{
		bIsNight = true;
	}

	return bIsNight;
}


int32 AShooterGameState::GetRealSecondsTillSunrise()
{
	float SunRiseMinutes = (SunriseTimeMark * 60);
	const int32 MinutesInDay = 24 * 60;

	float ElapsedTimeToday = GetElapsedMinutesCurrentDay();
	if (ElapsedTimeToday < SunRiseMinutes)
	{
		/* Still early in day cycle, so easy to get remaining time */
		return (SunRiseMinutes - ElapsedTimeToday) / TimeScale;
	}
	else
	{
		/* Sunrise will happen "tomorrow" so we need to add another full day to get remaining time */
		float MaxTimeTillNextSunrise = MinutesInDay + SunRiseMinutes;
		return (MaxTimeTillNextSunrise - ElapsedTimeToday) / TimeScale;
	}
}


int32 AShooterGameState::GetElapsedMinutesCurrentDay()
{
	return ElapsedGameMinutes - GetElapsedFullDaysInMinutes();
}


/* As with Server side functions, NetMulticast functions have a _Implementation body */
void AShooterGameState::BroadcastGameMessage_Implementation(EHUDMessage MessageID)
{
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; It++)
	{
		AShooterPlayerController* MyController = Cast<AShooterPlayerController>(*It);
		if (MyController && MyController->IsLocalController())
		{
			MyController->ClientHUDMessage(MessageID);
		}
	}
}


void AShooterGameState::AddPlayerState(APlayerState* PlayerState)
{
	Super::AddPlayerState(PlayerState);

	UShooterGameInstance* GI = GetWorld()->GetGameInstance<UShooterGameInstance>();
	if (ensure(GI))
	{
		GI->OnPlayerStateAdded.Broadcast(PlayerState);
	}
}


void AShooterGameState::RemovePlayerState(APlayerState* PlayerState)
{
	Super::RemovePlayerState(PlayerState);

	UShooterGameInstance* GI = GetWorld()->GetGameInstance<UShooterGameInstance>();
	if (ensure(GI))
	{
		GI->OnPlayerStateRemoved.Broadcast(PlayerState);
	}
}


int32 AShooterGameState::GetTotalScore()
{
	return TotalScore;
}


void AShooterGameState::AddScore(int32 Score)
{
	TotalScore += Score;
}


void AShooterGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AShooterGameState, ElapsedGameMinutes);
	DOREPLIFETIME(AShooterGameState, bIsNight);
	DOREPLIFETIME(AShooterGameState, TotalScore);
}