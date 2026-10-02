#pragma once

#include <JuceHeader.h>
#include <atomic>
#include <vector>
#include "../ParamsCalculator/CompParamsCalculator.h"

/// <summary>
/// Background driver for the real-time fixation mode.
/// Contract:
/// - prepare() runs on the caller (message) thread BEFORE start().
/// - once the thread runs, only the worker thread touches the estimator.
/// - requestUpdate() is called from any thread.
/// - onParamsReady is invoked from the WORKER thread.
/// </summary>
class FixationWorker : private juce::Thread
{
public:
    FixationWorker() : juce::Thread("FixationWorker") {}
    ~FixationWorker() override { stop(); }

    std::function<void(float attackMs, float releaseMs, float hpfFrequency, const std::vector<float>&, float fixedMismatch)> onParamsReady;

    std::function<void(float referenceMismatch)> onScoreReady;

    /// Preparation without a reference, only for fixation.
    /// Message thread, before start().
    void prepare(
        std::vector<std::vector<float>>& destSamples,
        double destSampleRate,
        juce::ValueTree& properties,
        bool isKWeightingUsed,
        const std::vector<bool>* isDestSampleKept)
    {
        this->isKWeightingUsed = isKWeightingUsed;
        if (isKWeightingUsed)
            weightedDest = CompParamsCalculator::applyKWeighting(destSamples, destSampleRate);
        estimator.prepareForFixation(
            isKWeightingUsed ? weightedDest : destSamples,
            destSampleRate,
            properties,
            isKWeightingUsed ? &destSamples : nullptr,
            isDestSampleKept);
        hasReference = false;
    }

