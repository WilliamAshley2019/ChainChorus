#include "PresetManager.h"
#pragma once

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include <JuceHeader.h>
#include <atomic>
#include <array>

//==============================================================================
// ChainChorus — Master Chorus Engine
// Merges KR-106 BBD Chorus + Analog Character + Mains/Clock Drift
// Derived from Ultra Stinger's kr106::Chorus primitives
// AlphaAudio by William Ashley Music
//==============================================================================

namespace kr106
{

    //==============================================================================
    // Model enum
    //==============================================================================
    enum Model { kJ6 = 0, kJ60 = 1, kJ106 = 2 };

    //==============================================================================
    // Analog noise floor
    //==============================================================================
    namespace analog_noise
    {
        constexpr float kDryBroadbandGain = 1.5e-3f;
        constexpr float kDryRipple120 = 1.8e-5f;
        constexpr float kDryRipple240 = 8.9e-6f;
        constexpr float kDryRipple360 = 6.3e-6f;
        constexpr float kWetBroadbandGain = 1.8e-3f;
        constexpr float kWetShelfCornerHz = 3000.f;
        constexpr float kWetShelfBoostDb = 6.f;
        constexpr float kWetRipple120 = 7.9e-5f;
        constexpr float kWetRipple240 = 2.2e-5f;
        constexpr float kWetRipple360 = 9.8e-6f;
    }

    struct AnalogFloorNoise
    {
        uint32_t mSeed = 0x12345678u;
        float mLpState = 0.f;
        float mLpCoeff = 0.f;
        float mShelfState = 0.f;
        float mShelfCoeff = 0.f;
        float mShelfGain = 0.f;
        float mPink0 = 0, mPink1 = 0, mPink2 = 0, mPink3 = 0, mPink4 = 0, mPink5 = 0, mPink6 = 0;
        bool mPinkEnabled = false;

        void Init(float sampleRate, float cutoffHz = 20000.f)
        {
            const float fc = std::min(cutoffHz, sampleRate * 0.45f);
            mLpCoeff = 1.f - expf(-2.f * (float)M_PI * fc / sampleRate);
            mShelfCoeff = 0.f;
            mShelfGain = 0.f;
        }

        void SetHighShelf(float shelfFcHz, float hfBoostDb, float sampleRate)
        {
            const float fc = std::min(shelfFcHz, sampleRate * 0.45f);
            mShelfCoeff = 1.f - expf(-2.f * (float)M_PI * fc / sampleRate);
            mShelfGain = powf(10.f, hfBoostDb / 20.f) - 1.f;
        }

        float Process()
        {
            mSeed = mSeed * 196314165u + 907633515u;
            float white = (2.f * (float)mSeed / (float)0xFFFFFFFFu) - 1.f;

            if (mPinkEnabled)
            {
                mPink0 = 0.99886f * mPink0 + white * 0.0555179f;
                mPink1 = 0.99332f * mPink1 + white * 0.0750759f;
                mPink2 = 0.96900f * mPink2 + white * 0.1538520f;
                mPink3 = 0.86650f * mPink3 + white * 0.3104856f;
                mPink4 = 0.55000f * mPink4 + white * 0.5329522f;
                mPink5 = -0.7616f * mPink5 - white * 0.0168980f;
                float pink = mPink0 + mPink1 + mPink2 + mPink3 + mPink4 + mPink5 + mPink6 + white * 0.5362f;
                mPink6 = white * 0.115926f;
                float pinkOut = pink * 0.11f;
                if (mShelfGain != 0.f)
                {
                    mShelfState += mShelfCoeff * (pinkOut - mShelfState);
                    return pinkOut + mShelfGain * (pinkOut - mShelfState);
                }
                return pinkOut;
            }
            mLpState += mLpCoeff * (white - mLpState);
            if (mShelfGain != 0.f)
            {
                mShelfState += mShelfCoeff * (mLpState - mShelfState);
                return mLpState + mShelfGain * (mLpState - mShelfState);
            }
            return mLpState;
        }
    };

    //==============================================================================
    // Rail ripple (mains hum)
    //==============================================================================
    struct RailRipple
    {
        float mPhase = 0.f, mInc = 0.f;
        float mA1 = 0.f, mA2 = 0.f, mA3 = 0.f;

