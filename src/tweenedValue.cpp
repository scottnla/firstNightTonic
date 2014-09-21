#include "tweenedValue.h"
#include "ofMain.h"

TweenedVariable::TweenedVariable(float startingValue) {
    currentValue = startingValue;
}

void TweenedVariable::update() {
    ofLog() << currentValue << "," << targetValue;
    if(currentValue < targetValue) {
        currentValue += delta;
    }
    else if (currentValue > targetValue) {
        currentValue -= delta;
    }
    else if(currentValue == targetValue) {
        //targetValue = 0.0;
    }
}

void TweenedVariable::setTarget(float newTarget) {
    targetValue = newTarget;
}

void TweenedVariable::setDelta(float newDelta) {
    delta = newDelta;
}

float TweenedVariable::getCurrentValue() {
    return currentValue;
}