    // Preparation with a reference, enabling scoreAgainstReference.
    // Message thread, before start().
    void prepare(
        std::vector<std::vector<float>>& refSamples,
        std::vector<std::vector<float>>& destSamples,
        double refSampleRate,
        double destSampleRate,
        juce::ValueTree& properties,
        bool isKWeightingUsed,
        const std::vector<bool>* isDestSampleKept)
    {
        this->isKWeightingUsed = isKWeightingUsed;
        if (isKWeightingUsed)
        {
            weightedRef = CompParamsCalculator::applyKWeighting(refSamples, refSampleRate);
            weightedDest = CompParamsCalculator::applyKWeighting(destSamples, destSampleRate);
        }
        estimator.prepare(
            isKWeightingUsed ? weightedRef : refSamples,
            isKWeightingUsed ? weightedDest : destSamples,
            destSampleRate,
            properties,
            isKWeightingUsed ? &destSamples : nullptr,
            isDestSampleKept);
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

    void requestMaterial(
        std::vector<std::vector<float>>& refSamples,
        std::vector<std::vector<float>>& destSamples,
        double refSampleRate,
        double destSampleRate,
        const juce::ValueTree& properties,
        bool isKWeightingUsed,
        const std::vector<bool>* isDestSampleKept)
    {
        {
            const juce::SpinLock::ScopedLockType lock(pendingLock);
            pendingMaterialRef = &refSamples;
            pendingMaterialDest = &destSamples;
            pendingMaterialRefSampleRate = refSampleRate;
            pendingMaterialDestSampleRate = destSampleRate;
            pendingMaterialProperties = properties;
            pendingMaterialKWeighting = isKWeightingUsed;
            pendingMaterialDestMask = isDestSampleKept;
        }
        materialGen.fetch_add(1, std::memory_order_release);
        notify();
    }

    bool isReferenceAvailable() const { return hasReference.load(); }

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

    void requestUpdate(float attackMs, float releaseMs, float hpfFrequency)
    {
        {
            const juce::SpinLock::ScopedLockType lock(pendingLock);
            pendingAttack = attackMs;
            pendingRelease = releaseMs;
            pendingHpfFrequency = hpfFrequency;
        }
        requestGen.fetch_add(1, std::memory_order_release);
        notify();
    }

    void requestRearm(
        const std::vector<float>& currentParams,
        float attackMs,
        float releaseMs,
        float hpfFrequency)
    {
        {
            const juce::SpinLock::ScopedLockType lock(pendingLock);
            pendingArmParams = currentParams;
            pendingArmAttack = attackMs;
            pendingArmRelease = releaseMs;
            pendingArmHpfFrequency = hpfFrequency;
        }
        armGen.fetch_add(1, std::memory_order_release);
        notify();
    }

    void requestEnvSettings(int balFilterTypeInt, int channelAggregationTypeInt, int useHpfInt)
    {
        {
            const juce::SpinLock::ScopedLockType lock(pendingLock);
            pendingBalFilter = balFilterTypeInt;
            pendingChannelAggregation = channelAggregationTypeInt;
            pendingUseHpf = useHpfInt;
        }
        envGen.fetch_add(1, std::memory_order_release);
        notify();
    }

    void requestScore(
        const std::vector<float>& params,
        float attackMs,
        float releaseMs,
        float hpfFrequency)
    {
        {
            const juce::SpinLock::ScopedLockType lock(pendingLock);
            pendingScoreParams = params;
            pendingScoreAttack = attackMs;
            pendingScoreRelease = releaseMs;
            pendingScoreHpfFrequency = hpfFrequency;
        }
        scoreGen.fetch_add(1, std::memory_order_release);
        notify();
    }

private:
    void rebuildIfNeeded(float attackMs, float releaseMs, float hpfFrequency, bool isSoftNeeded)
    {
        const bool isHardStale =
            attackMs != lastBuiltAttack ||
            releaseMs != lastBuiltRelease ||
            hpfFrequency != lastBuiltHpfFrequency;
        const bool isSoftStale = isSoftNeeded &&
            (attackMs != lastBuiltSoftAttack ||
                releaseMs != lastBuiltSoftRelease ||
                hpfFrequency != lastBuiltSoftHpfFrequency);
        if (!isHardStale && !isSoftStale)
            return;

        estimator.updateBallistics(attackMs, releaseMs, hpfFrequency, isSoftNeeded);
        lastBuiltAttack = attackMs;
        lastBuiltRelease = releaseMs;
        lastBuiltHpfFrequency = hpfFrequency;
        if (isSoftNeeded)
        {
            lastBuiltSoftAttack = attackMs;
            lastBuiltSoftRelease = releaseMs;
            lastBuiltSoftHpfFrequency = hpfFrequency;
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
                // 1st priority: K-weighting on/off
                const uint64_t mg = materialGen.load(std::memory_order_acquire);
                if (mg != materialProcessed.load())
                {
                    std::vector<std::vector<float>>* refSamples;
                    std::vector<std::vector<float>>* destSamples;
                    double refSampleRate, destSampleRate;
                    juce::ValueTree properties;
                    bool isWeighted;
                    const std::vector<bool>* isDestSampleKept;
                    {
                        const juce::SpinLock::ScopedLockType lock(pendingLock);
                        refSamples = pendingMaterialRef;
                        destSamples = pendingMaterialDest;
                        refSampleRate = pendingMaterialRefSampleRate;
                        destSampleRate = pendingMaterialDestSampleRate;
                        properties = pendingMaterialProperties;
                        isWeighted = pendingMaterialKWeighting;
                        isDestSampleKept = pendingMaterialDestMask;
                    }
                    if (refSamples != nullptr && destSamples != nullptr)
                    {
                        prepare(*refSamples, *destSamples, refSampleRate, destSampleRate,
                            properties, isWeighted, isDestSampleKept);
                        markHistogramsStale();
                    }
                    materialProcessed.store(mg);
                    continue;
                }

                // 2nd priority: number of knees
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
                        estimator.prepareStructure(
                            isKWeightingUsed ? weightedRef : *refSamples,
                            properties);
                        markHistogramsStale();
                    }
                    structureProcessed.store(stg);
                    continue;
                }

                // 3rd priority: the envelope or stereo processing type, HPF on/off
                const uint64_t eg = envGen.load(std::memory_order_acquire);
                if (eg != envProcessed.load())
                {
                    int bal, aggregation, useHpf;
                    {
                        const juce::SpinLock::ScopedLockType lock(pendingLock);
                        bal = pendingBalFilter;
                        aggregation = pendingChannelAggregation;
                        useHpf = pendingUseHpf;
                    }
                    estimator.updateEnvSettings(bal, aggregation, useHpf);
                    markHistogramsStale();
                    envProcessed.store(eg);
                    continue;
                }

                // 4th priority: the target quantiles array
                const uint64_t ag = armGen.load(std::memory_order_acquire);
                if (ag != armProcessed.load())
                {
                    std::vector<float> params;
                    float a, r, f;
                    {
                        const juce::SpinLock::ScopedLockType lock(pendingLock);
                        params = pendingArmParams;
                        a = pendingArmAttack;
                        r = pendingArmRelease;
                        f = pendingArmHpfFrequency;
                    }
                    if (!params.empty())
                    {
                        rebuildIfNeeded(a, r, f, false);
                        estimator.captureNominalKneeWidths(params);
                        target = estimator.calculateQuantilesFor(params);
                        lastResult = params;
                    }
                    armProcessed.store(ag);
                    continue;
                }

                // 5th priority: score (histogram rebuilding, no solve)
                const uint64_t sg = scoreGen.load(std::memory_order_acquire);
                if (sg != scoreProcessed.load())
                {
                    std::vector<float> params;
                    float a, r, f;
                    {
                        const juce::SpinLock::ScopedLockType lock(pendingLock);
                        params = pendingScoreParams;
                        a = pendingScoreAttack;
                        r = pendingScoreRelease;
                        f = pendingScoreHpfFrequency;
                    }
                    if (hasReference && !params.empty())
                    {
                        rebuildIfNeeded(a, r, f, true);
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

                // 6th priority: attack/release/HPF frequency values
                const uint64_t gen = requestGen.load(std::memory_order_acquire);
                if (gen == processedGen.load())
                    break; // nothing new

                float attackMs, releaseMs, hpfFrequency;
                {
                    const juce::SpinLock::ScopedLockType lock(pendingLock);
                    attackMs = pendingAttack;
                    releaseMs = pendingRelease;
                    hpfFrequency = pendingHpfFrequency;
                }

                if (target.empty())
                {
                    jassertfalse; // requestUpdate() before requestRearm()
                    processedGen.store(gen);
                    continue;
                }

                rebuildIfNeeded(attackMs, releaseMs, hpfFrequency, false);
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
                    onParamsReady(attackMs, releaseMs, hpfFrequency, result, estimator.getLastFitMismatch());
            }
        }
    }

