#pragma once

#include "ofMain.h"
#include "ofxGui.h"
#include "ofxOpenCv.h"
#include "ofxOpenNI.h"
#include "ofxPd.h"
#include "ofxMidi.h"

#include "soundObject.h"
#include <Externals.h>

using namespace std;
using namespace pd;
class ofApp : public ofBaseApp, public PdReceiver, public PdMidiReceiver{

	public:
		void setup();
		void update();
		void draw();

		void keyPressed(int key);
		void keyReleased(int key);
		void mouseMoved(int x, int y );
		void mouseDragged(int x, int y, int button);
		void mousePressed(int x, int y, int button);
		void mouseReleased(int x, int y, int button);
		void mouseEntered(int x, int y);
		void mouseExited(int x, int y);
		void windowResized(int w, int h);
		void dragEvent(ofDragInfo dragInfo);
		void gotMessage(ofMessage msg);
    void exit();
  
    void print(const std::string& message);
		
  
    void audioReceived(float * input, int bufferSize, int nChannels);
		void audioRequested(float * output, int bufferSize, int nChannels);
  
    void receiveBang(const std::string& dest);
    void receiveFloat(const std::string& dest, float value);
		void receiveSymbol(const std::string& dest, const std::string& symbol);
		void receiveList(const std::string& dest, const List& list);
		void receiveMessage(const std::string& dest, const std::string& msg, const List& list);
  
    void receiveNoteOn(const int channel, const int pitch, const int velocity);
		void receiveControlChange(const int channel, const int controller, const int value);
  
  // kinect
  ofxOpenNI kinect;
  
  // openCV contourFinder-----------------------
  ofImage kirinuki;
  ofImage kirinuki_BnW;
  ofxCvColorImage colorImg;
  ofxCvGrayscaleImage grayImage;
  ofxCvGrayscaleImage grayBg;
  ofxCvGrayscaleImage grayDiff;
  ofxCvContourFinder contourFinder;

  // pd
  ofxPd pd;
  
  // midi
  ofxMidiOut midiOut;
  
  // gui: openCV
  ofxPanel gui;
  ofxIntSlider groundLevel;
  ofxIntSlider threshold;
  ofxFloatSlider minBlobSize;
  ofxFloatSlider maxBlobSize;
  ofxIntSlider maxNumBlobs;
  ofxToggle findHoles;
  ofxToggle useApproximation;
  
  // gui: sound
  ofxPanel sndGui;
  ofxToggle start_stop;
  ofxFloatSlider BPM;
  ofxIntSlider key;
  ofxButton reset;
  
  bool start_stop_old;
  bool changeKey_old;
  
  int guiWidth;
  bool getBang;
  int radius;
  
  vector <soundObject *> object;
  vector <Patch> sndObject; // SndOをインスタンスとして読み込ませる
  
};
