#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
  ofSetDataPathRoot("../Resources/data/");
  
  ofBackground(0, 0, 0);
  ofEnableAlphaBlending();
  ofSetFrameRate(60);
  
  ofSoundStreamSetup(2, 1, this, 44100, 2048, 4);
  
  //setup kinect
  kinect.setup();
  kinect.setRegister(true);
  kinect.setMirror(true);
  kinect.addDepthGenerator();
  kinect.addImageGenerator();
  //align depth image to RGB image
  kinect.getDepthGenerator().GetAlternativeViewPointCap().SetViewPoint(kinect.getImageGenerator());
  kinect.start();

  kirinuki.allocate(640, 480, OF_IMAGE_COLOR);
  kirinuki_BnW.allocate(640, 480, OF_IMAGE_GRAYSCALE);
  colorImg.allocate(640,480);
  grayImage.allocate(640,480);
  grayBg.allocate(640,480);
  grayDiff.allocate(640,480);
  
  // setup pd
  pd.init(2, 1, 44100);
  Patch patch = pd.openPatch("kinectSound4.pd");
  pd.start();
  pd.addReceiver(*this);
  pd.addMidiReceiver(*this);
  
  pd.subscribe("toOF");

  
  // setup midi
  midiOut.listPorts();
  midiOut.openPort(0);
  
  // setup gui
  gui.setup();
  gui.setName("Contour Finder");
  gui.setPosition(10, 250);
  gui.add(groundLevel.setup("ground level", 2000, 0, 10000));
  gui.add(threshold.setup("threshold", 100, 0, 400));
  gui.add(minBlobSize.setup("minBlobSize", 0.1, 0, 1));
  gui.add(maxBlobSize.setup("maxBlobSize", 1, 0, 1));
  gui.add(maxNumBlobs.setup("maxNumBlobs", 10, 1, 100));
  gui.add(findHoles.setup("findHoles", false));
  gui.add(useApproximation.setup("useApproximation", true));
  
  sndGui.setup();
  sndGui.setName("sound");
  sndGui.add(start_stop.setup("start/stop", false));
  sndGui.add(BPM.setup("BPM", 100, 20, 240));
  sndGui.add(key.setup("key", 0, 0, 12));
  sndGui.add(reset.setup("reset"));
  
  start_stop_old = true;
  changeKey_old = true;
  
  guiWidth = 220;
  getBang = true;
  radius = 10;
  
}

