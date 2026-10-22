#pragma once
#include <juce_dsp/juce_dsp.h>
#include <ebur128.h>
#include <array>
#include <atomic>
#include <mutex>
#include <thread>
#include <limits>
namespace mm {
static_assert(std::atomic<uint32_t>::is_always_lock_free && std::atomic<uint64_t>::is_always_lock_free && std::atomic<float>::is_always_lock_free, "Real-time queue requires lock-free atomics");
constexpr float silence = -std::numeric_limits<float>::infinity();
constexpr float unavailable = std::numeric_limits<float>::quiet_NaN();
inline float db(double v) { return v > 0 ? float(20 * std::log10(v)) : silence; }
struct Frame { float l{}, r{}; uint32_t epoch{}; };
// One producer (audio), one consumer (analysis). Publish only complete frames.
class Queue {
 static constexpr uint32_t capacity=262144, mask=capacity-1;
 std::array<Frame,capacity> data{};
 alignas(64) std::atomic<uint32_t> write{0};
 alignas(64) std::atomic<uint32_t> read{0};
public:
 bool push(Frame f) noexcept { auto w=write.load(std::memory_order_relaxed); if(w-read.load(std::memory_order_acquire)==capacity) return false; data[w&mask]=f; write.store(w+1,std::memory_order_release); return true; }
 bool pop(Frame& f) noexcept {auto r=read.load(std::memory_order_relaxed); if(r==write.load(std::memory_order_acquire))return false;f=data[r&mask];read.store(r+1,std::memory_order_release);return true;}
 void clear() {read.store(write.load());} // only when producer/consumer stopped
};
struct Snapshot {
 float shortLU=unavailable, integrated=unavailable, truePeak=silence;
 std::array<float,2> peak{silence,silence}, hold{silence,silence}, vu{silence,silence};
 bool clip=false, tpExceeded=false, signal=false, healthy=true;
 float correlation=unavailable,width=unavailable;
 double seconds=0;
 uint64_t dropped=0;
 std::array<float,16> spectrum{}, spectrumHold{};
 std::array<bool,16> bandAvailable{};
 std::array<float,1024> scopeL{},scopeR{};
 std::array<float,256> fieldL{},fieldR{};
 std::array<float,500> historyMin{},historyMax{};
};
class Analyzer {
 ebur128_state* ebur=nullptr;
 double rate=48000; int channelCount=2; uint64_t samples=0;
 std::array<double,2> vuEnergy{};
 double ll=0,rr=0,lr=0, mid=0,side=0;
 juce::dsp::FFT fft{12};
 std::array<std::array<float,4096>,2> fftIn{};
 std::array<float,8192> fftWork{};
 std::array<float,4096> window{};
 int fftPos=0,scopePos=0,fieldPos=0,historyPos=0,historyCount=0,historyFrames=960;
 float historyLow=0,historyHigh=0;
 std::array<float,16384> ringL{},ringR{}; int ringPos=0;
 uint64_t scopeClock=0;
 void spectrum(); void scope();
public:
 Snapshot result;
 float tpLimit=-1, scopeGain=1, timebaseMs=20; bool freeze=false;
 Analyzer(); ~Analyzer();
 void prepare(double sampleRate,int channels=2); void reset(); void resetPeaks();
 void consume(const float* interleaved,size_t frames);
 void finish();
};
class AnalysisService {
 Queue queue;
 std::thread worker;
 std::atomic<bool> running{false};
 std::atomic<unsigned> resetRequest{0},peakRequest{0};
 std::atomic<uint64_t> dropped{0};
 std::atomic<uint32_t> epoch{0};
 std::mutex snapshotMutex;
 Snapshot latest;
 void run(double rate,int channels);
public:
 std::atomic<float> limit{-1},gain{1},timebase{20}; std::atomic<bool> freeze{false};
 ~AnalysisService(){stop();}
 void start(double rate,int channels=2); void stop();
 void push(float l,float r) noexcept {if(!queue.push({l,r,epoch.load(std::memory_order_relaxed)})){dropped.fetch_add(1,std::memory_order_relaxed); epoch.fetch_add(1,std::memory_order_relaxed);}}
 void discontinuity() noexcept {epoch.fetch_add(1,std::memory_order_relaxed);}
 void reset(){resetRequest.fetch_add(1); discontinuity();}
 void resetPeaks(){peakRequest.fetch_add(1);}
 Snapshot snapshot(){std::lock_guard<std::mutex> lock(snapshotMutex);return latest;}
};
}
