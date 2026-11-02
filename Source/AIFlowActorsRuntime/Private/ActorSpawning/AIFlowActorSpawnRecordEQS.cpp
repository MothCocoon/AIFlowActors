// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#include "ActorSpawning/AIFlowActorSpawnRecordEQS.h"

#include "ActorSpawning/FlowActorSpawnRecordOwner.h"
#include "EnvironmentQuery/EnvQuery.h"
#include "EnvironmentQuery/EnvQueryManager.h"
#include "GameFramework/Actor.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(AIFlowActorSpawnRecordEQS)

#if WITH_SERVER_CODE
void UAIFlowActorSpawnRecordEQS::CleanupRuntime()
{
	CancelActiveQuery();
	bHasQuerySpawnTransform = false;

	Super::CleanupRuntime();
}

bool UAIFlowActorSpawnRecordEQS::AdvanceCurrentState(EFlowActorSpawnRecordState& OutNextState)
{
	FLOW_ASSERT_ENUM_MAX(EFlowActorSpawnRecordState, 10);
	if (GetRecordState() != EFlowActorSpawnRecordState::WaitingForSpawnLocation)
	{
		return Super::AdvanceCurrentState(OutNextState);
	}

	if (ActiveQueryId != INDEX_NONE)
	{
		return false;
	}

	const IFlowActorSpawnRecordOwner* Owner = GetRecordOwner();
	AActor* Querier = nullptr;
	if (Owner)
	{
		Querier = Owner->TryGetActorOwner();
	}
	if (!IsValid(Querier) || !IsValid(LocationQuery.QueryTemplate.Get()) || LocationQuery.bUseBBKeyForQueryTemplate)
	{
		OutNextState = EFlowActorSpawnRecordState::FailedSpawn;
		return true;
	}

	LocationQuery.InitForOwnerAndBlackboard(*this, nullptr);
	FQueryFinishedSignature Finished = FQueryFinishedSignature::CreateUObject(
		this, &UAIFlowActorSpawnRecordEQS::HandleQueryFinished);
	ActiveConfigurationGeneration = GetConfigurationGeneration();
	ActiveQueryId = LocationQuery.Execute(*Querier, nullptr, Finished);

	if (ActiveQueryId == INDEX_NONE)
	{
		OutNextState = EFlowActorSpawnRecordState::FailedSpawn;
		return true;
	}

	return false;
}

void UAIFlowActorSpawnRecordEQS::OnStateChanged(
	EFlowActorSpawnRecordState PreviousState, EFlowActorSpawnRecordState NextState)
{
	FLOW_ASSERT_ENUM_MAX(EFlowActorSpawnRecordState, 10);
	Super::OnStateChanged(PreviousState, NextState);
	if (PreviousState == EFlowActorSpawnRecordState::WaitingForSpawnLocation
		&& NextState != EFlowActorSpawnRecordState::WaitingForSpawnLocation)
	{
		CancelActiveQuery();
	}

	if (NextState == EFlowActorSpawnRecordState::CleanupForRespawn
		|| NextState == EFlowActorSpawnRecordState::Invalid
		|| NextState == EFlowActorSpawnRecordState::FailedSpawn)
	{
		bHasQuerySpawnTransform = false;
	}
}

FTransform UAIFlowActorSpawnRecordEQS::GetDesiredSpawnTransform() const
{
	if (bHasQuerySpawnTransform)
	{
		return QuerySpawnTransform;
	}

	return Super::GetDesiredSpawnTransform();
}

void UAIFlowActorSpawnRecordEQS::HandleQueryFinished(TSharedPtr<FEnvQueryResult> Result)
{
	FLOW_ASSERT_ENUM_MAX(EFlowActorSpawnRecordState, 10);
	if (ActiveQueryId == INDEX_NONE || ActiveConfigurationGeneration != GetConfigurationGeneration()
		|| GetRecordState() != EFlowActorSpawnRecordState::WaitingForSpawnLocation
		|| !Result.IsValid() || Result->QueryID != ActiveQueryId || Result->IsAborted())
	{
		return;
	}

	const bool bSucceeded = Result->IsSuccessful() && Result->Items.Num() > 0 && Result->Items[0].IsValid();
	if (bSucceeded)
	{
		QuerySpawnTransform = Super::GetDesiredSpawnTransform();
		QuerySpawnTransform.SetLocation(Result->GetItemAsLocation(0));
	}

	bHasQuerySpawnTransform = bSucceeded;
	ActiveQueryId = INDEX_NONE;
	CompleteSpawnLocation(bSucceeded);
}

void UAIFlowActorSpawnRecordEQS::CancelActiveQuery()
{
	const int32 QueryId = ActiveQueryId;
	ActiveQueryId = INDEX_NONE;
	if (QueryId != INDEX_NONE)
	{
		if (UEnvQueryManager* Manager = UEnvQueryManager::GetCurrent(GetWorld()))
		{
			Manager->AbortQuery(QueryId);
		}
	}
}
#endif

#if WITH_EDITOR
EDataValidationResult UAIFlowActorSpawnRecordEQS::IsDataValid(FDataValidationContext& Context) const
{
	const EDataValidationResult ParentResult = Super::IsDataValid(Context);
	if (!IsValid(LocationQuery.QueryTemplate.Get()) || LocationQuery.bUseBBKeyForQueryTemplate)
	{
		Context.AddError(FText::FromString(TEXT("Choose an EQS query asset; Blackboard query selection is not supported by this record.")));
		return EDataValidationResult::Invalid;
	}

	return ParentResult;
}
#endif
