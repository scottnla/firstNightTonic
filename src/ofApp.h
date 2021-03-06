#pragma once

#include "ofMain.h"
#include "ofxTonic.h"
#include "tweenedValue.h"

using namespace Tonic;

class ofApp : public ofBaseApp, public ControlChangeSubscriber{

  Synth synth;
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
		void windowResized(int w, int h);
		void dragEvent(ofDragInfo dragInfo);
		void gotMessage(ofMessage msg);
        void setUniforms();
    
        void audioRequested (float * output, int bufferSize, int nChannels);
  
        // implementation of virtual method in ControlChangeSubscriber
        void valueChanged(string, TonicFloat);
 private:
    ofShader shader;
    ofFbo fbo;
    TweenedVariable* sSpiral;
    TweenedVariable* numLines;
    TweenedVariable* horzLines;
};