//--------------------------------------------------------------
void ofApp::update(){
  kinect.update();
  
  bool bNewFrame = false;
  bNewFrame = kinect.isNewFrame();
  //フレームが切り替わった際のみ画像を解析
  if (bNewFrame){
    unsigned char *kirinukiData = kirinuki.getPixels().getData();
    unsigned char *kirinukiBnWData = kirinuki_BnW.getPixels().getData();
    unsigned char *imageData = kinect.getImagePixels().getData();
    unsigned short *depthData = kinect.getDepthRawPixels().getData();
    for (int k = 0; k < 640*480; k++) {
      // 床面より上の物体のみを切り抜く
      if (0 < depthData[k] && depthData[k] < groundLevel) {
        kirinukiData[k * 3 + 0] = imageData[k * 3 + 0];
        kirinukiData[k * 3 + 1] = imageData[k * 3 + 1];
        kirinukiData[k * 3 + 2] = imageData[k * 3 + 2];
        kirinukiBnWData[k] = 255;
      } else {
        kirinukiData[k * 3 + 0] = 0;
        kirinukiData[k * 3 + 1] = 0;
        kirinukiData[k * 3 + 2] = 0;
        kirinukiBnWData[k] = 0;
      }
    }
    kirinuki.update();
    kirinuki_BnW.update();
    
    grayImage.setFromPixels(kirinuki_BnW.getPixels(), 640, 480);
    // take the abs value of the difference between background and incoming and then threshold:
    grayDiff.absDiff(grayBg, grayImage);
    grayDiff.threshold(threshold);
    contourFinder.findContours(grayDiff,
                               minBlobSize * minBlobSize * grayDiff.getWidth() * grayDiff.getHeight(),
                               maxBlobSize * maxBlobSize * grayDiff.getWidth() * grayDiff.getHeight(),
                               maxNumBlobs, findHoles, useApproximation);
    
    depthData = kinect.getDepthRawPixels().getData();
    
    // blob no num to object no num wo soroeru
    while (contourFinder.blobs.size() != object.size()){
      if (contourFinder.blobs.size() > object.size()) {
        object.push_back(new soundObject(ofPoint(contourFinder.blobs[object.size()+1].centroid.x, contourFinder.blobs[object.size()+1].centroid.y)));
        Patch p = pd.openPatch("sndO4.pd");
        sndObject.push_back(p);
      } else if (contourFinder.blobs.size() < object.size()) {
        object.pop_back();
        pd.closePatch(sndObject[object.size()]);
        sndObject.pop_back();
      }
    }
    // set parameter to object.
    int i = 0;
    for(vector <soundObject *>::iterator it = object.begin(); it != object.end(); ++it){
      (*it)->update();
      (*it)->setPos(ofPoint(contourFinder.blobs[i].centroid.x, contourFinder.blobs[i].centroid.y));
      (*it)->setColor(kinect.getImagePixels().getColor(contourFinder.blobs[i].centroid.x, contourFinder.blobs[i].centroid.y));
      (*it)->setDepth(depthData[(int)(contourFinder.blobs[i].centroid.x +  640*contourFinder.blobs[i].centroid.y)], groundLevel);
      (*it)->setSize(contourFinder.blobs[i].boundingRect.width*contourFinder.blobs[i].boundingRect.height);
      i++;
    }
  }
  
  if (getBang) {
    // Send data to Pd.
    
    if (start_stop!=start_stop_old) {
      pd.sendBang("start");
      start_stop_old = start_stop;
    }
  
    if (reset) {
      pd.sendBang("reset");
    }
    pd.sendFloat("BPM", BPM);
    pd.sendFloat("key", key);
  
    for (int i=0; i<sndObject.size(); i++) {
      pd.sendFloat(sndObject[i].dollarZeroStr()+"-length",
                   object[i]->getlength());
      pd.sendFloat(sndObject[i].dollarZeroStr()+"-vel",
                   object[i]->getVelocity());
      pd.sendFloat(sndObject[i].dollarZeroStr()+"-pulse",
                   object[i]->getPulse());
      pd.sendFloat(sndObject[i].dollarZeroStr()+"-oct",
                   object[i]->getOct());
      pd.sendFloat(sndObject[i].dollarZeroStr()+"-ch", i+1);
      pd.sendFloat(sndObject[i].dollarZeroStr()+"-pan",
                   object[i]->getPan());
      pd.sendFloat(sndObject[i].dollarZeroStr()+"-r",
                   object[i]->getColor().r);
      pd.sendFloat(sndObject[i].dollarZeroStr()+"-g",
                   object[i]->getColor().g);
      pd.sendFloat(sndObject[i].dollarZeroStr()+"-b",
                   object[i]->getColor().b);
      pd.sendFloat(sndObject[i].dollarZeroStr()+"-move",
                   object[i]->move);
    }
    getBang == false;
  }
  
  // read data from patch.
  if(pd.isQueued()) {
    // process any received messages, if you're using the queue and *do not*
    // call these, you won't receieve any messages or midi!
    pd.receiveMessages();
    pd.receiveMidi();
  }
}

