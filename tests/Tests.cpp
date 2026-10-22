#include "Processor.h"
#include <iostream>
#include <random>
#include <fstream>
int failures=0,checks=0;
void check(bool okay,const char* name){++checks;std::cout<<(okay?"PASS ":"FAIL ")<<name<<'\n';if(!okay)++failures;}
void near(double value,double expected,double tolerance,const char* name){std::cout<<"  actual="<<value<<" expected="<<expected<<" tolerance="<<tolerance<<'\n';check(std::isfinite(value)&&std::abs(value-expected)<=tolerance,name);}
void feed(mm::Analyzer& a,double sr,double seconds,float amp,float relation=1,double frequency=1000){std::array<float,512> buf{};size_t total=size_t(seconds*sr);for(size_t pos=0;pos<total;){size_t n=std::min<size_t>(256,total-pos);for(size_t i=0;i<n;++i){float x=amp*float(std::sin(juce::MathConstants<double>::twoPi*frequency*(pos+i)/sr));buf[2*i]=x;buf[2*i+1]=relation*x;}a.consume(buf.data(),n);pos+=n;}a.finish();}
void routing(){
 for(double sr:{44100.,48000.,96000.,192000.})for(int block:{1,17,256,1024,8192}){
  MasterProcessor p;p.prepareToPlay(sr,block);juce::MidiBuffer midi;juce::AudioBuffer<float> b(2,block);
  auto fill=[&](float l,float r){for(int i=0;i<block;++i){b.setSample(0,i,l);b.setSample(1,i,r);}};
  fill(.25f,-.5f);p.processBlock(b,midi);check(b.getSample(0,block-1)==.25f&&b.getSample(1,block-1)==-.5f,"unity dry pass-through / sample rate / block size");
  p.set("trim",6);for(int k=0;k<int(sr*.03/block)+2;++k){fill(.25f,-.5f);p.processBlock(b,midi);}near(b.getSample(0,block-1),.25*std::pow(10.,6./20),1e-5,"trim settled gain");
  p.set("dim",1);for(int k=0;k<int(sr*.03/block)+2;++k){fill(.25f,-.5f);p.processBlock(b,midi);}near(b.getSample(0,block-1),.025*std::pow(10.,6./20),1e-5,"Dim exactly -20 dB");
  p.set("mono",1);for(int k=0;k<int(sr*.03/block)+2;++k){fill(.25f,-.25f);p.processBlock(b,midi);}near(b.getSample(0,block-1),0,1e-7,"anti-phase mono cancellation");
  fill(.2f,.4f);p.processBlockBypassed(b,midi);check(b.getSample(0,0)==.2f&&b.getSample(1,0)==.4f,"host bypass ignores trim / monitoring");
  p.set("active",0);fill(.2f,.4f);p.processBlock(b,midi);check(b.getSample(0,0)==.2f&&b.getSample(1,0)==.4f,"Active off unity");
  fill(std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::infinity());p.processBlock(b,midi);check(b.getSample(0,0)==0&&b.getSample(1,0)==0,"NaN / Inf sanitised");p.releaseResources();
 }
 MasterProcessor p;p.prepareToPlay(48000,256);juce::AudioBuffer<float>b(2,256);juce::MidiBuffer midi;p.set("trim",12);float previous=.1f,maxStep=0;for(int block=0;block<6;++block){for(int i=0;i<256;++i){b.setSample(0,i,.1f);b.setSample(1,i,.1f);}p.processBlock(b,midi);for(int i=0;i<256;++i){maxStep=std::max(maxStep,std::abs(b.getSample(0,i)-previous));previous=b.getSample(0,i);}}check(maxStep<.001f,"20 ms trim ramp has bounded transitions");p.releaseResources();
}
void meters(){
 for(double sr:{32000.,44100.,48000.,96000.,192000.}){
  mm::Analyzer a;a.prepare(sr);check(std::isnan(a.result.shortLU),"no fabricated startup LUFS");feed(a,sr,.2,0);check(std::isnan(a.result.integrated),"integrated unavailable before 400 ms");feed(a,sr,3.2,0);check(std::isinf(a.result.shortLU)&&a.result.shortLU<0,"silence loudness -infinity");check(std::isnan(a.result.correlation),"silence correlation unavailable");a.reset();
  feed(a,sr,4,std::pow(10.f,-23.f/20));near(a.result.integrated,-23,0.12,"stereo 1 kHz loudness analytic reference");near(a.result.shortLU,-23,.12,"3 second short-term loudness");near(a.result.peak[0],-23,.04,"sample peak dBFS");near(a.result.vu[0],-23,.05,"VU sine peak-equivalent calibration");near(a.result.correlation,1,.001,"identical correlation +1");near(a.result.width,0,.001,"mono width 0%");float loud=a.result.integrated;feed(a,sr,4,.00001f);near(a.result.integrated,loud,.2,"integrated absolute and relative gating reject quiet tail");
  a.reset();feed(a,sr,1,.5f,-1);near(a.result.correlation,-1,.001,"inverted correlation -1");near(a.result.width,200,.001,"anti-phase width 200%");check(a.result.spectrum[6]>-10,"anti-phase stereo does not cancel FFT");check(a.result.bandAvailable[15]==(sr>40000),"above-Nyquist bands unavailable");
  a.reset();feed(a,sr,.3,std::pow(10.f,-18.f/20));near(a.result.vu[0],-18,.12,"VU 300 ms sine rise near 0 VU at -18 calibration");
  a.reset();feed(a,sr,.2,1.2);check(a.result.clip,"sample clipping latches");check(a.result.tpExceeded,"true peak exceedance latches");a.resetPeaks();check(!a.result.clip&&!a.result.tpExceeded&&std::isinf(a.result.truePeak),"peak reset clears all latches");a.reset();check(a.result.seconds==0&&std::isnan(a.result.integrated),"measurement reset elapsed / loudness");
 }
 mm::Analyzer a;a.prepare(48000);std::array<float,512> buf{};for(int block=0;block<400;++block){for(int i=0;i<256;++i){float x=.98f*float(std::sin(juce::MathConstants<double>::twoPi*12000*(block*256+i)/48000+juce::MathConstants<double>::pi/4));buf[2*i]=buf[2*i+1]=x;}a.consume(buf.data(),256);}near(a.result.truePeak,mm::db(.98),.6,"inter-sample Fs/4 phase pi/4 true peak reconstruction");check(a.result.truePeak>a.result.hold[0]+2,"true peak exceeds original sample peak");
 a.reset();std::mt19937 rng(52);std::uniform_real_distribution<float>d(-.3f,.3f);for(int block=0;block<1000;++block){for(auto& x:buf)x=d(rng);a.consume(buf.data(),256);}near(a.result.correlation,0,.04,"uncorrelated stereo correlation");near(a.result.width,100,4,"uncorrelated width near 100%");check(a.result.spectrum[12]>-60,"noise spectrum measured");
 a.prepare(48000,1);feed(a,48000,4,std::pow(10.f,-23.f/20));near(a.result.integrated,-26.01,.12,"mono weighting is one channel, not duplicated stereo");
 a.prepare(48000);feed(a,48000,4,.5f,1,4000);auto max=std::max_element(a.result.spectrum.begin(),a.result.spectrum.end());check(std::distance(a.result.spectrum.begin(),max)==9,"4 kHz tone lands in correct band");
}
void communication(){
 auto queue=std::make_unique<mm::Queue>();for(uint32_t i=0;i<262144;++i)if(!queue->push({float(i),-float(i),7})){check(false,"queue fills to declared capacity");break;}
 check(!queue->push({1,2,8}),"bounded queue overflow rejected without overwrite");bool ordered=true;mm::Frame frame;for(uint32_t i=0;i<262144;++i){if(!queue->pop(frame)||frame.l!=float(i)||frame.epoch!=7)ordered=false;}check(ordered&&!queue->pop(frame),"SPSC ordering and empty state");
 auto p=std::make_unique<MasterProcessor>();p->set("trim",6);p->set("dim",1);p->set("mono",1);p->prepareToPlay(48000,256);juce::MidiBuffer midi;juce::AudioBuffer<float>b(2,256);bool cancelled=true;
 for(int block=0;block<600;++block){for(int i=0;i<256;++i){float x=.125f*float(std::sin(juce::MathConstants<double>::twoPi*1000*(block*256+i)/48000));b.setSample(0,i,x);b.setSample(1,i,-x);}p->processBlock(b,midi);for(int i=0;i<256;++i)if(std::abs(b.getSample(0,i))>1e-7f)cancelled=false;}
 check(cancelled,"Mono monitoring cancellation with trim and Dim");
 mm::Snapshot result;for(int i=0;i<1000;++i){result=p->analysis.snapshot();if(result.seconds>=3.2-1e-6)break;std::this_thread::sleep_for(std::chrono::milliseconds(5));}
 near(result.hold[0],mm::db(.125)+6,.01,"measurement tap includes trim but excludes Mono and Dim");check(std::isfinite(result.shortLU)&&result.dropped==0,"worker measures with editor closed");
 p->analysis.resetPeaks();for(int i=0;i<200;++i){if(std::isinf(p->analysis.snapshot().hold[0]))break;std::this_thread::sleep_for(std::chrono::milliseconds(5));}check(std::isinf(p->analysis.snapshot().hold[0]),"peak reset publishes while transport idle");
 p->analysis.reset();for(int i=0;i<200;++i){if(p->analysis.snapshot().seconds==0)break;std::this_thread::sleep_for(std::chrono::milliseconds(5));}check(p->analysis.snapshot().seconds==0,"measurement reset publishes while transport idle");p->releaseResources();
}
void state(){MasterProcessor p;p.set("trim",3.2f);p.set("target",-16.5f);p.set("limit",-2.1f);p.set("mono",1);p.uiWidth=1024;p.uiHeight=768;juce::MemoryBlock b;p.getStateInformation(b);MasterProcessor q;q.setStateInformation(b.getData(),int(b.getSize()));near(q.value("trim"),3.2,.001,"state audio parameter restore");near(q.value("target"),-16.5,.001,"state analysis preference restore");near(q.value("limit"),-2.1,.001,"state true peak limit restore");check(q.value("mono")==1&&q.uiWidth==1024,"state UI and monitoring restore");q.setStateInformation("invalid",7);near(q.value("trim"),3.2,.001,"invalid state rejected");check(p.getLatencySamples()==0,"zero reported latency");}
int main(int argc,char** argv){
 if(argc==4&&std::string(argv[1])=="--analyze"){
  double rate=std::stod(argv[3]);mm::Analyzer a;a.prepare(rate);std::ifstream file(argv[2],std::ios::binary);std::array<float,512> block{};while(file){file.read(reinterpret_cast<char*>(block.data()),sizeof(block));auto count=file.gcount()/sizeof(float)/2;if(count)a.consume(block.data(),size_t(count));}
  std::cout<<"{\"integrated\":"<<a.result.integrated<<",\"true_peak\":"<<a.result.truePeak<<"}\n";return 0;
 }
 juce::ScopedJuceInitialiser_GUI gui;routing();meters();communication();state();std::cout<<checks<<" checks; "<<failures<<" failures\n";return failures?1:0;}
