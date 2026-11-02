// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors
#pragma once

#include "ActorSpawning/FlowDefaultActorSpawnRecord.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "Templates/SharedPointer.h"

#include "AIFlowActorSpawnRecordEQS.generated.h"

struct FEnvQueryResult;

/** Resolves a Flow actor's spawn transform through an asynchronous EQS location query. */
UCLASS(BlueprintType, Blueprintable, EditInlineNew, DisplayName = "Spawn using EQS")
class AIFLOWACTORSRUNTIME_API UAIFlowActorSpawnRecordEQS : public UFlowDefaultActorSpawnRecord
{
	GENERATED_BODY()

protected:
	/** A plain actor-querier query; project-specific dynamic contexts and fallback policies remain external. */
	UPROPERTY(EditAnywhere, Category = Configuration, DisplayName = "EQS Configuration",
		meta = (ShowOnlyInnerProperties, DisplayPriority = 2))
	FEQSParametrizedQueryExecutionRequest LocationQuery;

	/** Resolved transform retained until the deferred actor is acquired. */
	UPROPERTY(Transient)
	FTransform QuerySpawnTransform;

	/** Query ID used to cancel or reject a superseded EQS result. */
	int32 ActiveQueryId = INDEX_NONE;

	/** Record configuration captured when the query started; stale callbacks cannot spawn a reused slot. */
	uint32 ActiveConfigurationGeneration = 0;

	/** A successful query produced a usable location for this attempt. */
	bool bHasQuerySpawnTransform = false;

public:
#if WITH_SERVER_CODE
	virtual void CleanupRuntime() override;
#endif

protected:
#if WITH_SERVER_CODE
	virtual bool AdvanceCurrentState(EFlowActorSpawnRecordState& OutNextState) override;
	virtual void OnStateChanged(EFlowActorSpawnRecordState PreviousState, EFlowActorSpawnRecordState NextState) override;
	virtual FTransform GetDesiredSpawnTransform() const override;

	/** Ignore canceled, stale-generation, or superseded query results before resuming the record. */
	void HandleQueryFinished(TSharedPtr<FEnvQueryResult> Result);
	void CancelActiveQuery();
#endif

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif
};
