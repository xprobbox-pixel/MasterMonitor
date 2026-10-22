#include "Editor.h"
#include <iostream>
int main(int argc,char** argv){
 juce::ScopedJuceInitialiser_GUI gui;MasterProcessor p;juce::File dir(argc>1?argv[1]:".");dir.createDirectory();
 auto save=[&](juce::AudioProcessorEditor& editor,const juce::String& name){auto image=editor.createComponentSnapshot(editor.getLocalBounds(),true,1);juce::FileOutputStream stream(dir.getChildFile(name));stream.setPosition(0);stream.truncate();juce::PNGImageFormat png;png.writeImageToStream(image,stream);};
 {auto editor=std::unique_ptr<juce::AudioProcessorEditor>(p.createEditor());editor->setSize(1280,960);save(*editor,"UI-startup.png");}
 // These screenshots show actual analyser output from a documented generated fixture.
 p.prepareToPlay(48000,256);juce::MidiBuffer midi;juce::AudioBuffer<float>b(2,256);
 for(int block=0;block<600;++block){for(int i=0;i<256;++i){double t=double(block*256+i)/48000;float l=.17f*float(std::sin(juce::MathConstants<double>::twoPi*1000*t))+.05f*float(std::sin(juce::MathConstants<double>::twoPi*8000*t));float r=.16f*float(std::sin(juce::MathConstants<double>::twoPi*1000*t+.4))+.04f*float(std::sin(juce::MathConstants<double>::twoPi*400*t));b.setSample(0,i,l);b.setSample(1,i,r);}p.processBlock(b,midi);}
 for(int i=0;i<2000&&p.analysis.snapshot().seconds<3.199;++i)std::this_thread::sleep_for(std::chrono::milliseconds(5));
 auto editor=std::unique_ptr<juce::AudioProcessorEditor>(p.createEditor());
 for(auto size:{juce::Point<int>{1280,960},{1024,768}}){editor->setSize(size.x,size.y);save(*editor,"UI-"+juce::String(size.x)+".png");}
 auto* canvas=editor->findChildWithID("scaledControls");auto* menu=canvas?dynamic_cast<juce::ComboBox*>(canvas->findChildWithID("uiScale")):nullptr;
 if(!menu)return 2;
 for(int percent:{50,80,100,120}){menu->setSelectedId(percent,juce::sendNotificationSync);save(*editor,"UI-scale-"+juce::String(percent)+".png");}
 p.releaseResources();std::cout<<"Rendered actual editor: startup and 1280/1024 synthetic-signal snapshots\n";
}