        void SetMainsHz(float mainsHz, float sampleRate) { mInc = (2.f * mainsHz) / sampleRate; }
        void SetAmplitudes(float a1, float a2, float a3) { mA1 = a1; mA2 = a2; mA3 = a3; }
        void Reset() { mPhase = 0.f; }
        float Process()
        {
            mPhase += mInc;
            if (mPhase >= 1.f) mPhase -= 1.f;
            const float tp = 2.f * (float)M_PI * mPhase;
            return mA1 * sinf(tp) + mA2 * sinf(2.f * tp) + mA3 * sinf(3.f * tp);
        }
    };

    //==============================================================================
    // BBD filter (2nd-order Butterworth + 1-pole)
    //==============================================================================
    struct BBDFilter
    {
        static constexpr float kBiquadFc = 9661.f;
        static constexpr float kBiquadQ = 0.7071f;
        static constexpr float kPoleFc = 20000.f;

        struct SVFBiquad
        {
            float mIC1eq = 0.f, mIC2eq = 0.f;
            void Reset() { mIC1eq = mIC2eq = 0.f; }
            float Process(float input, float g, float a1)
            {
                float v3 = input - mIC2eq;
                float v1 = a1 * mIC1eq + a1 * g * v3;
                float v2 = mIC2eq + g * v1;
                mIC1eq = 2.f * v1 - mIC1eq;
                mIC2eq = 2.f * v2 - mIC2eq;
                return v2;
            }
        };

        struct TPT1
        {
            float mS = 0.f;
            void Reset() { mS = 0.f; }
            float Process(float input, float g)
            {
                float v = (input - mS) * g / (1.f + g);
                float lp = mS + v;
                mS = lp + v;
                return lp;
            }
        };

        SVFBiquad mBiquad;
        TPT1 mPole;
        float mG_bq = 0.f, mA1_bq = 0.f, mG_p = 0.f;

        void Init(float sampleRate)
        {
            float fc = std::min(kBiquadFc, sampleRate * 0.45f);
            mG_bq = tanf((float)M_PI * fc / sampleRate);
            mA1_bq = 1.f / (1.f + mG_bq / kBiquadQ + mG_bq * mG_bq);
            float fc_p = std::min(kPoleFc, sampleRate * 0.45f);
            mG_p = tanf((float)M_PI * fc_p / sampleRate);
            Reset();
        }

        void Reset() { mBiquad.Reset(); mPole.Reset(); }
        float Process(float input)
        {
            float x = mBiquad.Process(input, mG_bq, mA1_bq);
            return mPole.Process(x, mG_p);
        }
    };

    //==============================================================================
    // Chorus LFO
    //==============================================================================
    struct ChorusLFO
    {
        float mPhase = 0.f, mInc = 0.f;
        void SetRate(float hz, float sampleRate) { mInc = hz / sampleRate; }
        void Reset() { mPhase = 0.f; }
        float Triangle()
        {
            mPhase += mInc;
            if (mPhase >= 1.f) mPhase -= 1.f;
            return 1.f - 4.f * fabsf(mPhase - 0.5f);
        }
        float Sine()
        {
            mPhase += mInc;
            if (mPhase >= 1.f) mPhase -= 1.f;
            return sinf(2.f * (float)M_PI * mPhase);
        }
        void SetPhase(float p) { mPhase = p; }
    };

    //==============================================================================
    // Click ring
    //==============================================================================
    struct ClickRing
    {
        float mFreqCoeff = 0.f, mDamp = 0.f;
        float mLow = 0.f, mBand = 0.f;
        void Init(float resHz, float Q, float sampleRate)
        {
            mFreqCoeff = 2.f * std::sin((float)M_PI * resHz / sampleRate);
            mDamp = 1.f / Q;
            Reset();
        }
        void Reset() { mLow = mBand = 0.f; }
        float Process(float input)
        {
            mLow += mFreqCoeff * mBand;
            float high = input - mLow - mDamp * mBand;
            mBand += mFreqCoeff * high;
            return mLow;
        }
    };

