#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
  ofEnableSmoothing();
  ofSetFrameRate(60);
  ofSetVerticalSync(true);

  shader.load("cortex");
  fbo.allocate(ofGetWidth(), ofGetHeight());
  sSpiral = new TweenedVariable(0);
  sSpiral->setTarget(0);
  sSpiral->setDelta(0.25);
    horzLines = new TweenedVariable(0.0);
    horzLines->setTarget(0);
    horzLines->setDelta(0.25);
    numLines = new TweenedVariable(0.0);
    numLines->setTarget(0.0);
    numLines->setDelta(0.25);
    
    
  ofSoundStreamSetup(2, 0, this, 44100, 256, 4);

  class TheSynth : public Synth{
    public:
    TheSynth(){
          float scaleArr[6] = {0,2,3,7,10};
    vector<float> scale(scaleArr, scaleArr + sizeof scaleArr / sizeof scaleArr[0]);
    
    ControlGenerator touchDown = addParameter("touchDown", 0);
    ControlGenerator speed = addParameter("speed", 0.85);
    
    ControlGenerator switchIt;// = ControlMetro().bpm(30 * speed);
    switchIt = ControlTriggerFilter().sequence("10").trigger(touchDown);
    
    ControlGenerator bpm =         ControlStepper()
          .bidirectional(1)
          .start(550)
          .end(650)
          .step(100)
          .trigger(switchIt)
        * speed;
    
    ControlMetro metro = ControlMetro().bpm(bpm);
    
      
    metro.bpm(500);
      
    publishChanges( ControlPulse().trigger(metro).length(0.05), "metro");
    publishChanges( bpm, "bpm");
    
    ADSR env = ADSR(0.01, 1.5, 0.6, 0)
      .trigger(metro)
      .doesSustain(true)
      .legato(true)
      .decay(
        ControlRandom().min(0.05).max(0.1).trigger(switchIt)
      );
    
    ControlGenerator y = addParameter("y");
    ControlGenerator x = addParameter("x");
    
    ControlXYSpeed speedCalc = ControlXYSpeed().x(x).y(y);
    ControlGenerator stepperStart = y * 20 + 43;
    
    ControlStepper stepper1 = ControlStepper()
    .bidirectional(1)
    .start( stepperStart )
    .end(stepperStart + 50 *  addParameter("stepperSpread"))
    .step(
      ControlStepper()
      .bidirectional(1)
      .start(4)
      .end(
        ControlRandom()
        .min(9)
        .max(15)
        .trigger(
          switchIt
        )
        +
        ControlStepper().start(0).end(1).step(0.01).trigger(metro)
      )
      .step(1)
      .trigger(metro)
    )
    .trigger(metro);
    
   
    ControlSnapToScale scaleSnapper1 =  ControlSnapToScale()
      .setScale(scale)
      .input( stepper1 );
      
      
    publishChanges(scaleSnapper1, "note");
    
    Generator delayMix = ControlRandom().min(0).max(0.1).trigger(switchIt).smoothed();
    
    //////// Extra layer of stuff fade envelope ////////
    
    Generator extraLayerFade = ADSR(0.01, 0, 1, 15).exponential(true).trigger(touchDown).legato(true);
    
    //////// Acid ///////
    
    ControlSwitcher melodyCutoffMultiply = ControlSwitcher();
    for(int i = 0; i < 10; i++){
      melodyCutoffMultiply.addInput(randomFloat(2, 6));
    }
    melodyCutoffMultiply.inputIndex(ControlStepper().trigger(metro).end(10));
    
    Generator oneMinusYSmoothed = 1 - y.smoothed();
    
    Generator bassFreq = ControlMidiToFreq().input(scaleSnapper1 + -12).smoothed().length(
          ControlRandom().min(0).max(0.03).trigger(switchIt)
        );
    
    Generator acid = ((
    
      (SawtoothWaveBL().freq(
        bassFreq
      ) >> LPF12().cutoff(bassFreq * melodyCutoffMultiply))
      
     +
      
      SineWave().freq(
        ControlMidiToFreq().input(scaleSnapper1 + -12 + 7).smoothed().length(
          ControlRandom().min(0).max(0.03).trigger(switchIt)
        )
      ) 
      
    )
    
    * 0.25 * env * 2 * (0.1 + oneMinusYSmoothed * oneMinusYSmoothed)) >> HPF24().cutoff(1000);
    
     acid = acid >> StereoDelay(0.05,0.07).feedback(0).wetLevel(0.5);
    
    ////// NOISE //////
    
    int maxNoisePatternLen = 10;
    ControlSwitcher noiseEnvDecay = ControlSwitcher().inputIndex(ControlStepper().end(ControlRandom().min(2).max(maxNoisePatternLen).trigger(switchIt)).trigger(metro));
    for(int i = 0; i < maxNoisePatternLen; i++){
      noiseEnvDecay.addInput(randomFloat(0.01, 0.2));
    }
    Generator noiseEnv = ADSR(0.01, 1.1, 0,0).trigger(metro).decay(noiseEnvDecay ) * extraLayerFade;
    
    Generator noise = (MonoToStereoPanner().input(PinkNoise()).pan(-1) + MonoToStereoPanner().input(PinkNoise()).pan(1)) * 0.01 * noiseEnv;
    
    /////// PADS ///////
    
    Adder padsAdder;
    
    ControlGenerator bassScaleIndex =  20  +  10 * y;
    
    for(int i = 0; i< 3; i++){
      ControlGenerator midiNote = ControlSwitcher().doesWrap(true).addAfterWrap(12).setFloatInputs(scale).inputIndex(i*2 + bassScaleIndex);
      
      padsAdder = padsAdder
      + MonoToStereoPanner()
      .input(
        SawtoothWaveBL().freq( ControlMidiToFreq().input(midiNote).smoothed().length(0.2) + SineWave().freq(3 + i) * 2) * 0.05
      ).pan(-1 + i * 2. / 3.);
    }
    
    vector<float> padSeqInputs;
    padSeqInputs.push_back(0);
    padSeqInputs.push_back(1);
     
    ControlSwitcher padsSeq = ControlSwitcher().setFloatInputs(padSeqInputs);
    ControlGenerator padsTrigger = ControlTriggerFilter().trigger(metro).sequence("1000|1000|1000|0010|");
    publishChanges(padsTrigger, "padsTrigger");
    publishChanges(ControlPulse().trigger(padsTrigger));
    Generator pads = padsAdder >> LPF24().cutoff(5000 * speedCalc + 500 + 1500 * ADSR(0.001, 0.1,0,0).trigger(padsTrigger) );
    
    
    // pads high note
    ControlGenerator highNoteMidiNote = ControlSwitcher().doesWrap(true).addAfterWrap(12).setFloatInputs(scale).inputIndex(bassScaleIndex) + 36;
    pads = pads + SineWave().freq(ControlMidiToFreq().input(highNoteMidiNote).smoothed().length(0.2)) * 0.05 * ADSR(0,0.05,0,0).trigger(padsTrigger);
    
    pads = pads >> StereoDelay(0.2, 0.22).wetLevel(0.2);
    
    // bassy note for pads
    ControlGenerator bassNote = ControlValue(43);
    pads = pads + SineWave().freq(bassNote) * 0.1;
    pads = pads * ADSR(0.01,0.3,0.2,0).legato(true).trigger(padsTrigger);
    
    //////////// Kick /////////
    
    ADSR kickEnv = ADSR(0.001, 0.05,0,0).trigger(ControlTriggerFilter().trigger(metro).sequence("1000|1000|1000|1010"));
    Generator kick = SineWave().freq(50 + 200 * kickEnv) * kickEnv * 0.1;
    
    ///////////// Bass /////////
    
    ControlGenerator bassTrig;
    bassTrig = ControlTriggerFilter().trigger(metro).sequence("0010|0000|1000|0000");
    Generator bass = SineWave().freq( 50 + ADSR(0.001, 1, 0,0).trigger(bassTrig) ) * ADSR(0.001, 1.0, 0,0).trigger(bassTrig).legato(true) * 0.1;
    
    //////////////////
   setLimitOutput(false);
    setOutputGen((acid +  pads + noise + kick) * 2);
//    setOutputGen(noise + kick + bass);
    
    }
  };

  synth = TheSynth();
  synth.addControlChangeSubscriber(this);

}


