#pragma once

#include <JuceHeader.h>
#include <atomic>
#include <vector>
#include "../ParamsCalculator/CompParamsCalculatorEnv.h"

/// <summary>
/// Background driver for the real-time fixation mode.
/// Contract:
/// - prepare() and arm() run on the caller (message) thread BEFORE start().
/// - once the thread runs, only the worker thread touches the estimator.
/// - requestUpdate() is called from any thread.
/// - onParamsReady is invoked from the WORKER thread.
/// </summary>
class FixationWorker : private juce::Thread
{
public:
    FixationWorker() : juce::Thread("FixationWorker") {}
    ~FixationWorker() override { stop(); }

    std::function<void(float attackMs, float releaseMs, const std::vector<float>&, float fixedMismatch)> onParamsReady;

    std::function<void(float referenceMismatch)> onScoreReady;

    /// Preparation without a reference, only for fixation.
    /// Message thread, before arm()/start().
    void prepare(
        std::vector<std::vector<float>>& destSamples,
        double destSampleRate,
        juce::ValueTree& properties)
    {
        estimator.prepareForFixation(destSamples, destSampleRate, properties);
        hasReference = false;
    }

    // Preparation with a reference, enabling scoreAgainstReference.
    // Message thread, before arm()/start().
    void prepare(
        std::vector<std::vector<float>>& refSamples,
        std::vector<std::vector<float>>& destSamples,
        double destSampleRate,
        juce::ValueTree& properties)
    {
        estimator.prepare(refSamples, destSamples, destSampleRate, properties);
        hasReference = true;
    }

    void requestStructure(
        std::vector<std::vector<float>>& refSamples,
        const juce::ValueTree& properties)
    {
        {
            const juce::SpinLock::ScopedLockType lock(pendingLock);
            pendingStructureRef = &refSamples;
            pendingStructureProperties = properties;
        }
        structureGen.fetch_add(1, std::memory_order_release);
        notify();
    }

    bool isReferenceAvailable() const { return hasReference; }

    // Enter fixation. Message thread, before start().
    void arm(float attackMs, float releaseMs, const std::vector<float>& currentParams)
    {
        estimator.updateBallistics(attackMs, releaseMs, false);
        lastBuiltAttack = attackMs;
        lastBuiltRelease = releaseMs;
        estimator.captureNominalKneeWidths(currentParams);
        target = estimator.calculateQuantilesFor(currentParams);
        lastResult = currentParams;
        pendingAttack = attackMs;
        pendingRelease = releaseMs;
        processedGen.store(requestGen.load());
    }

    void start() 
    { 
        startThread(); 
    }

    void stop()
    {
        // Wait for a clean exit: a killed thread would orphan the SpinLock (deadlock), 
        // leak the solve's allocations, and leave the estimator/allocator corrupt. 
        stopThread(-1);
    }

    void requestUpdate(float attackMs, float releaseMs)
    {
        {
            const juce::SpinLock::ScopedLockType lock(pendingLock);
            pendingAttack = attackMs;
            pendingRelease = releaseMs;
        }
        requestGen.fetch_add(1, std::memory_order_release);
        notify();
    }

    void requestRearm(const std::vector<float>& currentParams, float attackMs, float releaseMs)
    {
        {
            const juce::SpinLock::ScopedLockType lock(pendingLock);
            pendingArmParams = currentParams;
            pendingArmAttack = attackMs;
            pendingArmRelease = releaseMs;
        }
        armGen.fetch_add(1, std::memory_order_release);
        notify();
    }

    void requestEnvSettings(int balFilterTypeInt, int channelAggregationTypeInt)
    {
        {
            const juce::SpinLock::ScopedLockType lock(pendingLock);
            pendingBalFilter = balFilterTypeInt;
            pendingChannelAggregation = channelAggregationTypeInt;
        }
        envGen.fetch_add(1, std::memory_order_release);
        notify();
    }

    void requestScore(const std::vector<float>& params, float attackMs, float releaseMs)
    {
        {
            const juce::SpinLock::ScopedLockType lock(pendingLock);
            pendingScoreParams = params;
            pendingScoreAttack = attackMs;
            pendingScoreRelease = releaseMs;
        }
        scoreGen.fetch_add(1, std::memory_order_release);
        notify();
    }

private:
    void rebuildIfNeeded(float attackMs, float releaseMs, bool isSoftNeeded)
    {
        const bool isHardStale = attackMs != lastBuiltAttack || releaseMs != lastBuiltRelease;
        const bool isSoftStale = isSoftNeeded
            && (attackMs != lastBuiltSoftAttack || releaseMs != lastBuiltSoftRelease);
        if (!isHardStale && !isSoftStale)
            return;

        estimator.updateBallistics(attackMs, releaseMs, isSoftNeeded);
        lastBuiltAttack = attackMs;
        lastBuiltRelease = releaseMs;
        if (isSoftNeeded)
        {
            lastBuiltSoftAttack = attackMs;
            lastBuiltSoftRelease = releaseMs;
        }
    }

    void markHistogramsStale()
    {
        lastBuiltAttack = lastBuiltRelease = std::numeric_limits<float>::quiet_NaN();
        lastBuiltSoftAttack = lastBuiltSoftRelease = std::numeric_limits<float>::quiet_NaN();
    }