    //==============================================================================
    // Leak noise
    //==============================================================================
    struct LeakNoise
    {
        uint32_t mSeed = 0xDEADBEEFu;
        float mHpState = 0.f, mHpCoeff = 0.f;
        float mLpState = 0.f, mLpCoeff = 0.f;
        void Init(float sampleRate, float hpCornerHz = 1500.f)
        {
            mHpCoeff = 1.f - expf(-2.f * (float)M_PI * hpCornerHz / sampleRate);
            mLpCoeff = 1.f - std::exp(-2.f * (float)M_PI * 4000.f / sampleRate);
        }
        float Process()
        {
            mSeed = mSeed * 196314165u + 907633515u;
            float white = (2.f * (float)mSeed / (float)0xFFFFFFFFu) - 1.f;
            mHpState += mHpCoeff * (white - mHpState);
            float hp = white - mHpState;
            mLpState += mLpCoeff * (hp - mLpState);
            return mLpState;
        }
    };

    //==============================================================================
    // BBD click
    //==============================================================================
    struct BBDClick
    {
        int mCounter = -1;
        int mDurationSamples = 0;
        bool mWasInZone = false;
        static constexpr float kTriggerThreshold = 0.95f;
        static constexpr float kDurationMs = 180.f;

        void Init(float sampleRate) { mDurationSamples = (int)(kDurationMs * 0.001f * sampleRate); }
        void Reset() { mCounter = -1; mWasInZone = false; }
        void Suppress() { mCounter = -1; mWasInZone = true; }

        float Process(float lfoForLine)
        {
            bool inZone = (lfoForLine > kTriggerThreshold);
            if (inZone && !mWasInZone) mCounter = 0;
            mWasInZone = inZone;
            if (mCounter < 0 || mCounter >= mDurationSamples)
            {
                mCounter = -1;
                return 0.f;
            }
            constexpr float kAsymPoint = 0.20f;
            constexpr float kSecondLobeScale = 0.85f;
            constexpr float kLeadDecayRate = 4.0f;
            constexpr float kTrailDecayRate = 8.0f;
            float t = (float)mCounter / (float)mDurationSamples;
            mCounter++;
            float out;
            if (t < kAsymPoint)
            {
                float u = t / kAsymPoint;
                out = -expf(-kLeadDecayRate * u) * sinf((float)M_PI * u);
            }
            else
            {
                float u = (t - kAsymPoint) / (1.f - kAsymPoint);
                out = kSecondLobeScale * expf(-kTrailDecayRate * u) * sinf((float)M_PI * u);
            }
            return out;
        }
    };

    //==============================================================================
    // BBD delay line
    //==============================================================================
    struct BBDLine
    {
        static constexpr int kNumStages = 256;
        std::vector<float> mBuf;
        int mMask = 0;
        int mWPos = 0;
        BBDFilter mPreFilter;
        BBDFilter mPostFilter;
        static constexpr float kBBDSatDriveDefault = 0.12f;
        float mSatDrive = kBBDSatDriveDefault;
        float mSatDriveSmooth = kBBDSatDriveDefault;
        float mSampleRate = 44100.f;

        void Init(float sampleRate)
        {
            mSampleRate = sampleRate;
            int minLen = (int)(sampleRate * 0.020f) + 4;
            int len = 1;
            while (len < minLen) len <<= 1;
            mBuf.assign((size_t)len, 0.f);
            mMask = len - 1;
            mWPos = 0;
            mPreFilter.Init(sampleRate);
            mPostFilter.Init(sampleRate);
        }

        void Clear()
        {
            std::fill(mBuf.begin(), mBuf.end(), 0.f);
            mWPos = 0;
            mPreFilter.Reset();
            mPostFilter.Reset();
        }

        static float Hermite(float frac, float y0, float y1, float y2, float y3)
        {
            float c0 = y1;
            float c1 = 0.5f * (y2 - y0);
            float c2 = y0 - 2.5f * y1 + 2.f * y2 - 0.5f * y3;
            float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
            return ((c3 * frac + c2) * frac + c1) * frac + c0;
        }

        float ReadHermite(float delaySamples) const
        {
            float rPos = (float)mWPos - delaySamples;
            if (rPos < 0.f) rPos += (float)(mMask + 1);
            int i1 = (int)rPos;
            float frac = rPos - (float)i1;
            return Hermite(frac,
                mBuf[(i1 - 1) & mMask],
                mBuf[i1 & mMask],
                mBuf[(i1 + 1) & mMask],
                mBuf[(i1 + 2) & mMask]);
        }

