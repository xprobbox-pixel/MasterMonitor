#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "Analysis.h"
class MasterProcessor final : public juce::AudioProcessor {
public:
 MasterProcessor(); ~MasterProcessor() override;
 juce::AudioProcessorValueTreeState state;
 mm::AnalysisService analysis;
 std::atomic<bool> measurementPaused{true};
 std::atomic<int> uiWidth{1280},uiHeight{960};
 void prepareToPlay(double,int) override;
 void releaseResources() override {analysis.stop();}
 void processBlock(juce::AudioBuffer<float>&,juce::MidiBuffer&) override;
 void processBlockBypassed(juce::AudioBuffer<float>&,juce::MidiBuffer&) override;
 bool isBusesLayoutSupported(const BusesLayout&)const override;
 juce::AudioProcessorEditor* createEditor() override;
 bool hasEditor()const override{return true;}
 const juce::String getName()const override{return "MASTER MONITOR";}
 bool acceptsMidi()const override{return false;} bool producesMidi()const override{return false;} bool isMidiEffect()const override{return false;}
 double getTailLengthSeconds()const override{return 0;} int getNumPrograms()override{return 1;} int getCurrentProgram()override{return 0;}
 void setCurrentProgram(int)override{} const juce::String getProgramName(int)override{return {}; } void changeProgramName(int,const juce::String&)override{}
 void getStateInformation(juce::MemoryBlock&)override;void setStateInformation(const void*,int)override;
 float value(const char* id)const {return state.getRawParameterValue(id)->load();}
 void set(const char* id,float v){auto* p=state.getParameter(id);p->setValueNotifyingHost(p->convertTo0to1(v));}
 static juce::AudioProcessorValueTreeState::ParameterLayout layout();
private:
 std::array<std::atomic<float>*,8> audioParams{};
 juce::SmoothedValue<float> trim,dim,mono;
 bool wasMeasuring=false;
 std::unique_ptr<juce::PropertiesFile> preferences;
};