    void run() override
    {
        while (!threadShouldExit())
        {
            wait(-1); // sleep until requestUpdate() or stop() notifies

            while (!threadShouldExit())
            {
                const uint64_t stg = structureGen.load(std::memory_order_acquire);
                if (stg != structureProcessed.load())
                {
                    std::vector<std::vector<float>>* refSamples;
                    juce::ValueTree properties;
                    {
                        const juce::SpinLock::ScopedLockType lock(pendingLock);
                        refSamples = pendingStructureRef;
                        properties = pendingStructureProperties;
                    }
                    if (refSamples != nullptr)
                    {
                        estimator.prepareStructure(*refSamples, properties);
                        markHistogramsStale();
                    }
                    structureProcessed.store(stg);
                    continue;
                }

                const uint64_t eg = envGen.load(std::memory_order_acquire);
                if (eg != envProcessed.load())
                {
                    int bal, aggregation;
                    {
                        const juce::SpinLock::ScopedLockType lock(pendingLock);
                        bal = pendingBalFilter;
                        aggregation = pendingChannelAggregation;
                    }
                    estimator.updateEnvSettings(bal, aggregation);
                    markHistogramsStale();
                    envProcessed.store(eg);
                    continue;
                }

                // 2nd priority: re-arm. 
                // It redefines the target quantiles array.
                const uint64_t ag = armGen.load(std::memory_order_acquire);
                if (ag != armProcessed.load())
                {
                    std::vector<float> params;
                    float a, r;
                    {
                        const juce::SpinLock::ScopedLockType lock(pendingLock);
                        params = pendingArmParams;
                        a = pendingArmAttack;
                        r = pendingArmRelease;
                    }
                    if (!params.empty())
                    {
                        rebuildIfNeeded(a, r, false);
                        estimator.captureNominalKneeWidths(params);
                        target = estimator.calculateQuantilesFor(params);
                        lastResult = params;
                    }
                    armProcessed.store(ag);
                    continue;
                }

                // 3rd priority: score.
                // No solve, only the histogram needs a rebuild.
                const uint64_t sg = scoreGen.load(std::memory_order_acquire);
                if (sg != scoreProcessed.load())
                {
                    std::vector<float> params;
                    float a, r;
                    {
                        const juce::SpinLock::ScopedLockType lock(pendingLock);
                        params = pendingScoreParams;
                        a = pendingScoreAttack;
                        r = pendingScoreRelease;
                    }
                    if (hasReference && !params.empty())
                    {
                        rebuildIfNeeded(a, r, true);
                        if (scoreGen.load() != sg)
                            continue; // a newer score request arrived
                        const float q = estimator.scoreAgainstReference(params);
                        if (scoreGen.load() != sg)
                            continue; // a newer score request arrived - drop the current result
                        scoreProcessed.store(sg);
                        if (onScoreReady)
                            onScoreReady(q);
                    }
                    else
                        scoreProcessed.store(sg);
                    continue;
                }

                // 4th priority: new attack/release parameters.
                const uint64_t gen = requestGen.load(std::memory_order_acquire);
                if (gen == processedGen.load())
                    break; // nothing new

                float attackMs, releaseMs;
                {
                    const juce::SpinLock::ScopedLockType lock(pendingLock);
                    attackMs = pendingAttack;
                    releaseMs = pendingRelease;
                }

                if (target.empty())
                {
                    jassertfalse; // requestUpdate() before arm()
                    processedGen.store(gen);
                    continue;
                }

                rebuildIfNeeded(attackMs, releaseMs, false);
                if (requestGen.load() != gen)
                    continue; // a newer request arrived

                std::vector<float> result;
                try
                {
                    result = estimator.solve(target, target, &lastResult);
                }
                catch (const std::exception&)
                {
                    processedGen.store(gen); // give up on this request, keep the loop alive
                    continue;
                }
                if (requestGen.load() != gen)
                    continue; // a newer request arrived - drop the current result

                lastResult = result;
                processedGen.store(gen);
                if (onParamsReady)
                    onParamsReady(attackMs, releaseMs, result, estimator.getLastFitMismatch());
            }
        }
    }

    CompParamsCalculatorEnv estimator;
    bool hasReference = false;

    // Worker thread state.
    std::vector<float> target;
    std::vector<float> lastResult;
    float lastBuiltAttack = std::numeric_limits<float>::quiet_NaN();
    float lastBuiltRelease = std::numeric_limits<float>::quiet_NaN();
    float lastBuiltSoftAttack = std::numeric_limits<float>::quiet_NaN();
    float lastBuiltSoftRelease = std::numeric_limits<float>::quiet_NaN();

    // Single-slot coalescing mailbox.
    juce::SpinLock pendingLock;
    float pendingAttack = 0.f, pendingRelease = 0.f;
    std::vector<float> pendingArmParams;
    float pendingArmAttack = 0.f, pendingArmRelease = 0.f;
    std::vector<std::vector<float>>* pendingStructureRef = nullptr;
    juce::ValueTree pendingStructureProperties;
    std::vector<float> pendingScoreParams;
    float pendingScoreAttack = 0.f, pendingScoreRelease = 0.f;
    int pendingBalFilter = 0, pendingChannelAggregation = 0;
    std::atomic<uint64_t> requestGen{ 0 };
    std::atomic<uint64_t> processedGen{ 0 };
    std::atomic<uint64_t> armGen{ 0 };
    std::atomic<uint64_t> armProcessed{ 0 };
    std::atomic<uint64_t> scoreGen{ 0 };
    std::atomic<uint64_t> scoreProcessed{ 0 };
    std::atomic<uint64_t> envGen{ 0 };
    std::atomic<uint64_t> envProcessed{ 0 };
    std::atomic<uint64_t> structureGen{ 0 };
    std::atomic<uint64_t> structureProcessed{ 0 };
};