        float Process(float input, float delaySamples, float injectedNoise = 0.f)
        {
            float withNoise = input + injectedNoise;
            float filtered = mPreFilter.Process(withNoise);
            mSatDriveSmooth += (mSatDrive - mSatDriveSmooth) * 0.001f;
            float sd = mSatDriveSmooth;
            float sat = (sd > 0.01f) ? tanhf(filtered * sd) / sd : filtered;
            mBuf[mWPos & mMask] = sat;
            float wet = ReadHermite(delaySamples);
            mWPos = (mWPos + 1) & mMask;
            return mPostFilter.Process(wet);
        }
    };

    //==============================================================================
    // Chorus — the main BBD dual-line chorus
    //==============================================================================
    struct Chorus
    {
        BBDLine mLine0, mLine1;
        ChorusLFO mLFO;
        BBDClick mClick0, mClick1;
        BBDClick mSlowClick0, mSlowClick1;
        LeakNoise mLeakNoise0, mLeakNoise1;
        ClickRing mClickRing0, mClickRing1;
        int mMode = 0;
        int mPendingMode = 0;
        bool mUseSine = false;
        float mSampleRate = 44100.f;

        static constexpr float kCenterDelayMs = 3.30f;
        static constexpr float kMinClockHz = 5000.f;
        static constexpr float kMinDelayMs = 0.1f;
        static constexpr float kChorusIRate = 0.514f;
        static constexpr float kChorusIIRate = 0.842f;
        static constexpr float kChorusI_IIRate = 7.85f;
        static constexpr float kChorusIDelayDepthMs = 2.13f;
        static constexpr float kChorusIIDelayDepthMs = 1.71f;
        static constexpr float kChorusI_IIDelayDepthMs = 0.236f;
        static constexpr float kDryGain = 0.863f;
        static constexpr float kWetGain = 1.257f;
        static constexpr float kBBDGainModC1 = 0.08f;
        static constexpr float kBBDCTELossCoeff = 4468.f;
        static constexpr float kBBDInvClockCenter = 1.f / 40000.f;
        static constexpr float kBBDLeakageGain = 8.8e-3f;
        static constexpr float kBBDClockTrim = 0.015f;
        static constexpr float kBBDGainTrim = 0.04f;
        static constexpr float kBBDClickGain = 0.11f;
        static constexpr float kBBDSlowClickGain = 0.022f;
        static constexpr float kClickRingFreqHz = 30.f;
        static constexpr float kClickRingQ = 18.f;
        static constexpr float kClickRingGain = 0.06f;
        static constexpr float kFadeMs = 5.f;
        float mFade = 0.f, mFadeTarget = 0.f, mFadeInc = 0.f;
        float mDelayDepth = 0.f, mTargetDelayDepth = 0.f, mGainModScale = 1.f;

        AnalogFloorNoise mWetPink0, mWetPink1;
        RailRipple mWetRipple;
        float mAnalogMul = 1.f, mMainsMul = 1.f, mClockMul = 1.f;
        float mBbdDriveUser = 50.f;

        // --- User overrides (exposed to the plugin) ---
        float mUserRateMul = 1.f;
        float mUserDepthMul = 1.f;
        float mUserWidth = 0.5f;      // 0 = mono-ish, 1 = full stereo phase offset
        float mUserMix = 1.f;         // 0..1 wet mix inside the chorus
        float mUserWetHpfHz = 0.f;    // wet-path HPF cutoff (0 = off)

        // Wet HPF state (one-pole per channel)
        float mHpfZL = 0.f, mHpfZR = 0.f, mHpfCoeff = 0.f;

        void SetWetHpf(float hz, float sampleRate)
        {
            if (hz <= 0.f)
            {
                mHpfCoeff = 0.f;
            }
            else
            {
                float fc = std::min(hz, sampleRate * 0.45f);
                mHpfCoeff = 1.f - expf(-2.f * (float)M_PI * fc / sampleRate);
            }
        }

        void updateBbdDrive()
        {
            float norm = mBbdDriveUser / 100.f;
            float drive = 0.48f * norm * norm;
            mLine0.mSatDrive = drive;
            mLine1.mSatDrive = drive;
        }