//--------------------------------------------------------------
void ofApp::draw(){
  ofSetColor(255, 255, 255);
  kinect.drawImage(guiWidth, 0, 640, 480);
  contourFinder.draw(guiWidth, 0, 640, 480);
  kirinuki.draw(guiWidth, 480, 320, 240);
  kirinuki_BnW.draw(guiWidth+320, 480, 320, 240);
  
  // Show parameters of objects
  for (int i=0; i < contourFinder.blobs.size(); i++) {
    if (object[i]->bIsblinking) {
      radius = 15;
    } else {
      radius = 10;
    }
    ofSetColor(255, 255, 255);
    ofDrawCircle(guiWidth+680, 50*(i+1), radius+2);
    ofSetColor(object[i]->getColor());
    ofDrawCircle(guiWidth+680, 50*(i+1), radius);
    ofSetColor(255, 255, 255);
    ofDrawBitmapString("x:" + ofToString((int)object[i]->getPos().x), guiWidth+700, 50*(i+1));
    ofDrawBitmapString("y:" + ofToString((int)object[i]->getPos().y), guiWidth+760, 50*(i+1));
    ofDrawBitmapString("velocity:" + ofToString(object[i]->getVelocity()), guiWidth+820, 50*(i+1));
    ofDrawBitmapString("length:" + ofToString(object[i]->getlength()), guiWidth+700, 20+50*(i+1));
    ofDrawBitmapString("pulse:" + ofToString(object[i]->getPulse()), guiWidth+790, 20+50*(i+1));
    ofDrawBitmapString("oct:" + ofToString(object[i]->getOct()), guiWidth+860, 20+50*(i+1));
    ofDrawBitmapString("move:" + ofToString(object[i]->move), guiWidth+920, 20+50*(i+1));
    
  }
  
  // Draw Objects;
  for(vector <soundObject *>::iterator it = object.begin(); it != object.end(); ++it){
    (*it)->draw(guiWidth, 0);
  }
  
  ofSetColor(255, 255, 255);
  ofDrawBitmapString("sndO:"+ofToString(sndObject.size()), 10, 10);
  
  gui.draw();
  sndGui.draw();
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
  switch (key) {
      case '1':
      
      break;
      
      case '2':
      
      break;
  }
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){
  
}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){
  
}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){
  
}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){
  pd.sendBang("a");
}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){
  
}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){
  
}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){
  
}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){
  
}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){
  
}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){
  
}

//--------------------------------------------------------------
void ofApp::exit(){
  midiOut.closePort();
}

//--------------------------------------------------------------
void ofApp::audioReceived(float * input, int bufferSize, int nChannels){
  
}

//--------------------------------------------------------------
void ofApp::audioRequested(float * output, int bufferSize, int nChannels){
  pd.audioOut(output, bufferSize, nChannels);

}

void ofApp::print(const std::string& message) {
  cout << message << endl;
}

void ofApp::receiveBang(const std::string& dest) {
  cout << "OF: bang " << dest << endl;
  getBang = true;
}

void ofApp::receiveFloat(const std::string& dest, float value) {
  cout << "OF: float " << dest << ": " << value << endl;
}

void ofApp::receiveSymbol(const std::string& dest, const std::string& symbol) {
  cout << "OF: symbol " << dest << ": " << symbol << endl;
}

void ofApp::receiveList(const std::string& dest, const List& list) {
  cout << "OF: list " << dest << ": ";
  
  // step through the list
  for(int i = 0; i < list.len(); ++i) {
    if(list.isFloat(i))
      cout << list.getFloat(i) << " ";
    else if(list.isSymbol(i))
      cout << list.getSymbol(i) << " ";
  }
  
  // you can also use the built in toString function or simply stream it out
  // cout << list.toString();
  // cout << list;
  
  // print an OSC-style type string
  cout << list.types() << endl;
}

void ofApp::receiveMessage(const std::string& dest, const std::string& msg, const List& list) {
  cout << "OF: message " << dest << ": " << msg << " " << list.toString() << list.types() << endl;
}


//--------------------------------------------------------------
void ofApp::receiveNoteOn(const int channel, const int pitch, const int velocity) {
  std::cout << "note: " << channel << " " << pitch << " " << velocity << endl;
  midiOut.sendNoteOn(channel, pitch, velocity);

  if (velocity!=0) object[channel-1]->blink();
}

//--------------------------------------------------------------
void ofApp::receiveControlChange(const int channel, const int controller, const int value) {
  std::cout << "ctrl: " << channel << " " << controller << " " << value << endl;
  midiOut.sendControlChange(channel, controller, value);
}