void ofApp::valueChanged(string name, TonicFloat value){
  if(name == "padsTrigger"){
      sSpiral->setTarget(1.0);
  }
}

//--------------------------------------------------------------
void ofApp::update(){
    sSpiral->update();
    if(sSpiral->getCurrentValue() == 1.0) {
        sSpiral->setTarget(0.0);
    }
    numLines->update();
    horzLines->update();
}

//--------------------------------------------------------------
void ofApp::draw(){
    
  ofClear(0,0,0);
  // this will cause the synth to call valueChanged if there are value changes to report
    fbo.begin();
    shader.begin();
    setUniforms();
    ofRect(0,0,ofGetWindowWidth(), ofGetWindowHeight());
    shader.end();
    fbo.end();
    fbo.draw(0,0,ofGetWindowWidth(), ofGetWindowHeight());
    synth.sendControlChangesToSubscribers();
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){

}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){
  synth.setParameter("x", (float)x / ofGetWindowWidth());
  synth.setParameter("y", 1.0 - (float)y / ofGetWindowHeight());
}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){
  
}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

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
void ofApp::audioRequested (float * output, int bufferSize, int nChannels){
  synth.fillBufferOfFloats(output, bufferSize, nChannels);
}

//--------------------------------------------------------------
void ofApp::setUniforms(){
    float resolution[] = {ofGetWindowWidth(), ofGetWindowHeight()};
    float time = 0.01*ofGetFrameNum();
    
    shader.setUniform1f("time",time);
    shader.setUniform2fv("resolution",resolution);
    
    //flags for turning patterns on/off
    shader.setUniform1i("bVert", true);
    shader.setUniform1i("bHorizon", true);
    shader.setUniform1i("bDiag", true);
    shader.setUniform1i("bDiagAlt", true);
    shader.setUniform1i("bArms", true);
    shader.setUniform1i("bRings", true);
    shader.setUniform1i("bSpiral", true);
    shader.setUniform1i("bSpiralAlt", true);
    
    //scalars for tweening
    shader.setUniform1f("sVert", sSpiral->getCurrentValue());
    shader.setUniform1f("sHorizon", 0);
    shader.setUniform1f("sDiag", 0);
    shader.setUniform1f("sDiagAlt", 0);
    shader.setUniform1f("sArms", 0.0);
    shader.setUniform1f("sRings", 1.0);
    shader.setUniform1f("sSpiral", 0.0);
    shader.setUniform1f("sSpiralAlt", 0.0);
    
    //pattern periods
    shader.setUniform1f("vertPeriod", 0.5);
    shader.setUniform1f("horizonPeriod", 0.5);
    shader.setUniform1f("diagPeriod", 0);
    shader.setUniform1f("diagAltPeriod", 0);
    shader.setUniform1f("armPeriod", 0.5);
    shader.setUniform1f("ringPeriod", 0.5);
    shader.setUniform1f("spiralPeriod", 0.75);
    shader.setUniform1f("spiralAltPeriod", 24.0);
    
    //pattern parameters
    shader.setUniform1f("numVert", 5);
    shader.setUniform1f("numHorizon", 0);
    shader.setUniform1f("numDiag", 0);
    shader.setUniform1f("numDiagAlt", 0);
    shader.setUniform1f("numArms", 5.0);
    shader.setUniform1f("numRings", 1.0);
    shader.setUniform1f("numSpiral", 3.0);
    shader.setUniform1f("numSpiralAlt", 3.0);
}