        void Init(float sampleRate)
        {
            mSampleRate = sampleRate;
            mLine0.Init(sampleRate);
            mLine1.Init(sampleRate);
            mClick0.Init(sampleRate);
            mClick1.Init(sampleRate);
            mSlowClick0.Init(sampleRate);
            mSlowClick1.Init(sampleRate);
            mLeakNoise0.Init(sampleRate, 800.f);
            mLeakNoise1.Init(sampleRate, 800.f);
            mClickRing0.Init(kClickRingFreqHz, kClickRingQ, sampleRate);
            mClickRing1.Init(kClickRingFreqHz, kClickRingQ, sampleRate);
            mWetPink0.Init(sampleRate);
            mWetPink1.Init(sampleRate);
            mWetPink0.SetHighShelf(analog_noise::kWetShelfCornerHz,
                analog_noise::kWetShelfBoostDb, sampleRate);
            mWetPink1.SetHighShelf(analog_noise::kWetShelfCornerHz,
                analog_noise::kWetShelfBoostDb, sampleRate);
            mWetRipple.SetMainsHz(60.f, sampleRate);
            mWetRipple.SetAmplitudes(analog_noise::kWetRipple120,
                analog_noise::kWetRipple240,
                analog_noise::kWetRipple360);
            mWetRipple.Reset();
            mLFO.Reset();
            mClick0.Reset(); mClick1.Reset();
            mSlowClick0.Reset(); mSlowClick1.Reset();
            mClickRing0.Reset(); mClickRing1.Reset();
            mFadeInc = 1.f / (kFadeMs * 0.001f * sampleRate);
            mHpfZL = mHpfZR = 0.f;
            if (mMode > 0)
            {
                ConfigureMode();
                mDelayDepth = mTargetDelayDepth;
                mFade = mFadeTarget = 1.f;
            }
        }

        void SetMode(int newMode)
        {
            if (mFadeInc <= 0.f && mSampleRate > 0.f)
                mFadeInc = 1.f / (kFadeMs * 0.001f * mSampleRate);
            if (newMode == mMode && newMode == mPendingMode) return;
            if (mMode > 0 && newMode > 0)
            {
                mPendingMode = newMode;
                mFadeTarget = 0.f;
                return;
            }
            mPendingMode = newMode;
            mMode = newMode;
            if (mMode == 0) { mFadeTarget = 0.f; return; }
            ConfigureMode();
            mFadeTarget = 1.f;
            if (mFade <= 0.f) mFade = mFadeInc;
            mClick0.Suppress(); mClick1.Suppress();
            mSlowClick0.Suppress(); mSlowClick1.Suppress();
            mClickRing0.Reset(); mClickRing1.Reset();
        }

        void Reset()
        {
            mLine0.Clear();
            mLine1.Clear();
            mClick0.Reset(); mClick1.Reset();
            mSlowClick0.Reset(); mSlowClick1.Reset();
            mClickRing0.Reset(); mClickRing1.Reset();
            mWetRipple.Reset();
            mHpfZL = mHpfZR = 0.f;
        }

