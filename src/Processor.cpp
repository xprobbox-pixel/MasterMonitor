#include "Processor.h"
#include "Editor.h"
using namespace juce;
AudioProcessorValueTreeState::ParameterLayout MasterProcessor::layout(){
 AudioProcessorValueTreeState::ParameterLayout p;
 auto add=[&](const char* id,const char* name,float lo,float hi,float def,const char* unit,bool automated){
  auto attributes=AudioParameterFloatAttributes().withLabel(unit).withAutomatable(automated)
   .withStringFromValueFunction([](float v,int){return String(v,1);});
  p.add(std::make_unique<AudioParameterFloat>(ParameterID{id,1},name,NormalisableRange<float>{lo,hi,0.1f},def,attributes));};
 add("trim","Bus Trim",-24,12,0,"dB",true);
 p.add(std::make_unique<AudioParameterBool>(ParameterID{"dim",1},"Dim Output -20 dB",false));
 p.add(std::make_unique<AudioParameterBool>(ParameterID{"mono",1},"Mono Sum Output",false));
 p.add(std::make_unique<AudioParameterBool>(ParameterID{"active",1},"Active",true));
 add("target","Loudness Target",-24,-6,-14,"LUFS",false);add("limit","True-Peak Limit",-6,0,-1,"dBTP",false);
 add("calibration","VU Calibration",-24,-12,-18,"dBFS",false);add("scopeGain","Scope Display Gain",0.1f,8,1,"x",false);
 add("timebase","Scope Timebase",5,100,20,"ms",false);add("zoom","Display Zoom",1,2,1,"x",false);
 p.add(std::make_unique<AudioParameterBool>(ParameterID{"freeze",1},"Freeze Scope",false,AudioParameterBoolAttributes().withAutomatable(false)));
 return p;
}
MasterProcessor::MasterProcessor():AudioProcessor(BusesProperties().withInput("Input",AudioChannelSet::stereo(),true).withOutput("Output",AudioChannelSet::stereo(),true)),state(*this,nullptr,"MasterMonitor",layout()){
 const char* ids[]{"trim","dim","mono","active","limit","scopeGain","timebase","freeze"};for(size_t i=0;i<audioParams.size();++i)audioParams[i]=state.getRawParameterValue(ids[i]);setLatencySamples(0);
 PropertiesFile::Options options;options.applicationName="MASTER MONITOR";options.filenameSuffix="settings";options.folderName="MasterMonitor";options.osxLibrarySubFolder="Application Support";
 if(SystemStats::getEnvironmentVariable("MM_NO_USER_SETTINGS","")=="1")return;
 preferences=std::make_unique<PropertiesFile>(options);
 uiWidth=jlimit(1024,1920,preferences->getIntValue("width",1280));uiHeight=jlimit(768,1440,preferences->getIntValue("height",960));
 for(const char* id:{"target","limit","calibration","scopeGain","timebase","zoom","freeze"})if(preferences->containsKey(id)){float v=float(preferences->getDoubleValue(id));auto* parameter=state.getParameter(id);if(std::isfinite(v))parameter->setValueNotifyingHost(parameter->convertTo0to1(jlimit(parameter->getNormalisableRange().start,parameter->getNormalisableRange().end,v)));}
}
MasterProcessor::~MasterProcessor(){analysis.stop();if(!preferences)return;preferences->setValue("width",uiWidth.load());preferences->setValue("height",uiHeight.load());for(const char* id:{"target","limit","calibration","scopeGain","timebase","zoom","freeze"})preferences->setValue(id,value(id));preferences->saveIfNeeded();}
bool MasterProcessor::isBusesLayoutSupported(const BusesLayout& b)const {auto in=b.getMainInputChannelSet(),out=b.getMainOutputChannelSet();return in==out&&(in==AudioChannelSet::mono()||in==AudioChannelSet::stereo());}
void MasterProcessor::prepareToPlay(double sr,int){trim.reset(sr,0.02);dim.reset(sr,0.02);mono.reset(sr,0.02);trim.setCurrentAndTargetValue(Decibels::decibelsToGain(value("trim")));dim.setCurrentAndTargetValue(value("dim")>0.5f?0.1f:1);mono.setCurrentAndTargetValue(value("mono"));analysis.start(sr,getTotalNumInputChannels());wasMeasuring=false;}
void MasterProcessor::processBlockBypassed(AudioBuffer<float>& b,MidiBuffer&){ScopedNoDenormals noDenormals;measurementPaused=true;for(int c=0;c<b.getNumChannels();++c)for(int i=0;i<b.getNumSamples();++i)if(!std::isfinite(b.getSample(c,i)))b.setSample(c,i,0);if(wasMeasuring){analysis.discontinuity();wasMeasuring=false;}}
void MasterProcessor::processBlock(AudioBuffer<float>& b,MidiBuffer& midi){
 ScopedNoDenormals noDenormals;
 if(audioParams[3]->load()<0.5f){processBlockBypassed(b,midi);return;}
 const int channels=b.getNumChannels();if(channels<1)return;
 trim.setTargetValue(Decibels::decibelsToGain(audioParams[0]->load()));dim.setTargetValue(audioParams[1]->load()>0.5f?0.1f:1);mono.setTargetValue(audioParams[2]->load());
 analysis.limit=audioParams[4]->load();analysis.gain=audioParams[5]->load();analysis.timebase=audioParams[6]->load();analysis.freeze=audioParams[7]->load()>0.5f;
 wasMeasuring=true;measurementPaused=false;
 float* l=b.getWritePointer(0);float* r=channels>1?b.getWritePointer(1):nullptr;
 for(int i=0;i<b.getNumSamples();++i){float g=trim.getNextValue(),d=dim.getNextValue(),m=mono.getNextValue();float a=std::isfinite(l[i])?l[i]*g:0,z=r?(std::isfinite(r[i])?r[i]*g:0):a;
  analysis.push(a,z);float centre=0.5f*(a+z);l[i]=(a+m*(centre-a))*d;if(r)r[i]=(z+m*(centre-z))*d;
 }
}
void MasterProcessor::getStateInformation(MemoryBlock& b){auto tree=state.copyState();tree.setProperty("schema",1,nullptr);tree.setProperty("width",uiWidth.load(),nullptr);tree.setProperty("height",uiHeight.load(),nullptr);auto xml=tree.createXml();copyXmlToBinary(*xml,b);}
void MasterProcessor::setStateInformation(const void* bytes,int size){
 auto xml=getXmlFromBinary(bytes,size);if(!xml||!xml->hasTagName("MasterMonitor"))return;auto tree=ValueTree::fromXml(*xml);if(int(tree.getProperty("schema",1))!=1)return;
 for(auto child:tree){auto id=child.getProperty("id").toString();if(auto* p=state.getParameter(id)){float v=float(child.getProperty("value"));if(std::isfinite(v))p->setValueNotifyingHost(p->convertTo0to1(jlimit(p->getNormalisableRange().start,p->getNormalisableRange().end,v)));}}
 uiWidth=jlimit(1024,1920,int(tree.getProperty("width",1280)));uiHeight=jlimit(768,1440,int(tree.getProperty("height",960)));
}
AudioProcessorEditor* MasterProcessor::createEditor(){return new MasterEditor(*this);}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new MasterProcessor();}
