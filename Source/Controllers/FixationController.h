#pragma once

#include <JuceHeader.h>
#include <memory>
#include <vector>
#include "FixationWorker.h"
#include "MatchController.h"
#include "../PluginProcessor.h"
#include "../Data/Messages.h"
#include "../Data/Ranges.h"

class FixationController
{
public:
    explicit FixationController(MatchCompressorAudioProcessor& processor)
        : processor(processor)
    {}

    std::function<void(float fixationMismatch)> FixationApplied;

    std::function<void(float referenceMismatch)> ReferenceScored;

    std::function<void()> SessionEnded;

    void beginSession()
    {
        endSession();

        auto& matchingData = processor.getMatchingData();
        auto& refSamples = matchingData.refSamples;
        auto& destSamples = matchingData.destSamples;
        if (refSamples.empty() || refSamples[0].empty()
            || destSamples.empty() || destSamples[0].empty()
            || matchingData.calculatedCompParams.size() < 4)
            return; // no reference or no data: scoring is not possible

        worker = std::make_unique<FixationWorker>();
        juce::ValueTree paramsTree = getCurrentParamsTree();
        worker->prepare(
            refSamples,
            destSamples,
            matchingData.refSampleRate,
            matchingData.destSampleRate,
            paramsTree);
        preparedKneesNumber = (int)paramsTree.getProperty(setKneesNumberId);
        preparedBalFilterType = (int)paramsTree.getProperty(setBalFilterTypeId);
        preparedChannelAggregationType = (int)paramsTree.getProperty(setChannelAggregationTypeId);
        hasSessionReference = true;
        wireCallbacks();
        worker->start();
    }

    /// Re-prepare the scoring session 
    /// if the current plugin parameters differ from what it was prepared with. 
    /// Returns true if a re-prepare happened. 
    /// No-op without a reference session.
    bool refreshConfig()
    {
        if (!getHasSessionReference())
            return false;

        if (preparedKneesNumber != getCurrentKneesNumber())
        {
            juce::ValueTree tree = getCurrentParamsTree();
            worker->requestStructure(processor.getMatchingData().refSamples, tree);
            preparedKneesNumber = (int)tree.getProperty(setKneesNumberId);
            preparedBalFilterType = (int)tree.getProperty(setBalFilterTypeId);
            preparedChannelAggregationType = (int)tree.getProperty(setChannelAggregationTypeId);
            return true;
        }

        // Just rebuild the histogram on the worker thread.
        if (preparedBalFilterType != getCurrentBalFilterType() ||
            preparedChannelAggregationType != getCurrentChannelAggrerationType())
        {
            preparedBalFilterType = getCurrentBalFilterType();
            preparedChannelAggregationType = getCurrentChannelAggrerationType();
            worker->requestEnvSettings(preparedBalFilterType, preparedChannelAggregationType);
            return true;
        }
        return false;
    }

    void endSession()
    {
        const bool wasScoring = getHasSessionReference();
        worker.reset();
        hasSessionReference = false;
        sessionToken++;
        if (wasScoring)
            juce::NullCheckedInvocation::invoke(SessionEnded);
    }

    bool getHasSessionReference() const { return worker != nullptr && hasSessionReference; }

    void requestScore()
    {
        refreshConfig();
        if (getHasSessionReference())
            worker->requestScore(getCurrentCompParams(), getCurrentAttack(), getCurrentRelease());
    }

    /// Enter fixation mode. Returns false if there is no match / no source audio.
    bool enterFixation()
    {
        auto& matchingData = processor.getMatchingData();
        if (matchingData.calculatedCompParams.size() < 4)
            return false;

        refreshConfig();
        if (getHasSessionReference())
        {
            worker->requestRearm(getCurrentCompParams(), getCurrentAttack(), getCurrentRelease());
            return true;
        }

        // No reference session (Learn).
        auto& destSamples = matchingData.destSamples;
        if (destSamples.empty() || destSamples[0].empty())
            return false;

        worker = std::make_unique<FixationWorker>();
        juce::ValueTree config = getCurrentParamsTree();
        worker->prepare(destSamples, matchingData.destSampleRate, config);
        hasSessionReference = false;
        worker->arm(getCurrentAttack(), getCurrentRelease(), getCurrentCompParams());
        wireCallbacks();
        worker->start();
        return true;
    }

    void exitFixation()
    {
        if (getHasSessionReference())
            return;   // keep the persistent scoring worker alive
        endSession(); // Learn: drop the dest-only fixation worker
    }