        void Process(float input, float& outL, float& outR)
        {
            if (mFade < mFadeTarget) mFade = std::min(mFade + mFadeInc, mFadeTarget);
            else if (mFade > mFadeTarget) mFade = std::max(mFade - mFadeInc, mFadeTarget);

            if (mFade <= 0.f && mPendingMode != mMode)
            {
                mMode = mPendingMode;
                if (mMode > 0)
                {
                    ConfigureMode();
                    mDelayDepth = mTargetDelayDepth;
                    mFadeTarget = 1.f;
                    mClick0.Suppress(); mClick1.Suppress();
                    mSlowClick0.Suppress(); mSlowClick1.Suppress();
                    mClickRing0.Reset(); mClickRing1.Reset();
                }
            }

            mDelayDepth += (mTargetDelayDepth - mDelayDepth) * mFadeInc;

            float lfo = mUseSine ? mLFO.Sine() : mLFO.Triangle();

            // Apply user rate multiplier by scaling the LFO increment per-sample.
            // (Simple approach: the user rate multiplier is applied at SetMode via
            // ConfigureMode; here we simply use the LFO output as-is and let the
            // processor push the rate into mLFO each block.)

            // Width: blend between pure stereo (full phase offset via center delay
            // on one line) and mono (identical delays).
            const float widthMix = juce::jlimit(0.f, 1.f, mUserWidth);

            // Depth scaling: user depth multiplier (1.0 = native BBD depth)
            const float depthScale = juce::jlimit(0.f, 2.f, mUserDepthMul);

            float lfoL = lfo * (1.f + widthMix);
            float lfoR = lfo * (1.f - widthMix);

            float delay0Ms = (kCenterDelayMs + mDelayDepth * depthScale * lfoL) * (1.f - kBBDClockTrim);
            float delay1Ms = (kCenterDelayMs - mDelayDepth * depthScale * lfoR) * (1.f + kBBDClockTrim);
            delay0Ms = std::max(delay0Ms, kMinDelayMs);
            delay1Ms = std::max(delay1Ms, kMinDelayMs);
            float delay0samp = delay0Ms * 0.001f * mSampleRate;
            float delay1samp = delay1Ms * 0.001f * mSampleRate;
            float clock0 = std::max((float)BBDLine::kNumStages / (2.f * delay0Ms * 0.001f), kMinClockHz);
            float clock1 = std::max((float)BBDLine::kNumStages / (2.f * delay1Ms * 0.001f), kMinClockHz);

            static constexpr float kLeakMinFrac = 0.0126f;
            float invDepth = (mDelayDepth > 1e-9f) ? 1.f / mDelayDepth : 0.f;
            float lfo0 = (delay0Ms - kCenterDelayMs) * invDepth;
            float lfo1 = (delay1Ms - kCenterDelayMs) * invDepth;
            float lfoNorm0 = (lfo0 + 1.f) * 0.5f;
            float lfoNorm1 = (lfo1 + 1.f) * 0.5f;
            float leakAmount0 = mDelayDepth * (kLeakMinFrac + (1.f - kLeakMinFrac) * lfoNorm0);
            float leakAmount1 = mDelayDepth * (kLeakMinFrac + (1.f - kLeakMinFrac) * lfoNorm1);

            float noiseN0 = mLeakNoise0.Process();
            float noiseN1 = mLeakNoise1.Process();
            float wetPink0 = mWetPink0.Process() * analog_noise::kWetBroadbandGain * mAnalogMul;
            float wetPink1 = mWetPink1.Process() * analog_noise::kWetBroadbandGain * mAnalogMul;

            float click0Pri = mClick0.Process(-lfo) * kBBDClickGain * mGainModScale;
            float click1Pri = mClick1.Process(lfo) * kBBDClickGain * mGainModScale;
            float click0Slo = mSlowClick0.Process(lfo) * kBBDSlowClickGain * mGainModScale;
            float click1Slo = mSlowClick1.Process(-lfo) * kBBDSlowClickGain * mGainModScale;

            float bbdIn0 = (wetPink0 + noiseN0 * kBBDLeakageGain * leakAmount0
                + click0Pri - click0Slo) * mClockMul;
            float bbdIn1 = (wetPink1 + noiseN1 * kBBDLeakageGain * leakAmount1
                + click1Pri - click1Slo) * mClockMul;

            float wet0 = mLine0.Process(input, delay0samp, bbdIn0);
            float wet1 = mLine1.Process(input, delay1samp, bbdIn1);

            float gain0 = (1.f + kBBDGainTrim) * (1.f - kBBDCTELossCoeff * (1.f / clock0 - kBBDInvClockCenter));
            float gain1 = (1.f - kBBDGainTrim) * (1.f - kBBDCTELossCoeff * (1.f / clock1 - kBBDInvClockCenter));
            wet0 *= gain0;
            wet1 *= gain1;

            wet0 += mClickRing0.Process(click0Pri) * kClickRingGain * mClockMul;
            wet1 += mClickRing1.Process(click1Pri) * kClickRingGain * mClockMul;

            const float ripple = mWetRipple.Process() * mMainsMul;
            wet0 += ripple;
            wet1 += ripple;

            // --- Wet-path HPF (user) ---
            if (mHpfCoeff > 0.f)
            {
                mHpfZL += mHpfCoeff * (wet0 - mHpfZL);
                mHpfZR += mHpfCoeff * (wet1 - mHpfZR);
                wet0 -= mHpfZL;
                wet1 -= mHpfZR;
            }

            // --- User mix (blend dry into the chorus output) ---
            const float dryMix = 1.f - mUserMix;
            const float wetMix = mUserMix;

            // BBD fade + dry/wet
            float dryMixBBD = 1.f - mFade * (1.f - kDryGain);
            float wetMixBBD = mFade * kWetGain;
            float chL = dryMixBBD * input + wetMixBBD * wet0;
            float chR = dryMixBBD * input + wetMixBBD * wet1;

            // Outer user mix
            outL = input * dryMix + chL * wetMix;
            outR = input * dryMix + chR * wetMix;
        }

