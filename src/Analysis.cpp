#include "Analysis.h"
#include <chrono>
namespace mm {
Analyzer::Analyzer(){for(size_t i=0;i<window.size();++i)window[i]=float(0.5-0.5*std::cos(juce::MathConstants<double>::twoPi*i/4096.0));prepare(48000);}
Analyzer::~Analyzer(){if(ebur)ebur128_destroy(&ebur);}
void Analyzer::prepare(double sr,int channels){rate=sr;channelCount=channels;historyFrames=std::max(1,int(rate/50));reset();}
void Analyzer::reset(){
 if(ebur)ebur128_destroy(&ebur);
 ebur=ebur128_init(2,(unsigned long)rate,EBUR128_MODE_S|EBUR128_MODE_I|EBUR128_MODE_TRUE_PEAK|EBUR128_MODE_HISTOGRAM);
 if(ebur && channelCount==1)ebur128_set_channel(ebur,1,EBUR128_UNUSED);
 result=Snapshot{};result.healthy=ebur!=nullptr; result.spectrum.fill(-90);result.spectrumHold.fill(-90);
 samples=0;vuEnergy={};ll=rr=lr=mid=side=0;fftPos=ringPos=scopePos=fieldPos=historyPos=historyCount=0;scopeClock=0;historyLow=historyHigh=0;
 for(auto& a:fftIn)a.fill(0);ringL.fill(0);ringR.fill(0);
}
void Analyzer::resetPeaks(){result.hold={silence,silence};result.clip=false;result.truePeak=silence;result.tpExceeded=false;result.spectrumHold.fill(-90);}
void Analyzer::consume(const float* input,size_t n){
 if(!ebur)return;
 if(ebur128_add_frames_float(ebur,input,n)!=EBUR128_SUCCESS){result.healthy=false;return;}
 const double vuA=std::exp(-1.0/(rate*0.065)), corrA=std::exp(-1.0/(rate*0.3));
 std::array<float,2> pk{};
 for(size_t i=0;i<n;++i){
  const float l=input[2*i],r=input[2*i+1];
  pk[0]=std::max(pk[0],std::abs(l));pk[1]=std::max(pk[1],std::abs(r));
  vuEnergy[0]=vuA*vuEnergy[0]+(1-vuA)*l*l;vuEnergy[1]=vuA*vuEnergy[1]+(1-vuA)*r*r;
  ll=corrA*ll+(1-corrA)*l*l;rr=corrA*rr+(1-corrA)*r*r;lr=corrA*lr+(1-corrA)*l*r;
  mid=corrA*mid+(1-corrA)*0.25*(l+r)*(l+r);side=corrA*side+(1-corrA)*0.25*(l-r)*(l-r);
  fftIn[0][size_t(fftPos)]=l;fftIn[1][size_t(fftPos)]=r;if(++fftPos==4096){spectrum();fftPos=0;}
  ringL[size_t(ringPos)]=l;ringR[size_t(ringPos)]=r;ringPos=(ringPos+1)%16384;
  if(!freeze && (++scopeClock%17)==0){result.fieldL[size_t(fieldPos)]=l;result.fieldR[size_t(fieldPos)]=r;fieldPos=(fieldPos+1)%256;}
  historyLow=std::min({historyLow,l,r});historyHigh=std::max({historyHigh,l,r});
  if(++historyCount>=historyFrames){
   // 50 bins/s, preserving both extremes. Linear order in published view.
   std::move(result.historyMin.begin()+1,result.historyMin.end(),result.historyMin.begin());
   std::move(result.historyMax.begin()+1,result.historyMax.end(),result.historyMax.begin());
   result.historyMin.back()=historyLow;result.historyMax.back()=historyHigh;historyCount=0;historyLow=historyHigh=0;
  }
 }
 samples+=n;
 for(size_t c=0;c<2;++c){result.peak[c]=std::max(db(pk[c]),result.peak[c]-float(24.0*n/rate));result.hold[c]=std::max(result.hold[c],db(pk[c]));result.vu[c]=db(std::sqrt(vuEnergy[c]*2));}
 result.clip=result.clip||pk[0]>=1||pk[1]>=1;
 double tp=0;for(unsigned c=0;c<2;++c){double p=0;ebur128_prev_true_peak(ebur,c,&p);tp=std::max(tp,p);}
 result.truePeak=std::max(result.truePeak,db(tp));result.tpExceeded=result.tpExceeded||db(tp)>tpLimit;
 result.signal=(ll+rr)>1e-12;
 if(ll>1e-12&&rr>1e-12)result.correlation=float(juce::jlimit(-1.0,1.0,lr/std::sqrt(ll*rr)));else result.correlation=unavailable;
 result.width=mid+side>1e-12?float(200*side/(mid+side)):unavailable;
 result.seconds=double(samples)/rate;
 if(samples>=uint64_t(rate*3)){double x;result.shortLU=ebur128_loudness_shortterm(ebur,&x)==0?float(x):unavailable;}
 if(samples>=uint64_t(rate*0.4)){double x;result.integrated=ebur128_loudness_global(ebur,&x)==0?float(x):unavailable;}
}
void Analyzer::spectrum(){
 constexpr std::array<double,16> centres{31,63,125,250,400,630,1000,1600,2500,4000,6300,8000,10000,12000,16000,20000};
 std::array<double,16> energies{};
 for(int c=0;c<2;++c){
  fftWork.fill(0);for(int i=0;i<4096;++i)fftWork[size_t(i)]=fftIn[size_t(c)][size_t(i)]*window[size_t(i)];
  fft.performFrequencyOnlyForwardTransform(fftWork.data());
  for(int b=0;b<16;++b){double low=b==0?20:std::sqrt(centres[size_t(b-1)]*centres[size_t(b)]), high=b==15?22050:std::sqrt(centres[size_t(b)]*centres[size_t(b+1)]);
   result.bandAvailable[size_t(b)]=centres[size_t(b)]<rate/2;
   if(!result.bandAvailable[size_t(b)])continue;
   double power=0;for(int k=1;k<=2048;++k)if(k*rate/4096>=low&&k*rate/4096<std::min(high,rate/2))power+=double(fftWork[size_t(k)])*fftWork[size_t(k)];
   energies[size_t(b)]+=power/(4096.0*4096.0*0.375); // average stereo power, Hann ENBW corrected
  }
 }
 for(size_t b=0;b<16;++b){float value=db(std::sqrt(energies[b])); result.spectrum[b]=std::max(value,result.spectrum[b]-float(20*4096/rate));result.spectrumHold[b]=std::max(result.spectrumHold[b],value);}
}
void Analyzer::scope(){
 if(freeze)return;
 int count=juce::jlimit(32,16000,int(rate*timebaseMs/1000));int start=(ringPos-count+16384)%16384;
 // rising zero trigger on L, bounded to a preceding timebase.
 for(int i=0;i<count/2;++i){int a=(start-i+16384)%16384,b=(a+1)%16384;if(ringL[size_t(a)]<=0&&ringL[size_t(b)]>0){start=b;break;}}
 for(int p=0;p<512;++p){int begin=p*count/512,end=std::max(begin+1,(p+1)*count/512);float loL=1e20f,hiL=-1e20f,loR=1e20f,hiR=-1e20f;
  for(int k=begin;k<end;++k){auto idx=size_t((start+k)%16384);loL=std::min(loL,ringL[idx]);hiL=std::max(hiL,ringL[idx]);loR=std::min(loR,ringR[idx]);hiR=std::max(hiR,ringR[idx]);}
  result.scopeL[size_t(2*p)]=loL;result.scopeL[size_t(2*p+1)]=hiL;result.scopeR[size_t(2*p)]=loR;result.scopeR[size_t(2*p+1)]=hiR;
 }
}
void Analyzer::finish(){scope();}
void AnalysisService::start(double rate,int channels){stop();queue.clear();dropped=0;{std::lock_guard<std::mutex> l(snapshotMutex);latest=Snapshot{};}running=true;worker=std::thread([this,rate,channels]{run(rate,channels);});}
void AnalysisService::stop(){running=false;if(worker.joinable())worker.join();}
void AnalysisService::run(double rate,int channels){
 Analyzer a;a.prepare(rate,channels);std::array<float,512> buffer{};unsigned lastReset=resetRequest.load(),lastPeak=peakRequest.load();uint32_t lastEpoch=epoch.load();
 while(running.load()){
  bool dirty=false;
  if(lastReset!=resetRequest.load()){lastReset=resetRequest.load();a.reset();dirty=true;}
  if(lastPeak!=peakRequest.load()){lastPeak=peakRequest.load();a.resetPeaks();dirty=true;}
  a.tpLimit=limit.load();a.scopeGain=gain.load();a.timebaseMs=timebase.load();a.freeze=freeze.load();
  Frame f;size_t n=0;
  while(n<256&&queue.pop(f)){if(f.epoch!=lastEpoch){if(n){a.consume(buffer.data(),n);n=0;}lastEpoch=f.epoch;a.reset();}buffer[2*n]=f.l;buffer[2*n+1]=f.r;++n;}
  if(n){a.consume(buffer.data(),n);a.finish();}
  if(n||dirty){a.result.dropped=dropped.load();std::lock_guard<std::mutex> l(snapshotMutex);latest=a.result;}
  else std::this_thread::sleep_for(std::chrono::milliseconds(2));
 }
}
}