    /// Re-arm from the current APVTS state after Reset replaced the curve and the ballistics 
    /// while the behind the worker is running.
    void restartFixation()
    {
        if (worker == nullptr)
            return;
        refreshConfig();
        if (getHasSessionReference())
            worker->requestRearm(getCurrentCompParams(), getCurrentAttack(), getCurrentRelease());
        else
        {
            exitFixation();
            enterFixation();
        }
    }

    void setTarget(float attackMs, float releaseMs)
    {
        if (worker != nullptr)
            worker->requestUpdate(attackMs, releaseMs);
    }

private:
    float getCurrentAttack() const { return *processor.apvts.getRawParameterValue(attackId); }
    float getCurrentRelease() const { return *processor.apvts.getRawParameterValue(releaseId); }
    int getCurrentKneesNumber() const
    {
        return juce::roundToInt(processor.apvts.getRawParameterValue(kneesNumberId)->load());
    }
    int getCurrentBalFilterType() const
    {
        return juce::roundToInt(processor.apvts.getRawParameterValue(balFilterTypeId)->load());
    }
    int getCurrentChannelAggrerationType() const
    {
        return juce::roundToInt(processor.apvts.getRawParameterValue(channelAggrerationTypeId)->load());
    }

    juce::ValueTree getCurrentParamsTree() const
    {
        auto& apvts = processor.apvts;
        juce::ValueTree t = processor.getMatchingData().properties.createCopy();
        t.setProperty(setKneesNumberId, getCurrentKneesNumber(), nullptr);
        t.setProperty(setBalFilterTypeId,
            juce::roundToInt(apvts.getRawParameterValue(balFilterTypeId)->load()), nullptr);
        t.setProperty(setChannelAggregationTypeId,
            juce::roundToInt(apvts.getRawParameterValue(channelAggrerationTypeId)->load()), nullptr);
        return t;
    }

    void wireCallbacks()
    {
        juce::WeakReference<FixationController> weak(this);
        const int token = sessionToken;
        worker->onParamsReady =
            [weak, token](float a, float r, const std::vector<float>& params, float fixationMismatch)
            {
                // worker thread -> message thread (async, never blocking: see stop()).
                juce::MessageManager::callAsync([weak, token, a, r, params, fixationMismatch]
                    {
                        if (auto* self = weak.get())
                            if (self->sessionToken == token)
                            {
                                self->applyFixationParams(a, r, params);
                                juce::NullCheckedInvocation::invoke(self->FixationApplied, fixationMismatch);
                            }
                    });
            };
        worker->onScoreReady =
            [weak, token](float referenceMismatch)
            {
                juce::MessageManager::callAsync([weak, token, referenceMismatch]
                    {
                        if (auto* self = weak.get())
                            if (self->sessionToken == token)
                                juce::NullCheckedInvocation::invoke(self->ReferenceScored, referenceMismatch);
                    });
            };
    }

    std::vector<float> getCurrentCompParams()
    {
        // Current plugin knee count, not the match's: the user may have changed it.
        return MatchController::getCurrentCompParams(processor.apvts, getCurrentKneesNumber());
    }

    void applyFixationParams(float attackMs, float releaseMs, const std::vector<float>& params)
    {
        int kneesNumber = ((int)params.size() - 1) / 3;

        setParameter(attackId, attackRange, attackMs);
        setParameter(releaseId, releaseRange, releaseMs);
        setParameter(gainId, gainRange, params[0]);
        for (int i = 0; i < kneesNumber; i++)
        {
            float t = params[i * 3 + 1];
            float ratioInverse = params[i * 3 + 2];
            float kw = params[i * 3 + 3];

            auto iStr = std::to_string(i);
            setParameter(thresholdId + iStr, thresholdRange, t);
            setParameter(ratioInverseId + iStr, ratioInverseRange, ratioInverse);
            setParameter(kneeWidthId + iStr, kneeWidthRange, kw);
        }
    }

    void setParameter(juce::String name, const juce::NormalisableRange<float> range, float value)
    {
        auto* par = processor.apvts.getParameter(name);
        par->beginChangeGesture();
        par->setValueNotifyingHost(range.convertTo0to1(value));
        par->endChangeGesture();
    }

    MatchCompressorAudioProcessor& processor;
    std::unique_ptr<FixationWorker> worker;
    bool hasSessionReference = false;
    int preparedKneesNumber = 0, preparedBalFilterType = 0, preparedChannelAggregationType = 0;
    int sessionToken = 0;

    JUCE_DECLARE_WEAK_REFERENCEABLE(FixationController)
};