    private:
        void ConfigureMode()
        {
            switch (mMode)
            {
            case 1:
                mLFO.SetRate(kChorusIRate * mUserRateMul, mSampleRate);
                mTargetDelayDepth = kChorusIDelayDepthMs;
                mGainModScale = 1.f;
                mUseSine = false;
                break;
            case 2:
                mLFO.SetRate(kChorusIIRate * mUserRateMul, mSampleRate);
                mTargetDelayDepth = kChorusIIDelayDepthMs;
                mGainModScale = kChorusIIDelayDepthMs / kChorusIDelayDepthMs;
                mUseSine = false;
                break;
            case 3:
                mLFO.SetRate(kChorusI_IIRate * mUserRateMul, mSampleRate);
                mTargetDelayDepth = kChorusI_IIDelayDepthMs;
                mGainModScale = kChorusI_IIDelayDepthMs / kChorusIDelayDepthMs;
                mUseSine = true;
                break;
            }
        }
    };

} // namespace kr106

//==============================================================================
// Parameter IDs
//==============================================================================
namespace ChainChorusParams
{
    static constexpr const char* MODE = "mode";                 // 0=Off, 1=I, 2=II, 3=I+II
    static constexpr const char* RATE = "rate";                 // 0..1 rate multiplier
    static constexpr const char* DEPTH = "depth";               // 0..1 depth multiplier
    static constexpr const char* WIDTH = "width";               // 0..1 stereo width
    static constexpr const char* MIX = "mix";                   // 0..1 wet mix
    static constexpr const char* WET_HPF = "wet_hpf";           // Hz, 0 = off
    static constexpr const char* ANALOG = "analog";             // 0..1 analog noise amount
    static constexpr const char* MAINS = "mains";               // 0..1 mains ripple amount
    static constexpr const char* CLOCK = "clock";               // 0..1 clock drift amount
    static constexpr const char* BBD_DRIVE = "bbd_drive";       // 0..1 BBD saturation
    static constexpr const char* PINK = "pink";                 // 0..1 pink noise toggle
    static constexpr const char* OUTPUT = "output";             // dB
    static constexpr const char* BYPASS = "bypass";             // bool
}

//==============================================================================
// Processor
//==============================================================================
class ChainChorusAudioProcessor : public juce::AudioProcessor,
    public juce::AudioProcessorValueTreeState::Listener
{
public:
    ChainChorusAudioProcessor();
    ~ChainChorusAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "ChainChorus"; }
    bool acceptsMidi()  const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 4.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    void parameterChanged(const juce::String& paramID, float newValue) override;

    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // A/B snapshot (for editor)
    void captureAState();
    void swapAB();
    bool hasAState() const { return aStateValid; }

    // Output meter (thread-safe)
    std::atomic<float> levelPeakL{ 0.0f };
    std::atomic<float> levelPeakR{ 0.0f };

    //==========================================================================
// Preset manager
//==========================================================================
    PresetManager& getPresetManager() { return *presetManager; }
    const PresetManager& getPresetManager() const { return *presetManager; }

    // Preset navigation (used by editor arrow buttons)
    void loadPreviousPreset();
    void loadNextPreset();
    void loadPresetByIndex(int index);
    int  getCurrentPresetIndex() const { return currentPresetIndex; }

    // Editor hook: called after a preset loads (including via arrows)
    std::function<void(int)> onPresetChanged;

private:
    kr106::Chorus chorus;
    double currentSampleRate = 44100.0;
    int    currentBlockSize = 512;
    bool   isPrepared = false;

    // Wet-path HPF
    float  wetHpfCoeff = 0.f;

    // Parameter smoothing
    juce::SmoothedValue<float> smoothedMix;
    juce::SmoothedValue<float> smoothedOutput;

    // A/B snapshot
    juce::ValueTree aState;
    bool aStateValid = false;

    void updateMode(int mode);
    void updateChorusParams();

    std::unique_ptr<PresetManager> presetManager;
    int currentPresetIndex = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChainChorusAudioProcessor)
};