    CompParamsCalculator estimator;
    std::atomic<bool> hasReference{ false };

    bool isKWeightingUsed = false;
    std::vector<std::vector<float>> weightedRef, weightedDest;

    // Worker thread state.
    std::vector<float> target;
    std::vector<float> lastResult;
    float lastBuiltAttack = std::numeric_limits<float>::quiet_NaN();
    float lastBuiltRelease = std::numeric_limits<float>::quiet_NaN();
    float lastBuiltSoftAttack = std::numeric_limits<float>::quiet_NaN();
    float lastBuiltSoftRelease = std::numeric_limits<float>::quiet_NaN();
    float lastBuiltHpfFrequency = std::numeric_limits<float>::quiet_NaN();
    float lastBuiltSoftHpfFrequency = std::numeric_limits<float>::quiet_NaN();

    // Single-slot coalescing mailbox.
    juce::SpinLock pendingLock;
    float pendingAttack = 0.f, pendingRelease = 0.f, pendingHpfFrequency = 0.f;
    std::vector<float> pendingArmParams;
    float pendingArmAttack = 0.f, pendingArmRelease = 0.f, pendingArmHpfFrequency = 0.f;
    std::vector<std::vector<float>>* pendingStructureRef = nullptr;
    juce::ValueTree pendingStructureProperties;
    std::vector<std::vector<float>>* pendingMaterialRef = nullptr;
    std::vector<std::vector<float>>* pendingMaterialDest = nullptr;
    double pendingMaterialRefSampleRate = 0., pendingMaterialDestSampleRate = 0.;
    juce::ValueTree pendingMaterialProperties;
    bool pendingMaterialKWeighting = false;
    const std::vector<bool>* pendingMaterialDestMask = nullptr;
    std::vector<float> pendingScoreParams;
    float pendingScoreAttack = 0.f, pendingScoreRelease = 0.f, pendingScoreHpfFrequency = 0.f;
    int pendingBalFilter = 0, pendingChannelAggregation = 0, pendingUseHpf = 0;
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
    std::atomic<uint64_t> materialGen{ 0 };
    std::atomic<uint64_t> materialProcessed{ 0 };
};
