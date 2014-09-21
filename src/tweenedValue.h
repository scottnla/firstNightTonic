#include <vector>



class TweenedVariable {
public:
    TweenedVariable(float startingValue);
    void update();
    void setTarget(float newTarget);
    void setDelta(float newDelta);
    float getCurrentValue();
private:
    float currentValue;
    float targetValue;
    float delta;
};